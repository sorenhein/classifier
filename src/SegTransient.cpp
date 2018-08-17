#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <math.h>

#include "SegTransient.h"
#include "Trace.h"


// Current working definition of a transient:
// * One of the first 5 runs.
// * Covers at least 20 samples.
// * Average at least 0.3g.
// * Ratio between beginning and end values is at least 3.
// * Ratio between beginning and middle values is at least 1.7.
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
  transientType = TRANSIENT_NONE;
}


SegTransient::~SegTransient()
{
}


bool SegTransient::detectPossibleRun(
  const vector<Run>& runs,
  unsigned& rno) const
{
  if (runs.size() < TRANSIENT_RANGE)
    return false;

  for (unsigned i = 0; i < TRANSIENT_RANGE; i++)
  {
    if (runs[i].len < TRANSIENT_MIN_LENGTH)
      continue;

    if (runs[i].cum < TRANSIENT_MIN_AVG * runs[i].len)
      continue;
    
    rno = i;
    return true;
  } 
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
  const Run& run) const
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

  if (sumFront < TRANSIENT_RATIO_FULL * sumBack)
{
cout << "Decline: Failed full\n";
cout << sumFront << ", " << sumBack << ", " <<
  first << ", " << last << endl;
    // In very choppy traces, the last trace can be atypical.
    if (sumFront < TRANSIENT_RATIO_FULL * sumAlmostBack)
      return false;
}
  if (sumFront < TRANSIENT_RATIO_MID * sumMid)
{
cout << "Decline: Failed mid\n";
cout << sumFront << ", " << sumMid << ", " <<
  first << ", " << last << endl;
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

  const double cand1 = cum * cum / vcum;
  const double cand2 = samples[f];

  // Probably cand2 is actually better in practice.
  transientAmpl = 0.85 * cand2 + 0.15 * cand1;
  timeConstant = 1000. * cum / transientAmpl;
  // timeConstant = 1000. * vcum / cum;

  // This is purely informational, so the values are not
  // so important.  It probably tends to say something about
  // the sensor hardware.
  if (timeConstant >= 40. && run.cum >= 400.)
    transientType = TRANSIENT_LARGE;
  else if (timeConstant >= 15. && run.cum >= 150.)
    transientType = TRANSIENT_MEDIUM;
  else
    transientType = TRANSIENT_SMALL;
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

  fitError = sqrt(err / l);
  return true;
}


bool SegTransient::largeActualDeviation(
  const vector<double>& samples,
  const Run& run,
  unsigned& devpos) const
{
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
  // We consider two consecutive, large deviations from
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

  // Could be a large signal away from zero towards
  // the end.
  unsigned devpos;
  Run runtmp = run;

  if (SegTransient::largeActualDeviation(samples, run, devpos))
  {
    cout << "ACTUALDEV " << devpos << "\n";
    runtmp.len = devpos - run.first;
    transientLength = devpos - run.first - buildupLength;
  }

  if (! SegTransient::checkDecline(samples, runtmp))
  {
    // Could be a very choppy trace where it makes sense
    // to start from the beginning.
    buildupLength = 0;
    if (SegTransient::checkDecline(samples, runtmp))
    {
cout << "SALVAGED\n";
      firstBuildupSample = runtmp.first;
      buildupStart = 0.;
      transientLength = runtmp.len;
    }
    else
    {
      transientType = TRANSIENT_NONE;
      return false;
    }

  }

  SegTransient::estimateTransientParams(samples, runtmp);

  SegTransient::synthesize();

  if (SegTransient::largeSynthDeviation(samples, devpos))
  {
    cout << "LARGE DEVIATION " << devpos << "\n";
    runtmp.len = devpos;
    transientLength = devpos - buildupLength;

cout << "HAD ampl " << transientAmpl << ", " <<
  timeConstant << ", " << synth.size() << "\n";
    // Redo the estimation.
    SegTransient::estimateTransientParams(samples, runtmp);

    SegTransient::synthesize();
cout << "NOW ampl " << transientAmpl << ", " <<
  timeConstant << ", " << synth.size() << "\n";
  }

  if (! SegTransient::errorIsSmall(samples))
  {
    // Currently not possible.  fitError is probably an
    // indication of a signal being present during the transient.
    transientType = TRANSIENT_NONE;
    return false;
  }

cout << "Returning TRUE, type " << transientType << "\n";
  return true;
}


void SegTransient::writeBinary(const string& origname) const
{
  if (transientType == TRANSIENT_NONE ||
      transientType == TRANSIENT_SIZE)
    return;

  // Make the transient file name by
  // * Replacing /raw/ with /transient/
  // * Adding _offset_N before .dat

  string tname = origname;
  auto tp1 = tname.find("/raw/");
  if (tp1 == string::npos)
    return;

  auto tp2 = tname.find(".dat");
  if (tp2 == string::npos)
    return;

  tname.insert(tp2, "_offset_" + to_string(firstBuildupSample));
  tname.replace(tp1, 5, "/transient/");

  ofstream fout(tname, std::ios::out | std::ios::binary);

  fout.write(reinterpret_cast<const char *>(synth.data()),
    synth.size() * sizeof(float));
  fout.close();
}


string SegTransient::strCSV() const
{
  stringstream ss;
  ss << transientType << SEPARATOR <<
    firstBuildupSample << SEPARATOR <<
    buildupLength << SEPARATOR <<
    fixed << setprecision(2) << buildupStart << SEPARATOR <<
    transientLength << SEPARATOR <<
    fixed << setprecision(2) << transientAmpl << SEPARATOR <<
    fixed << setprecision(2) << timeConstant;
  
  return ss.str();
}

