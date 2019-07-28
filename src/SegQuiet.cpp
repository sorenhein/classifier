#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <limits>
#include <math.h>

#include "SegQuiet.h"

#include "util/io.h"

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


void SegQuiet::makeStarts(
  const Interval& interval,
  const QuietPlace direction,
  vector<unsigned>& startList) const
{
  const unsigned numInts = interval.len / INT_LENGTH;

  if (direction == QUIET_BACK)
  {
    // Backwards from the end.
    for (unsigned i = 0; i < numInts; i++)
      startList.push_back(interval.len - (i+1) * INT_LENGTH);
  }
  else if (direction == QUIET_FRONT)
  {
    for (unsigned i = 0; i < numInts; i++)
      startList.push_back(i * INT_LENGTH);
  }
}


void SegQuiet::makeStats(
  const vector<float>& samples,
  const unsigned first,
  const unsigned len,
  QuietStats& qstats) const
{
  float sum = 0.;
  float sumsq = 0.;
  for (unsigned i = first; i < first+len; i++)
  {
    sum += samples[i];
    sumsq += samples[i] * samples[i];
  }

  qstats.mean = sum / len;
  qstats.sdev = sqrt((len * sumsq - sum * sum) / (len * (len-1.f)));
}


QuietGrade SegQuiet::isQuiet(const QuietStats& qstats) const
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


void SegQuiet::addQuiet(
  const unsigned start, 
  const unsigned len,
  const QuietGrade grade,
  const float mean)
{
  Interval interval;
  interval.first = start;
  interval.len = len;
  interval.grade = grade;
  interval.mean = mean;
  quiet.push_back(interval);
}


unsigned SegQuiet::curate() const
{
  const unsigned l = quiet.size();

  for (unsigned i = 0; i < l; i++)
  {
    // Skip a single red not followed so quickly by another one.
    if (quiet[i].grade == GRADE_RED)
    {
      if (i + NUM_QUIET_FOLLOWERS >= l)
        return i;

      for (unsigned j = i+1; j <= i+NUM_QUIET_FOLLOWERS; j++)
      {
        if (quiet[j].grade == GRADE_RED)
          return i;
      }
    }
  }

  return l;
}


void SegQuiet::setFinetuneRange(
  const vector<float>& samples,
  const QuietPlace direction,
  const Interval& quietInt,
  vector<unsigned>& fineStarts) const
{
  // This matters very little, as we add samples to the
  // end anyway.

  const unsigned ls = samples.size();
  unsigned first, last;
  if (direction == QUIET_FRONT)
  {
    first = quietInt.first;

    if (first + 2 * INT_LENGTH >= ls)
      last = ls;
    else
      last = first + 2 * INT_LENGTH;

    for (unsigned i = first; i < last; i += INT_FINE_LENGTH)
      fineStarts.push_back(i);
  }
  else if (direction == QUIET_BACK)
  {
    if (quietInt.first < INT_LENGTH)
      first = 0;
    else
      first = quietInt.first - INT_LENGTH;

    if (first + 2 * INT_LENGTH >= ls)
      last = ls;
    else
      last = first + 2 * INT_LENGTH;

    for (unsigned i = first; i < last; i += INT_FINE_LENGTH)
      fineStarts.push_back(last - INT_FINE_LENGTH - (i - first));
  }
}


void SegQuiet::getFinetuneStatistics(
  const vector<float>& samples,
  vector<unsigned>& fineStarts,
  vector<QuietStats>& fineList,
  float& sdevThreshold) const
{
  float sdevMax = 0., sdevMin = numeric_limits<float>::max();
  for (unsigned i = 0; i < fineStarts.size(); i++)
  {
    SegQuiet::makeStats(samples, fineStarts[i], 
      INT_FINE_LENGTH, fineList[i]);
    if (fineList[i].sdev > sdevMax)
      sdevMax = fineList[i].sdev;
    if (fineList[i].sdev < sdevMin)
      sdevMin = fineList[i].sdev;
  }
  sdevThreshold = (sdevMin + sdevMax) / 2.f;
}


void SegQuiet::adjustIntervals(
  const QuietPlace direction,
  Interval& quietInt,
  const unsigned index)
{
  if (direction == QUIET_FRONT)
  {
    // Shrink or extend the last interval, as the case may be.
    // Could shrink to zero!
    quietInt.len = index - quietInt.first;
  }
  else if (direction == QUIET_BACK)
  {
    // Shrink or extend the last interval, as the case may be.
    quietInt.len = quietInt.first + quietInt.len - index;
    quietInt.first = index;
  }
}



void SegQuiet::finetune(
  const vector<float>& samples,
  const QuietPlace direction,
  Interval& quietInt)
{
  // Attempt to find the point of departure from general noise
  // more accurately.

  vector<unsigned> fineStarts;
  fineStarts.clear();
  SegQuiet::setFinetuneRange(samples, direction, 
    quietInt, fineStarts);

  vector<QuietStats> fineList(fineStarts.size());
  float sdevThreshold;
  SegQuiet::getFinetuneStatistics(samples, fineStarts, 
    fineList, sdevThreshold);

  for (unsigned i = 0; i < fineStarts.size(); i++)
  {
    if (fineList[i].sdev >= sdevThreshold ||
        abs(fineList[i].mean) >= MEAN_SOMEWHAT_QUIET)
    {
      SegQuiet::adjustIntervals(direction, quietInt, fineStarts[i]);
      return;
    }
  }
  cout << "Odd error\n";
  return;
}


void SegQuiet::adjustOutputIntervals(
  const Interval& avail,
  const QuietPlace direction)
{
  const unsigned l = avail.first + avail.len;

  if (direction == QUIET_FRONT)
  {
    writeInterval.first = avail.first;
    if (quiet.size() == 0)
    {
      writeInterval.len = 0;
      activeInterval.first = avail.first;
      activeInterval.len = avail.len;
      return;
    }

    writeInterval.len = quiet.back().first + quiet.back().len -
      writeInterval.first;

    activeInterval.first = writeInterval.first + writeInterval.len;
    activeInterval.len = l - activeInterval.first;

    if (activeInterval.first + 1000 < l)
      writeInterval.len += 1000;
    else
      writeInterval.len = l - writeInterval.first;

  }
  else if (direction == QUIET_BACK)
  {
    if (quiet.size() == 0)
    {
      writeInterval.first = avail.first;
      writeInterval.len = avail.first;
      activeInterval.first = avail.first;
      activeInterval.len = avail.len;
      return;
    }

    writeInterval.first = quiet.back().first;

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


void SegQuiet::makeSynth()
{
  synth.resize(writeInterval.len);
  for (unsigned i = 0; i < writeInterval.len; i++)
    synth[i] = 0.;

  for (unsigned i = 0; i < quiet.size(); i++)
  {
    float g;
    if (quiet[i].grade == GRADE_GREEN)
      g = OUTPUT_VERY_QUIET;
    else if (quiet[i].grade == GRADE_AMBER)
      g = OUTPUT_SOMEWHAT_QUIET;
    else
      g = 0.;

    for (unsigned j = quiet[i].first; 
        j < quiet[i].first + quiet[i].len; j++)
      synth[j - writeInterval.first] = g;
  }
}


bool SegQuiet::detect(
  const vector<float>& samples,
  const Interval& available,
  const QuietPlace direction,
  Interval& active)
{
  if (direction != QUIET_FRONT && direction != QUIET_BACK)
    return false;

  QuietStats qstats;
  qstats.len = INT_LENGTH;
  quiet.clear();

  vector<unsigned> startList;
  SegQuiet::makeStarts(available, direction, startList);
    
  unsigned runReds = 0, totalReds = 0;

  for (unsigned start: startList)
  {
    SegQuiet::makeStats(samples, start, INT_LENGTH, qstats);
    const QuietGrade grade = SegQuiet::isQuiet(qstats);

    if (grade == GRADE_DEEP_RED)
      break;

    SegQuiet::addQuiet(start, INT_LENGTH, grade, qstats.mean);

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

  const unsigned n = SegQuiet::curate();
  quiet.resize(n);
  if (n > 0)
    SegQuiet::finetune(samples, direction, quiet.back());

  // Make output a bit longer in order to better see.
  SegQuiet::adjustOutputIntervals(available, direction);

  SegQuiet::makeSynth();

  active = activeInterval;
  return true;
}


void SegQuiet::printStats(
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


void SegQuiet::writeFile(const string& filename) const
{
  writeBinary(filename, writeInterval.first, synth);
}

