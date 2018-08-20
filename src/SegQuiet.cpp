#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <math.h>

#include "SegQuiet.h"

#define SAMPLE_RATE 2000.

// TODO Move to times, not sample numbers
#define INT_LENGTH 100
#define QUIET_OUTPUT 3.

#define SDEV_OUTLIERS 2.
#define SDEV_SAMPLE_MEAN 3.
#define SDEV_ABSOLUTE 0.3
#define MEAN_VERY_QUIET 0.03
#define SDEV_VERY_QUIET 0.03
#define OUTLIERS_EXPECTED 5
#define OUTLIER_TOLERANCE 2.
#define RUN_RANDOM_DENSITY 0.3

#define SEPARATOR ";"


SegQuiet::SegQuiet()
{
}


SegQuiet::~SegQuiet()
{
}


void SegQuiet::reset()
{
  quiet.clear();
}


void SegQuiet::makeStats(
  const vector<double>& samples,
  const unsigned first,
  const unsigned len,
  QuietStats& qstats) const
{
  double sum = 0.;
  double sumsq = 0.;
  for (unsigned i = first; i < first+len; i++)
  {
    sum += samples[i];
    sumsq += samples[i] * samples[i];
  }

  qstats.mean = sum / len;
  qstats.sdev = sqrt((len * sumsq - sum * sum) / (len * (len-1.)));

  qstats.lower = qstats.mean - SDEV_OUTLIERS * qstats.sdev;
  qstats.upper = qstats.mean + SDEV_OUTLIERS * qstats.sdev;

  qstats.numOutliers = 0;
  qstats.valOutliers = 0.;
  for (unsigned i = first; i < first+len; i++)
  {
    if (samples[i] < qstats.lower || samples[i] > qstats.upper)
    {
      qstats.numOutliers++;
      if (qstats.sdev > 0.)
        qstats.valOutliers += abs(samples[i] - qstats.mean) /
          qstats.sdev;
    }
  }
}


unsigned SegQuiet::countRuns(
  const vector<Run>& runs,
  const unsigned first,
  const unsigned len) const
{
  unsigned n = 0;
  for (unsigned i = 0; i < runs.size(); i++)
  {
    if (runs[i].first < first)
      continue;

    if (runs[i].first < first+len)
      n++;
    else
      return n;
  }
  return n;
}


bool SegQuiet::isQuiet(const QuietStats& qstats) const
{
  // Current conditions for a quiet interval:
  // 0. Very low absolute values.
  // 1. The true mean should be roughly within the sample mean
  //    +/- 3 standard deviations of the mean.  The sdev of
  //    the mean is the sample sdev / sqrt(n).
  // 2. The sample sdev should be less than an absolute
  //    threshold.
  // 3. There are minus points for outliers beyond the
  //    expectation which is about 5 for 2 sdevs.  The cost
  //    is the sum of the number of sdevs, minus 5 * 2 as
  //    a baseline.
  // 4. There's an empirically expected number of runs.
  //    If there are too few, it tends to indicate non-random
  //    temporal correlation.

  if (qstats.mean < MEAN_VERY_QUIET &&
      qstats.sdev < SDEV_VERY_QUIET)
    return true;

  const double meanSdev = qstats.sdev / sqrt(qstats.len);
  const double absMean = abs(qstats.mean);
  if (qstats.sdev > 0. && 
      absMean / meanSdev > SDEV_SAMPLE_MEAN)
    return false;
  
  if (qstats.sdev > SDEV_ABSOLUTE)
    return false;
  
  if (qstats.valOutliers / 
      (OUTLIERS_EXPECTED * SDEV_OUTLIERS) >= OUTLIER_TOLERANCE)
    return false;
  
  if (qstats.numRuns < RUN_RANDOM_DENSITY * qstats.len)
    return false;

  return true;
}


void SegQuiet::makeSynth(const unsigned l)
{
  synth.resize(l - offset);
  for (unsigned i = offset; i < l; i++)
    synth[i - offset] = 0.;

  for (unsigned i = 0; i < quiet.size(); i++)
  {
    for (unsigned j = quiet[i].first; 
        j < quiet[i].first + quiet[i].len; j++)
      synth[j - offset] = QUIET_OUTPUT;
  }
}


void SegQuiet::makeActive(
  vector<Interval>& active,
  const unsigned firstSampleNo,
  const unsigned l) const
{
  active.clear();
  Interval aint;
  aint.first = firstSampleNo;
  aint.len = offset - firstSampleNo;

  for (unsigned i = offset; i < l; i++)
  {
    if (synth[i - offset] == 0.)
    {
      if (aint.len == 0)
        aint.first = i;

      aint.len++;
    }
    else if (aint.len > 0)
    {
      active.push_back(aint);
      aint.len = 0;
    }
  }

  if (aint.len > 0)
    active.push_back(aint);
}


bool SegQuiet::detect(
  const vector<double>& samples,
  const vector<Run>& runs,
  const unsigned firstSampleNo,
  vector<Interval>& active)
{
  const unsigned l = samples.size();
  const unsigned numInts = (l - firstSampleNo) / INT_LENGTH; 
  offset = l - numInts * INT_LENGTH;

  QuietStats qstats;
  qstats.len = INT_LENGTH;
  quiet.clear();
  for (unsigned i = offset; i < l; i += INT_LENGTH)
  {
    SegQuiet::makeStats(samples, i, INT_LENGTH, qstats);
    qstats.numRuns = SegQuiet::countRuns(runs, i, INT_LENGTH);

bool flag = false;
    if (SegQuiet::isQuiet(qstats))
    {
flag = true;
      Interval interval;
      interval.first = i;
      interval.len = INT_LENGTH;
      quiet.push_back(interval);
    }

    if (i >= l-1000)
      // SegQuiet::printStats(qstats, i);
      SegQuiet::printShortStats(qstats, i, flag);
  }

  SegQuiet::makeSynth(l);

  SegQuiet::makeActive(active, firstSampleNo, l);

  return (active.size() > 0);
}


void SegQuiet::writeBinary(const string& origname) const
{
  // Make the transient file name by:
  // * Replacing /raw/ with /quiet/
  // * Adding _offset_0 before .dat

  string tname = origname;
  auto tp1 = tname.find("/raw/");
  if (tp1 == string::npos)
    return;

  auto tp2 = tname.find(".dat");
  if (tp2 == string::npos)
    return;

  tname.insert(tp2, "_offset_" + to_string(offset));
  tname.replace(tp1, 5, "/quiet/");

  ofstream fout(tname, std::ios::out | std::ios::binary);

  fout.write(reinterpret_cast<const char *>(synth.data()),
    synth.size() * sizeof(float));
  fout.close();
}


void SegQuiet::printStats(
  const QuietStats& qstats,
  const unsigned first) const
{
  cout << 
    first << SEPARATOR <<
    qstats.mean << SEPARATOR <<
    qstats.sdev << SEPARATOR <<
    qstats.len << SEPARATOR <<
    qstats.numOutliers << SEPARATOR <<
    qstats.valOutliers << SEPARATOR <<
    qstats.numRuns << "\n";
}


void SegQuiet::printShortStats(
  const QuietStats& qstats,
  const unsigned first,
  const bool flag) const
{
  cout << 
    first << SEPARATOR <<
    qstats.mean << SEPARATOR <<
    qstats.sdev << SEPARATOR <<
    flag << "\n";
}


string SegQuiet::headerCSV() const
{
  stringstream ss;

  return ss.str();
}


string SegQuiet::strCSV() const
{
  stringstream ss;
  
  return ss.str();
}

