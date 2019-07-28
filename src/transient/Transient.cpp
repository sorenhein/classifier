#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <math.h>

#include "Transient.h"
#include "Candidate.h"

#include "../const.h"

#include "../util/io.h"


// Current working definition of a transient:
// * As in Candidate.h
// * Ratio between beginning and end values is at least 3.
// * Ratio between beginning and middle values is at least 1.7.
// * Has a bit of a slope towards zero, at least.
// * TODO: Should be independent of sample rate.

#define TRANSIENT_RATIO_FULL 3.
#define TRANSIENT_RATIO_MID 1.7

#define TRANSIENT_LARGE_DEV 10.
#define SAMPLE_RATE 2000.

#define SEPARATOR ";"


Transient::Transient()
{
}


Transient::~Transient()
{
}


void Transient::reset()
{
  transientType = TRANSIENT_NONE;
  status = TSTATUS_SIZE;

  firstBuildupSample = 0;
  buildupLength = 0;
  buildupStart = 0.;
  transientLength = 0;

  transientAmpl = 0.;
  timeConstant = 0.;
}


bool Transient::findEarlyPeak(
  const vector<float>& samples,
  const double sampleRate,
  const Run& run)
{
  // There might be a quasi-linear piece (buildup) before the actual
  // transient.  The transient itself then starts when a 
  // number of samples in a row are moving towards zero.

  // If the transient drops as exp(-t/tau), then in a single time
  // step of 1/fs (sample frequency), it drops by a factor of
  // exp(-1 / (fs * tau)) ~= 1 - 1/(fs * tau) when fs is large
  // relative to 1/tau.

  const float ratioLimit = 
    1.f - 1.f / (static_cast<float>(sampleRate) * TRANSIENT_MAX_TAU);

  float diff = samples[run.first+1] - samples[run.first];
  bool posSlopeFlag = (diff >= 0. ? true : false);
  unsigned runLen = 2;

  for (unsigned i = run.first + 2; i < run.first + run.len; i++)
  {
    diff = samples[i] - samples[i-1];

    if ((posSlopeFlag && diff >= 0.) || (! posSlopeFlag && diff < 0.))
    {
      // Continuing the slope towards the zero line.
      runLen++;
    }
    else if (posSlopeFlag != run.posFlag &&
        runLen >= TRANSIENT_SMALL_RUN &&
        samples[i-1] / samples[i-runLen] <= ratioLimit)
    {
      // This is either a negative transient heading positive,
      // or a positive transient heading negative.  In either case
      // it is trending toward the zero line.  It is not a great test, 
      // as it could happen that we get a spurious early run followed 
      // by the actual run.

      firstBuildupSample = run.first;
      buildupLength = i - run.first - runLen;
      buildupStart = samples[run.first];
      transientLength = run.len - buildupLength;
      return true;
    }
    else
    {
      posSlopeFlag = ! posSlopeFlag;
      runLen = 2;
    }
  }
  return false;
}


bool Transient::checkDecline(
  const vector<float>& samples,
  const Run& run)
{
  // A good transient moves from large to medium to small
  // over the course of the transient.

  const unsigned first = run.first + buildupLength;
  const unsigned last = run.first + run.len;
  const unsigned mid = (first + last - TRANSIENT_SMALL_RUN) >> 1;

  float sumFront = 0., sumMid = 0., 
    sumBack = 0., sumAlmostBack = 0.;

  for (unsigned i = first; i < first + TRANSIENT_SMALL_RUN; i++)
    sumFront += samples[i];

  for (unsigned i = mid; i < mid + TRANSIENT_SMALL_RUN; i++)
    sumMid += samples[i];

  for (unsigned i = last - TRANSIENT_SMALL_RUN; i < last; i++)
    sumBack += samples[i];

  for (unsigned i = last-TRANSIENT_SMALL_RUN-1; i < last-1; i++)
    sumAlmostBack += samples[i];

  if (sumFront < 0.)
  {
    sumFront = -sumFront;
    sumMid = -sumMid;
    sumBack = -sumBack;
    sumAlmostBack = -sumAlmostBack;
  }

  if (sumFront < TRANSIENT_RATIO_FULL * sumBack &&
      sumFront < TRANSIENT_RATIO_FULL * sumAlmostBack)
  {
    // In very choppy traces, the last sample can be atypical,
    // so we also permit skipping this.
    status = TSTATUS_NO_FULL_DECLINE;
    return false;
  }

  if (sumFront < TRANSIENT_RATIO_MID * sumMid)
  {
    status = TSTATUS_NO_MID_DECLINE;
    return false;
  }

  return true;
}


void Transient::estimateTransientParams(
  const vector<float>& samples,
  const Run& run)
{
  // This doesn't have to be so accurate, so the basic idea
  // is to use the integral of A * exp(-t/tau) and the
  // integral of A * t * exp(-t/tau).  The first one is
  // approximately A * tau, and the second one A * tau^2.

  float cum = 0., vcum = 0.;
  const unsigned f = firstBuildupSample + buildupLength;
  for (unsigned i = f; i < f + run.len; i++)
  {
    cum += samples[i];
    vcum += (i - f) * samples[i];
  }
  cum /= SAMPLE_RATE;
  vcum /= (SAMPLE_RATE * SAMPLE_RATE);

  // Actually we can also just use the first sample.
  // Because the math is so pretty, we average the two,
  // but in truth cand2 is probably better...

  const float cand1 = cum * cum / vcum;
  const float cand2 = samples[f];

  transientAmpl = 0.85f * cand2 + 0.15f * cand1;
  timeConstant = 1000.f * cum / transientAmpl;

  // This is purely informational, so the values are not
  // so important.  It probably tends to say something about
  // the sensor hardware.
  if (transientAmpl >= 40. && 
      timeConstant >= 5. && timeConstant <= 17.)
    transientType = TRANSIENT_LARGE_POS;
  else if (transientAmpl <= -40. && 
      timeConstant >= 5. && timeConstant <= 17.)
    transientType = TRANSIENT_LARGE_NEG;
  else if (transientAmpl >= 15. && 
      timeConstant >= 5. && timeConstant <= 17.)
    transientType = TRANSIENT_MEDIUM;
  else if (transientAmpl <= 3. && transientAmpl >= -3.)
    transientType = TRANSIENT_SMALL;
  else
    transientType = TRANSIENT_SIZE;
}


void Transient::synthesize()
{
  const unsigned l = buildupLength + transientLength;
  synth.resize(l);

  // Synthesize the early quasi-linear bit.
  for (unsigned i = 0; i < buildupLength; i++)
    synth[i] = static_cast<float>(buildupStart + 
      i * (transientAmpl - buildupStart) / buildupLength);

  // Synthesize the quasi-exponential part.
  for (unsigned i = buildupLength; i < l; i++)
    synth[i] = static_cast<float> (transientAmpl * exp(
      - static_cast<float>(i - buildupLength) / SAMPLE_RATE / 
          (timeConstant / 1000.)));
}


bool Transient::errorIsSmall(const vector<float>& samples)
{
  const unsigned l = synth.size();
  float err = 0.;
  for (unsigned i = 0; i < l; i++)
  {
    const float diff = synth[i] - samples[firstBuildupSample + i];
    err += diff * diff;
  }

  // Not currently possibly to fail this test.
  fitError = sqrt(err / l);
  return true;
}


bool Transient::largeActualDeviation(
  const vector<float>& samples,
  const Run& run,
  unsigned& devpos) const
{
  // We stop the transient if there's a big bump away from
  // the middle in the actual trace.  A big bump is two 
  // consecutive samples or three with enough mass.

  for (unsigned i = firstBuildupSample + buildupLength;
    i < run.first + run.len - 3; i++)
  {
    float diff1 = samples[i+1] - samples[i];
    float diff2 = samples[i+2] - samples[i];
    float diff3 = samples[i+3] - samples[i];
    if (samples[i] < 0.)
    {
      diff1 = -diff1;
      diff2 = -diff2;
      diff3 = -diff3;
    }

    if (diff1 > TRANSIENT_LARGE_DEV &&
        diff2 > TRANSIENT_LARGE_DEV)
    {
      devpos = i;
      return true;
    }
    else if (diff1 > 0 && diff2 > 0 && diff3 > 0 &&
        diff1 + diff2 + diff3 > 2. * TRANSIENT_LARGE_DEV)
    {
      devpos = i;
      return true;
    }
  }
  return false;
}


bool Transient::largeSynthDeviation(
  const vector<float>& samples,
  unsigned& devpos) const
{
  // We also consider two consecutive, large deviations from
  // the synthesized transient to signal the end.

  for (unsigned i = 1; i < synth.size()-1; i++)
  {
    const unsigned j = firstBuildupSample + i;
    const float diff1 = samples[j] - synth[i];
    const float diff2 = samples[j+1] - synth[i+1];

    if (samples[j] < 0. && 
        diff1 < -TRANSIENT_LARGE_DEV &&
        diff2 < -TRANSIENT_LARGE_DEV)
    {
      devpos = i-1;
      return true;
    }
    else if (samples[j] > 0. && 
        diff1 > TRANSIENT_LARGE_DEV &&
        diff2 > TRANSIENT_LARGE_DEV)
    {
      devpos = i-1;
      return true;
    }
  }
  return false;
}



bool Transient::detect(
  const vector<float>& samples,
  const double sampleRate)
{
  Transient::reset();
  Candidate candidate;
  Run run;

  // Look for a candidate run with the same polarity.
  if (! candidate.detect(samples, sampleRate, run, 
      transientType, status))
  {
    transientType = TRANSIENT_NONE;
    return false;
  }

  // Look for the place within the run to start the transient.
  if (! Transient::findEarlyPeak(samples, sampleRate, run))
  {
    // We won't give up yet.
    firstBuildupSample = run.first;
    transientLength = run.len;
  }

  // Could be a large signal away from zero towards the end.
  unsigned devpos;
  Run runcorr = run;

  if (Transient::largeActualDeviation(samples, run, devpos))
  {
    runcorr.len = devpos - run.first;
    transientLength = devpos - run.first - buildupLength;
    status = TSTATUS_BACK_ACTUAL_CORR;
  }

  if (! Transient::checkDecline(samples, runcorr))
  {
    // Could be a very choppy trace where it makes sense
    // to start from the beginning.
    buildupLength = 0;
    if (Transient::checkDecline(samples, runcorr))
    {
      firstBuildupSample = runcorr.first;
      buildupStart = 0.;
      transientLength = runcorr.len;
      status = TSTATUS_FRONT_ACTUAL_CORR;
    }
    else
    {
      transientType = TRANSIENT_NONE;
      return false;
    }
  }

  Transient::estimateTransientParams(samples, runcorr);

  Transient::synthesize();

  if (Transient::largeSynthDeviation(samples, devpos))
  {
    runcorr.len = devpos;
    transientLength = devpos - buildupLength;
    status = TSTATUS_BACK_SYNTH_CORR;

    // Redo the estimation.
    Transient::estimateTransientParams(samples, runcorr);

    Transient::synthesize();
  }

  if (! Transient::errorIsSmall(samples))
  {
    // Currently not possible.  fitError is probably an
    // indication of a signal being present during the transient.
    transientType = TRANSIENT_NONE;
    status = TSTATUS_BAD_FIT;
    return false;
  }

  return true;
}


unsigned Transient::lastSampleNo() const
{
  return firstBuildupSample + buildupLength + transientLength;
}


void Transient::writeFile(const string& filename) const
{
  if (transientType == TRANSIENT_NONE ||
      transientType == TRANSIENT_SIZE)
    return;

  writeBinary(filename, firstBuildupSample, synth);
}


string Transient::strHeader() const
{
  stringstream ss;
  ss << 
    setw(8) << "Prepos" <<
    setw(8) << "Prelen" <<
    setw(10) << "Pre-ampl" <<
    setw(6) << "Len" <<
    setw(10) << "Ampl (g)" <<
    setw(10) << "Tau (ms)" << "\n";

  return ss.str();
}


string Transient::str() const
{
  stringstream ss;
  ss << "Transient\n";
  ss << string(9, '-') << "\n\n";

  if (transientType == TRANSIENT_NONE)
  {
    if (status == TSTATUS_TOO_FEW_RUNS)
      ss << "Too few runs found";
    else if (status == TSTATUS_NO_CANDIDATE_RUN)
      ss << "No candidate run found";
    else if (status == TSTATUS_NO_FULL_DECLINE)
      ss << "Didn't decline enough over full run";
    else if (status == TSTATUS_NO_MID_DECLINE)
      ss << "Didn't decline enough in the middle";
    else
      ss << "Reason not known (ERROR)";
    return ss.str() + "\n\n";
  }
  else if (transientType == TRANSIENT_SIZE)
    return ss.str() + "Search for transient not run\n\n";

  ss << Transient::strHeader();
  ss << 
    setw(8) << firstBuildupSample <<
    setw(8) << buildupLength <<
    setw(10) << fixed << setprecision(2) << buildupStart <<
    setw(6) << transientLength <<
    setw(10) << fixed << setprecision(2) << transientAmpl <<
    setw(10) << fixed << setprecision(2) << timeConstant << "\n\n";

  return ss.str();
}


string Transient::strHeaderCSV() const
{
  stringstream ss;
  ss << 
    "Status" << SEPARATOR <<
    "Type" << SEPARATOR <<
    "Prepos" << SEPARATOR <<
    "Prelen" << SEPARATOR <<
    "Level" << SEPARATOR <<
    "Len" << SEPARATOR <<
    "Ampl" << SEPARATOR <<
    "Tau";

  return ss.str();
}


string Transient::strCSV() const
{
  stringstream ss;
  ss << 
    Transient::strHeaderCSV() <<
    static_cast<unsigned>(status) << SEPARATOR <<
    static_cast<unsigned>(transientType) << SEPARATOR <<
    firstBuildupSample << SEPARATOR <<
    buildupLength << SEPARATOR <<
    fixed << setprecision(2) << buildupStart << SEPARATOR <<
    transientLength << SEPARATOR <<
    fixed << setprecision(2) << transientAmpl << SEPARATOR <<
    fixed << setprecision(2) << timeConstant;
  
  return ss.str();
}

