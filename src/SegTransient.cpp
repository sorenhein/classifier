#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <math.h>

#include "SegTransient.h"
#include "Trace.h"
#include "write.h"


// Current working definition of a transient:
// * One of the first 5 runs.
// * Covers at least 20 samples.
// * Average at least 0.3g.
// * Ratio between beginning and end values is at least 3.
// * Ratio between beginning and middle values is at least 1.7.
// * Has a bit of a slope towards zero, at least.
// * TODO: Should be independent of sample rate.

#define TRANSIENT_RANGE 5
#define TRANSIENT_MIN_LENGTH 20
#define TRANSIENT_MIN_AVG 0.3
#define TRANSIENT_RATIO_FULL 3.
#define TRANSIENT_RATIO_MID 1.7
#define TRANSIENT_SMALL_RUN 3
#define TRANSIENT_STEP 0.98

#define TRANSIENT_LARGE_DEV 10.
#define SAMPLE_RATE 2000.

#define SEPARATOR ";"


SegTransient::SegTransient()
{
}


SegTransient::~SegTransient()
{
}


void SegTransient::reset()
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


bool SegTransient::detectPossibleRun(
  const vector<Run>& runs,
  unsigned& rno)
{
  if (runs.size() < TRANSIENT_RANGE)
  {
    status = TSTATUS_TOO_FEW_RUNS;
    return false;
  }

  for (unsigned i = 0; i < TRANSIENT_RANGE; i++)
  {
    if (runs[i].len < TRANSIENT_MIN_LENGTH)
      continue;

    if (runs[i].cum < TRANSIENT_MIN_AVG * runs[i].len)
      continue;
    
    rno = i;
    return true;
  } 

  status = TSTATUS_NO_CANDIDATE_RUN;
  return false;
}


bool SegTransient::findEarlyPeak(
  const vector<double>& samples,
  const Run& run)
{
  // There might be a quasi-linear piece before the actual
  // transient.  The transient itself then starts when a 
  // number of samples in a row are moving towards zero.

  double diff = samples[run.first+1] - samples[run.first];
  bool posSlopeFlag = (diff >= 0. ? true : false);
  unsigned runLen = 2;

  for (unsigned i = run.first + 2; i < run.first + run.len; i++)
  {
    // TODO Combine this code at the cost of abstraction
    diff = samples[i] - samples[i-1];
    if (posSlopeFlag)
    {
      if (diff >= 0.)
        runLen++;
      else if (! run.posFlag && 
          runLen >= TRANSIENT_SMALL_RUN &&
          samples[i-1] / samples[i-runLen] <= TRANSIENT_STEP)
      {
        // This is not a great test, as it could happen
        // that we get a spurious early run followed by the
        // actual run.

        // It's a negative transient, and we've got enough
        // positively sloping samples.
        firstBuildupSample = run.first;
        buildupLength = i - run.first - runLen;
        buildupStart = samples[run.first];
        transientLength = run.len - buildupLength;
        return true;
      }
      else
      {
        posSlopeFlag = false;
        runLen = 2;
      }
    }
    else if (diff < 0.)
      runLen++;
    else if (run.posFlag && 
        runLen >= TRANSIENT_SMALL_RUN &&
        samples[i-1] / samples[i-runLen] <= TRANSIENT_STEP)
    {
      // It's a positive transient, and we've got enough
      // negatively sloping samples.
      firstBuildupSample = run.first;
      buildupLength = i - run.first - runLen;
      buildupStart = samples[run.first];
      transientLength = run.len - buildupLength;
      return true;
    }
    else
    {
      posSlopeFlag = true;
      runLen = 2;
    }
  }
  return false;
}


bool SegTransient::checkDecline(
  const vector<double>& samples,
  const Run& run)
{
  // A good transient moves from large to medium to small
  // over the course of the transient.

  const unsigned first = run.first + buildupLength;
  const unsigned last = run.first + run.len;
  const unsigned mid = (first + last - TRANSIENT_SMALL_RUN) >> 1;

  double sumFront = 0., sumMid = 0., 
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


void SegTransient::estimateTransientParams(
  const vector<double>& samples,
  const Run& run)
{
  // This doesn't have to be so accurate, so the basic idea
  // is to use the integral of A * exp(-t/tau) and the
  // integral of A * t * exp(-t/tau).  The first one is
  // approximately A * tau, and the second one A * tau^2.

  double cum = 0., vcum = 0.;
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

  const double cand1 = cum * cum / vcum;
  const double cand2 = samples[f];

  transientAmpl = 0.85 * cand2 + 0.15 * cand1;
  timeConstant = 1000. * cum / transientAmpl;

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


void SegTransient::synthesize()
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
      - static_cast<double>(i - buildupLength) / SAMPLE_RATE / 
          (timeConstant / 1000.)));
}


bool SegTransient::errorIsSmall(const vector<double>& samples)
{
  const unsigned l = synth.size();
  double err = 0.;
  for (unsigned i = 0; i < l; i++)
  {
    const double diff = synth[i] - samples[firstBuildupSample + i];
    err += diff * diff;
  }

  // Not currently possibly to fail this test.
  fitError = sqrt(err / l);
  return true;
}


bool SegTransient::largeActualDeviation(
  const vector<double>& samples,
  const Run& run,
  unsigned& devpos) const
{
  // We stop the transient if there's a big bump away from
  // the middle in the actual trace.  A big bump is two 
  // consecutive samples or three with enough mass.

  for (unsigned i = firstBuildupSample + buildupLength;
    i < run.first + run.len - 3; i++)
  {
    double diff1 = samples[i+1] - samples[i];
    double diff2 = samples[i+2] - samples[i];
    double diff3 = samples[i+3] - samples[i];
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


bool SegTransient::largeSynthDeviation(
  const vector<double>& samples,
  unsigned& devpos) const
{
  // We also consider two consecutive, large deviations from
  // the synthesized transient to signal the end.

  for (unsigned i = 1; i < synth.size()-1; i++)
  {
    const unsigned j = firstBuildupSample + i;
    const double diff1 = samples[j] - synth[i];
    const double diff2 = samples[j+1] - synth[i+1];

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


bool SegTransient::detect(
  const vector<double>& samples,
  const vector<Run>& runs)
{
  SegTransient::reset();

  unsigned rno;
  if (! SegTransient::detectPossibleRun(runs, rno))
  {
    transientType = TRANSIENT_NONE;
    return false;
  }

  const Run& run = runs[rno];
  if (! SegTransient::findEarlyPeak(samples, run))
  {
    // We won't reject the entire transient just because we
    // can't easily find the pattern.
    firstBuildupSample = run.first;
    buildupLength = 0;
    buildupStart = 0.;
    transientLength = run.len;
  }

  // Could be a large signal away from zero towards the end.
  unsigned devpos;
  Run runcorr = run;

  if (SegTransient::largeActualDeviation(samples, run, devpos))
  {
    runcorr.len = devpos - run.first;
    transientLength = devpos - run.first - buildupLength;
    status = TSTATUS_BACK_ACTUAL_CORR;
  }

  if (! SegTransient::checkDecline(samples, runcorr))
  {
    // Could be a very choppy trace where it makes sense
    // to start from the beginning.
    buildupLength = 0;
    if (SegTransient::checkDecline(samples, runcorr))
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

  SegTransient::estimateTransientParams(samples, runcorr);

  SegTransient::synthesize();

  if (SegTransient::largeSynthDeviation(samples, devpos))
  {
    runcorr.len = devpos;
    transientLength = devpos - buildupLength;
    status = TSTATUS_BACK_SYNTH_CORR;

    // Redo the estimation.
    SegTransient::estimateTransientParams(samples, runcorr);

    SegTransient::synthesize();
  }

  if (! SegTransient::errorIsSmall(samples))
  {
    // Currently not possible.  fitError is probably an
    // indication of a signal being present during the transient.
    transientType = TRANSIENT_NONE;
    status = TSTATUS_BAD_FIT;
    return false;
  }

  return true;
}


unsigned SegTransient::lastSampleNo() const
{
  return firstBuildupSample + buildupLength + transientLength;
}


void SegTransient::writeFile(const string& filename) const
{
  if (transientType == TRANSIENT_NONE ||
      transientType == TRANSIENT_SIZE)
    return;

  writeBinary(filename, firstBuildupSample, synth);
}


string SegTransient::strHeader() const
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


string SegTransient::str() const
{
  stringstream ss;
  ss << "Transient\n";

  if (transientType == TRANSIENT_NONE)
  {
    string s = "No transient found: ";
    if (status == TSTATUS_TOO_FEW_RUNS)
      s += "Too few runs found\n";
    else if (status == TSTATUS_NO_CANDIDATE_RUN)
      s += "No candidate run found\n";
    else if (status == TSTATUS_NO_FULL_DECLINE)
      s += "Didn't decline enough over full run\n";
    else if (status == TSTATUS_NO_MID_DECLINE)
      s += "Didn't decline enough in the middle\n";
    else
      s += "Reason not known (ERROR)\n";
    return ss.str() + s;
  }
  else if (transientType == TRANSIENT_SIZE)
    return ss.str() + "Search for transient not run\n";

  ss << SegTransient::strHeader();
  ss << 
    setw(8) << firstBuildupSample <<
    setw(8) << buildupLength <<
    setw(10) << fixed << setprecision(2) << buildupStart <<
    setw(6) << transientLength <<
    setw(10) << fixed << setprecision(2) << transientAmpl <<
    setw(10) << fixed << setprecision(2) << timeConstant << "\n";

  return ss.str();
}


string SegTransient::headerCSV() const
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


string SegTransient::strCSV() const
{
  stringstream ss;
  ss << 
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

