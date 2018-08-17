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
// * Average at least 0.5g.
// * Ratio between beginning and end values is at least 3.
// * Ratio between beginning and middle values is at least 1.7.
// * TODO: Should be independent of sample rate.

#define TRANSIENT_RANGE 5
#define TRANSIENT_MIN_LENGTH 20
#define TRANSIENT_MIN_AVG 0.5
#define TRANSIENT_RATIO_FULL 3.
#define TRANSIENT_RATIO_MID 1.7
#define TRANSIENT_SMALL_RUN 3
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
  unsigned runLen = 1;

  for (unsigned i = run.first + 2; i < run.first + run.len; i++)
  {
    diff = samples[i] - samples[i-1];
    if (posSlopeFlag)
    {
      if (diff >= 0.)
        runLen++;
      else if (! run.posFlag && runLen >= TRANSIENT_SMALL_RUN)
      {
        // It's a negative transient, and we've got enough
        // positively sloping samples.
        firstBuildupSample = run.first;
        buildupLength = i - run.first;
        buildupStart = samples[run.first];
        transientLength = run.len - buildupLength;
        return true;
      }
      else
      {
        posSlopeFlag = false;
        runLen = 1;
      }
    }
    else if (diff < 0.)
      runLen++;
    else if (run.posFlag && runLen >= TRANSIENT_SMALL_RUN)
    {
      // It's a positive transient, and we've got enough
      // negatively sloping samples.
      firstBuildupSample = run.first;
      buildupLength = i - run.first;
      buildupStart = samples[run.first];
      transientLength = run.len - buildupLength;
      return true;
    }
    else
    {
      posSlopeFlag = true;
      runLen = 1;
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

  double sumFront = 0., sumMid = 0., sumBack = 0.;

  for (unsigned i = first; i < first + TRANSIENT_SMALL_RUN; i++)
    sumFront += samples[i];

  for (unsigned i = mid; i < mid + TRANSIENT_SMALL_RUN; i++)
    sumMid += samples[i];

  for (unsigned i = last - TRANSIENT_SMALL_RUN; i < last; i++)
    sumBack += samples[i];

  if (sumFront < 0.)
  {
    sumFront = -sumFront;
    sumMid = -sumMid;
    sumBack = -sumBack;
  }

  if (sumFront < TRANSIENT_RATIO_FULL * sumBack)
    return false;
  else if (sumFront < TRANSIENT_RATIO_MID * sumMid)
    return false;
  else
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

  double cum = run.cum / SAMPLE_RATE;
  if (! run.posFlag)
    cum = -cum;

  double vcum = 0.;
  const unsigned f = firstBuildupSample + buildupLength;
  for (unsigned i = f; i < f + run.len; i++)
    vcum += (i - f) * samples[i];
  vcum /= (SAMPLE_RATE * SAMPLE_RATE);

  timeConstant = 1000. * vcum / cum;
  transientAmpl = cum * cum / vcum;

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


bool SegTransient::detect(
  const vector<double>& samples,
  const vector<Run>& runs)
{
  unsigned rno;
  if (! SegTransient::detectPossibleRun(runs, rno))
    return false;

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

  if (! SegTransient::checkDecline(samples, run))
    return false;

  SegTransient::estimateTransientParams(samples, run);
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

  const unsigned l = buildupLength + transientLength;

  // Synthesize the early quasi-linear bit.
  vector<float> ff(l);
  for (unsigned i = 0; i < buildupLength; i++)
    ff[i] = static_cast<float>(buildupStart + 
      i * (transientAmpl - buildupStart) / buildupLength);

  // Synthesize the quasi-exponential part.
  for (unsigned i = buildupLength; i < l; i++)
    ff[i] = static_cast<float> (transientAmpl * exp(
      - static_cast<double>(i - buildupLength) / SAMPLE_RATE / 
          (timeConstant / 1000.)));

  ofstream fout(tname, std::ios::out | std::ios::binary);

  fout.write(reinterpret_cast<char *>(ff.data()),
    ff.size() * sizeof(float));
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

