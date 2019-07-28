#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <limits>
#include <math.h>

#include "Quiet.h"

#include "../const.h"

#include "../util/io.h"

#define SAMPLE_RATE 2000.

// TODO Move to times, not sample numbers
#define INT_LENGTH 100
#define INT_FINE_LENGTH 10

#define OUTPUT_VERY_QUIET 3.f
#define OUTPUT_SOMEWHAT_QUIET 1.5f

// Model parameters.
#define MEAN_VERY_QUIET 0.2
#define SDEV_VERY_QUIET 0.06

#define MEAN_SOMEWHAT_QUIET 0.2
#define SDEV_SOMEWHAT_QUIET 0.2

#define MEAN_QUIET_LIMIT 0.3
#define SDEV_QUIET_LIMIT 0.3

#define NUM_NON_QUIET_RUNS 2
#define NUM_QUIET_FOLLOWERS 4

#define SEPARATOR ";"


Quiet::Quiet()
{
}


Quiet::~Quiet()
{
}


void Quiet::makeStarts(
  const Interval& interval,
  const bool fromBackFlag,
  const unsigned duration,
  list<QuietStats>& quietList) const
{
  const unsigned numInts = interval.len / duration;

  if (! fromBackFlag)
  {
    // Divide up the interval into chunks of equal size, leaving off
    // any partial chunk at the end.
    for (unsigned i = 0; i < numInts; i++)
    {
      quietList.emplace_back(QuietStats());
      QuietStats& q = quietList.back();
      q.start = interval.first + i * duration;
      q.len = duration;
      q.grade = GRADE_SIZE;
    }
  }
  else
  {
    // Backwards from the end.
    for (unsigned i = 0; i < numInts; i++)
    {
      quietList.emplace_back(QuietStats());
      QuietStats& q = quietList.back();
      q.start = interval.first + interval.len - (i+1) * duration;
      q.len = duration;
      q.grade = GRADE_SIZE;
    }
  }
}


void Quiet::makeStats(
  const vector<float>& samples,
  QuietStats& qentry) const
{
  float sum = 0.;
  float sumsq = 0.;
  for (unsigned i = qentry.start; i < qentry.start + qentry.len; i++)
  {
    sum += samples[i];
    sumsq += samples[i] * samples[i];
  }

  qentry.mean = sum / qentry.len;
  qentry.sdev = sqrt((qentry.len * sumsq - sum * sum) / 
    (qentry.len * (qentry.len-1.f)));
}


QuietGrade Quiet::isQuiet(const QuietStats& qstats) const
{
  if (abs(qstats.mean) < MEAN_VERY_QUIET &&
      qstats.sdev < SDEV_VERY_QUIET)
    return GRADE_GREEN;
  else if (abs(qstats.mean) < MEAN_SOMEWHAT_QUIET &&
      qstats.sdev < SDEV_SOMEWHAT_QUIET)
    return GRADE_AMBER;
  else if (abs(qstats.mean) < MEAN_QUIET_LIMIT &&
      qstats.sdev < SDEV_SOMEWHAT_QUIET)
    return GRADE_RED;
  else if (abs(qstats.mean) < MEAN_SOMEWHAT_QUIET &&
      qstats.sdev < SDEV_QUIET_LIMIT)
    return GRADE_RED;
  else
    return GRADE_DEEP_RED;
}


unsigned Quiet::curate(const list<QuietStats>& quietList) const
{
  const unsigned l = quietList.size();

  unsigned i = 0;
  for (auto qit = quietList.begin(); 
      qit != quietList.end(); qit++, i++)
  {
    if (qit->grade == GRADE_DEEP_RED || qit->grade == GRADE_SIZE)
      return i;

    // Skip a single red not followed so quickly by another one.
    if (qit->grade == GRADE_RED)
    {
      if (i + NUM_QUIET_FOLLOWERS >= l)
        return i;

      // TODO Check off-by-one?
      auto nqit = next(qit);
      for (unsigned j = 0; j < NUM_QUIET_FOLLOWERS; j++)
      {
        if (nqit->grade == GRADE_RED ||
            nqit->grade == GRADE_DEEP_RED ||
            nqit->grade == GRADE_SIZE)
          return i;
        else
          nqit = next(nqit);
      }
    }
  }

  return l;
}


void Quiet::getFinetuneStatistics(
  const vector<float>& samples,
  list<QuietStats>& fineStarts,
  float& sdevThreshold) const
{
  float sdevMax = 0., sdevMin = numeric_limits<float>::max();
  for (auto& fine: fineStarts)
  {
    Quiet::makeStats(samples, fine);
    if (fine.sdev > sdevMax)
      sdevMax = fine.sdev;
    if (fine.sdev < sdevMin)
      sdevMin = fine.sdev;
  }
  sdevThreshold = (sdevMin + sdevMax) / 2.f;
}


void Quiet::adjustIntervals(
  const bool fromBackFlag,
  QuietStats& qstatsCoarse,
  const unsigned index)
{
  if (! fromBackFlag)
  {
    // Shrink or extend the last interval, as the case may be.
    // Could shrink to zero!
    qstatsCoarse.len = index - qstatsCoarse.start;
  }
  else
  {
    // Shrink or extend the last interval, as the case may be.
    qstatsCoarse.len = qstatsCoarse.start + qstatsCoarse.len - index;
    qstatsCoarse.start = index;
  }
}


void Quiet::setFineInterval(
  const QuietStats& qstatsCoarse,
  const bool fromBackFlag,
  const unsigned sampleSize,
  Interval& intervalFine) const
{
  if (! fromBackFlag)
    intervalFine.first = qstatsCoarse.start;
  else if (qstatsCoarse.start < durationCoarse)
    intervalFine.first = 0;
  else
    intervalFine.first = qstatsCoarse.start - durationCoarse;

  unsigned last = intervalFine.first + 2 * durationCoarse;
  if (last >= sampleSize)
    last = sampleSize;

  intervalFine.len = last - intervalFine.first;
}


void Quiet::finetune(
  const vector<float>& samples,
  const bool fromBackFlag,
  QuietStats& qstats)
{
  // Attempt to find the point of departure from general noise
  // more accurately.

  Interval intervalFine;
  Quiet::setFineInterval(qstats, fromBackFlag, samples.size(), 
    intervalFine);

  // This matters very little, as we add samples to the end anyway.
  list<QuietStats> fineStarts;
  Quiet::makeStarts(intervalFine, fromBackFlag, 
    durationFine, fineStarts);

  float sdevThreshold;
  Quiet::getFinetuneStatistics(samples, fineStarts, sdevThreshold);

  for (auto& fine: fineStarts)
  {
    if (fine.sdev >= sdevThreshold ||
        abs(fine.mean) >= MEAN_SOMEWHAT_QUIET)
    {
      Quiet::adjustIntervals(fromBackFlag, qstats, fine.start);
      return;
    }
  }
  cout << "Odd error\n";
  return;
}


void Quiet::adjustOutputIntervals(
  const list<QuietStats>& quietList,
  const Interval& avail,
  const bool fromBackFlag)
{
  const unsigned l = avail.first + avail.len;

  if (! fromBackFlag)
  {
    writeInterval.first = avail.first;
    if (quietList.empty())
    {
      writeInterval.len = 0;
      activeInterval.first = avail.first;
      activeInterval.len = avail.len;
      return;
    }

    writeInterval.len = quietList.back().start + quietList.back().len -
      writeInterval.first;

    activeInterval.first = writeInterval.first + writeInterval.len;
    activeInterval.len = l - activeInterval.first;

    if (activeInterval.first + 1000 < l)
      writeInterval.len += 1000;
    else
      writeInterval.len = l - writeInterval.first;
  }
  else
  {
    if (quietList.empty())
    {
      writeInterval.first = avail.first;
      writeInterval.len = avail.first;
      activeInterval.first = avail.first;
      activeInterval.len = avail.len;
      return;
    }

    writeInterval.first = quietList.back().start;

    activeInterval.first = avail.first;
    activeInterval.len = writeInterval.first - avail.first;

    // TODO 500 is enough here.  Should depend on filter.
    if (activeInterval.first + activeInterval.len + 1000 < l)
      activeInterval.len += 1000;
    else
      activeInterval.len = avail.len;

    if (writeInterval.first >= 1000)
      writeInterval.first -= 1000;
    else
      writeInterval.first = 0;
    writeInterval.len = l - writeInterval.first;
  }
}


void Quiet::makeSynth(const list<QuietStats>& quietList)
{
  synth.resize(writeInterval.len);
  for (unsigned i = 0; i < writeInterval.len; i++)
    synth[i] = 0.;

  for (auto& quiet: quietList)
  {
    float g;
    if (quiet.grade == GRADE_GREEN)
      g = OUTPUT_VERY_QUIET;
    else if (quiet.grade == GRADE_AMBER)
      g = OUTPUT_SOMEWHAT_QUIET;
    else
      g = 0.;

    for (unsigned j = quiet.start; j < quiet.start + quiet.len; j++)
      synth[j - writeInterval.first] = g;
  }
}


bool Quiet::detect(
  const vector<float>& samples,
  const double sampleRate,
  const Interval& available,
  const bool fromBackFlag,
  Interval& active)
{
  durationCoarse = 
    static_cast<unsigned>(sampleRate * QUIET_DURATION_COARSE);
  durationFine = 
    static_cast<unsigned>(sampleRate * QUIET_DURATION_FINE);

  // Chop up the interval into chunks of size qstats.len, starting
  // either from the front or the back depending on fromBackFlag.
  Quiet::makeStarts(available, fromBackFlag, durationCoarse, quietCoarse);
    
  unsigned runReds = 0, totalReds = 0;

  for (auto& st: quietCoarse)
  {
    Quiet::makeStats(samples, st);
    const QuietGrade grade = Quiet::isQuiet(st);
    st.grade = grade;

    if (grade == GRADE_DEEP_RED)
      break;

    if (grade == GRADE_RED)
    {
      runReds++;
      totalReds++;
    }
    else
      runReds = 0;

    if (runReds == NUM_NON_QUIET_RUNS)
      break;
  }

  const unsigned n = Quiet::curate(quietCoarse);

  quietCoarse.resize(n);

  if (n > 0)
    Quiet::finetune(samples, fromBackFlag, quietCoarse.back());

  // Make output a bit longer in order to better see.
  Quiet::adjustOutputIntervals(quietCoarse, available, fromBackFlag);

  Quiet::makeSynth(quietCoarse);

  active = activeInterval;
  return true;
}


void Quiet::printStats(
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


void Quiet::writeFile(const string& filename) const
{
  writeBinary(filename, writeInterval.first, synth);
}

