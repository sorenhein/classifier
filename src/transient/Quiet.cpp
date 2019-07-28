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
  const unsigned chunkSize,
  vector<QuietStats>& startList) const
{
  const unsigned numInts = interval.len / chunkSize;

  if (! fromBackFlag)
  {
    // Divide up the interval into chunks of equal size, leaving off
    // any partial chunk at the end.
    for (unsigned i = 0; i < numInts; i++)
    {
      startList.emplace_back(QuietStats());
      QuietStats& q = startList.back();
      q.start = i * chunkSize;
      // startList.push_back(i * chunkSize);
    }
  }
  else
  {
    // Backwards from the end.
    for (unsigned i = 0; i < numInts; i++)
    {
      startList.emplace_back(QuietStats());
      QuietStats& q = startList.back();
      q.start = interval.len - (i+1) * chunkSize;
      // startList.push_back(interval.len - (i+1) * chunkSize);
    }
  }
}


void Quiet::makeStats(
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


void Quiet::addQuiet(
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


unsigned Quiet::curate() const
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


void Quiet::setFinetuneRange(
  const vector<float>& samples,
  const bool fromBackFlag,
  const Interval& quietInt,
  vector<unsigned>& fineStarts) const
{
  // This matters very little, as we add samples to the
  // end anyway.

  const unsigned ls = samples.size();
  unsigned first, last;
  if (! fromBackFlag)
  {
    first = quietInt.first;

    if (first + 2 * INT_LENGTH >= ls)
      last = ls;
    else
      last = first + 2 * INT_LENGTH;

    for (unsigned i = first; i < last; i += INT_FINE_LENGTH)
      fineStarts.push_back(i);
  }
  else
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


void Quiet::getFinetuneStatistics(
  const vector<float>& samples,
  vector<unsigned>& fineStarts,
  vector<QuietStats>& fineList,
  float& sdevThreshold) const
{
  float sdevMax = 0., sdevMin = numeric_limits<float>::max();
  for (unsigned i = 0; i < fineStarts.size(); i++)
  {
    Quiet::makeStats(samples, fineStarts[i], 
      INT_FINE_LENGTH, fineList[i]);
    if (fineList[i].sdev > sdevMax)
      sdevMax = fineList[i].sdev;
    if (fineList[i].sdev < sdevMin)
      sdevMin = fineList[i].sdev;
  }
  sdevThreshold = (sdevMin + sdevMax) / 2.f;
}


void Quiet::adjustIntervals(
  const bool fromBackFlag,
  Interval& quietInt,
  const unsigned index)
{
  if (! fromBackFlag)
  {
    // Shrink or extend the last interval, as the case may be.
    // Could shrink to zero!
    quietInt.len = index - quietInt.first;
  }
  else
  {
    // Shrink or extend the last interval, as the case may be.
    quietInt.len = quietInt.first + quietInt.len - index;
    quietInt.first = index;
  }
}



void Quiet::finetune(
  const vector<float>& samples,
  const bool fromBackFlag,
  Interval& quietInt)
{
  // Attempt to find the point of departure from general noise
  // more accurately.

  vector<unsigned> fineStarts;
  fineStarts.clear();
  Quiet::setFinetuneRange(samples, fromBackFlag, quietInt, fineStarts);

  vector<QuietStats> fineList(fineStarts.size());
  float sdevThreshold;
  Quiet::getFinetuneStatistics(samples, fineStarts, 
    fineList, sdevThreshold);

  for (unsigned i = 0; i < fineStarts.size(); i++)
  {
    if (fineList[i].sdev >= sdevThreshold ||
        abs(fineList[i].mean) >= MEAN_SOMEWHAT_QUIET)
    {
      Quiet::adjustIntervals(fromBackFlag, quietInt, fineStarts[i]);
      return;
    }
  }
  cout << "Odd error\n";
  return;
}


void Quiet::adjustOutputIntervals(
  const Interval& avail,
  const bool fromBackFlag)
{
  const unsigned l = avail.first + avail.len;

  if (! fromBackFlag)
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
  else
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


void Quiet::makeSynth()
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


bool Quiet::detect(
  const vector<float>& samples,
  const double sampleRate,
  const Interval& available,
  const bool fromBackFlag,
  Interval& active)
{
  QuietStats qstats;
  qstats.len = static_cast<unsigned>(sampleRate * QUIET_DURATION);
  quiet.clear();

  // Chop up the interval into chunks of size qstats.len, starting
  // either from the front or the back depending on fromBackFlag.
  vector<QuietStats> startList;
  Quiet::makeStarts(available, fromBackFlag, qstats.len, startList);
    
  unsigned runReds = 0, totalReds = 0;

  for (auto& st: startList)
  {
    Quiet::makeStats(samples, st.start, INT_LENGTH, qstats);
    const QuietGrade grade = Quiet::isQuiet(qstats);

    if (grade == GRADE_DEEP_RED)
      break;

    Quiet::addQuiet(st.start, INT_LENGTH, grade, qstats.mean);

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

  const unsigned n = Quiet::curate();
  quiet.resize(n);
  if (n > 0)
    Quiet::finetune(samples, fromBackFlag, quiet.back());

  // Make output a bit longer in order to better see.
  Quiet::adjustOutputIntervals(available, fromBackFlag);

  Quiet::makeSynth();

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

