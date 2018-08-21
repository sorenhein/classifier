#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <math.h>

#include "SegQuiet.h"

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
  else
  {
    for (unsigned i = 0; i < numInts; i++)
      startList.push_back(i * INT_LENGTH);
  }
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
}


QuietGrade SegQuiet::isQuiet(const QuietStats& qstats) const
{
  if (qstats.mean < MEAN_VERY_QUIET &&
      qstats.sdev < SDEV_VERY_QUIET)
    return GRADE_GREEN;
  else if (qstats.mean < MEAN_SOMEWHAT_QUIET &&
      qstats.sdev < SDEV_SOMEWHAT_QUIET)
    return GRADE_AMBER;
  else
    return GRADE_RED;
}


void SegQuiet::addQuiet(
  const unsigned start, 
  const unsigned len,
  const QuietGrade grade)
{
  Interval interval;
  interval.first = start;
  interval.len = len;
  interval.grade = grade;
  quiet.push_back(interval);
}


unsigned SegQuiet::curate(
  const unsigned runReds,
  const unsigned totalReds) const
{
  const unsigned l = quiet.size();

  // Not even a few reds in a row?
  if (runReds < NUM_NON_QUIET_RUNS)
    return l;

  // No spurious reds on the way?
  if (totalReds == NUM_NON_QUIET_RUNS)
    return l - NUM_NON_QUIET_RUNS;

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
  const vector<double>& samples,
  const QuietPlace direction,
  vector<unsigned>& fineStarts) const
{
  const unsigned lq = quiet.size();
  if (lq == 0)
    return;

  const unsigned ls = samples.size();
  unsigned first, last;
  if (direction == QUIET_FRONT)
  {
    first = quiet[lq-1].first;

    if (first + 2 * INT_LENGTH >= ls)
      last = ls;
    else
      last = first + 2 * INT_LENGTH;

    for (unsigned i = first; i < last; i += INT_FINE_LENGTH)
      fineStarts.push_back(i);
  }
  else if (direction == QUIET_BACK)
  {
    if (quiet[lq-1].first < INT_LENGTH)
      first = 0;
    else
      first = quiet[lq-1].first - INT_LENGTH;

    if (first + 2 * INT_LENGTH >= ls)
      last = ls;
    else
      last = first + 2 * INT_LENGTH;

    for (unsigned i = first; i < last; i += INT_FINE_LENGTH)
      fineStarts.push_back(last - INT_FINE_LENGTH - (i - first));
  }
}


void SegQuiet::getFinetuneStatistics(
  const vector<double>& samples,
  vector<unsigned>& fineStarts,
  vector<QuietStats>& fineList,
  double& sdevThreshold) const
{
  double sdevMax = 0., sdevMin = numeric_limits<double>::max();
  for (unsigned i = 0; i < fineStarts.size(); i++)
  {
    SegQuiet::makeStats(samples, fineStarts[i], 
      INT_FINE_LENGTH, fineList[i]);
    if (fineList[i].sdev > sdevMax)
      sdevMax = fineList[i].sdev;
    if (fineList[i].sdev < sdevMin)
      sdevMin = fineList[i].sdev;
  }
  sdevThreshold = (sdevMin + sdevMax) / 2.;
}


void SegQuiet::adjustIntervals(
  const QuietPlace direction,
  const unsigned index)
{
  const unsigned lq = quiet.size();
  Interval * qlast  = &quiet.back();

  if (direction == QUIET_FRONT)
  {
    if (index <= qlast->first)
    {
      // Cut the last interval, shrink the previous one.
      quiet.pop_back();
      if (quiet.size() == 0)
        return;

      qlast = &quiet.back();
    }

    // Shrink or extend the last interval, as the case may be.
    qlast->len = index - qlast->first;
  }
  else if (direction == QUIET_BACK)
  {
    if (index >= qlast->first + qlast->len)
    {
      // Cut the "last" (temporally first) interval.
      quiet.pop_back();
      if (quiet.size() == 0)
        return;

      qlast = &quiet.back();
    }

    // Shrink or extend the last interval, as the case may be.
    qlast->len = qlast->first + qlast->len - index;
    qlast->first = index;
  }
}



void SegQuiet::finetune(
  const vector<double>& samples,
  const QuietPlace direction)
{
  // Attempt to find the point of departure from general noise
  // more accurately.

  vector<unsigned> fineStarts;
  fineStarts.clear();
  SegQuiet::setFinetuneRange(samples, direction, fineStarts);
  if (fineStarts.size() == 0)
    return;

  vector<QuietStats> fineList(fineStarts.size());
  double sdevThreshold;
  SegQuiet::getFinetuneStatistics(samples, fineStarts, 
    fineList, sdevThreshold);

  if (direction == QUIET_FRONT || direction == QUIET_BACK)
  {
    for (unsigned i = 0; i < fineStarts.size(); i++)
    {
      if (fineList[i].sdev >= sdevThreshold ||
          abs(fineList[i].mean) >= MEAN_SOMEWHAT_QUIET)
      {
        SegQuiet::adjustIntervals(direction, fineStarts[i]);
        return;
      }
    }
    cout << "Odd error\n";
    return;
  }
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


void SegQuiet::makeActive(vector<Interval>& active) const
{
  active.clear();
  Interval aint;
  aint.first = activeInterval.first;
  aint.len = 0;

  for (unsigned i = activeInterval.first;
      i < activeInterval.first + activeInterval.len; i++)
  {
    double val = 0.;
    if (i >= writeInterval.first &&
        i < writeInterval.first + writeInterval.len)
      val = synth[i - writeInterval.first];

    if (val == 0.)
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
  const vector<Interval>& available,
  const QuietPlace direction,
  vector<Interval>& active)
{
  const unsigned la = available.size();
  QuietStats qstats;
  qstats.len = INT_LENGTH;

  if (direction == QUIET_FRONT || direction == QUIET_BACK)
  {
    if (la != 1)
    {
      cout << "Need exactly one interval for front or back.\n";
      return false;
    }

    quiet.clear();

    vector<unsigned> startList;
    SegQuiet::makeStarts(available[0], direction, startList);
    
    unsigned runReds = 0, totalReds = 0;

    for (unsigned start: startList)
    {
      SegQuiet::makeStats(samples, start, INT_LENGTH, qstats);
      const QuietGrade grade = SegQuiet::isQuiet(qstats);
      SegQuiet::addQuiet(start, INT_LENGTH, grade);

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

    const unsigned n = SegQuiet::curate(runReds, totalReds);
    quiet.resize(n);
    SegQuiet::finetune(samples, direction);

    // Make output a bit longer in order to better see.
    SegQuiet::adjustOutputIntervals(available[0], direction);
  }
  else if (direction == QUIET_INTRA)
  {
    quiet.clear();

    for (unsigned availNo = 0; availNo < la; availNo++)
    {
      vector<unsigned> startList;
      SegQuiet::makeStarts(available[availNo], direction, startList);

      for (unsigned start: startList)
      {
        SegQuiet::makeStats(samples, start, INT_LENGTH, qstats);
        const QuietGrade grade = SegQuiet::isQuiet(qstats);
        SegQuiet::addQuiet(start, INT_LENGTH, grade);
      }
    }

    writeInterval.first = available.front().first;
    writeInterval.len = available.back().first + 
      available.back().len - available.front().first;

    // TODO curate, finetune
  }

  SegQuiet::makeSynth();

  SegQuiet::makeActive(active);

  return (active.size() > 0);
}


void SegQuiet::writeBinary(
  const string& origname,
  const string& dirname) const
{
  // Make the transient file name by:
  // * Replacing /raw/ with /dirname/
  // * Adding _offset_N before .dat

  if (synth.size() == 0)
    return;

  string tname = origname;
  auto tp1 = tname.find("/raw/");
  if (tp1 == string::npos)
    return;

  auto tp2 = tname.find(".dat");
  if (tp2 == string::npos)
    return;

  tname.insert(tp2, "_offset_" + to_string(writeInterval.first));
  tname.replace(tp1, 5, "/" + dirname + "/");

  ofstream fout(tname, std::ios::out | std::ios::binary);

  fout.write(reinterpret_cast<const char *>(synth.data()),
    synth.size() * sizeof(float));
  fout.close();
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

