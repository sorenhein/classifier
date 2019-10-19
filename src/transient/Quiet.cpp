#include <limits>

#include "Quiet.h"

#include "../const.h"

#include "../util/io.h"


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
  list<QuietData>& quietList) const
{
  const unsigned numInts = interval.len / duration;
  quietList.clear();

  if (! fromBackFlag)
  {
    // Divide up the interval into chunks of equal size, leaving off
    // any partial chunk at the end.
    for (unsigned i = 0; i < numInts; i++)
    {
      quietList.emplace_back(QuietData());
      QuietData& q = quietList.back();
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
      quietList.emplace_back(QuietData());
      QuietData& q = quietList.back();
      q.start = interval.first + interval.len - (i+1) * duration;
      q.len = duration;
      q.grade = GRADE_SIZE;
    }
  }
}


void Quiet::makeStats(
  const vector<float>& samples,
  QuietData& qentry) const
{
  float sum = 0.;
  float sumsq = 0.;
  for (unsigned i = qentry.start; i < qentry.start + qentry.len; i++)
  {
    sum += samples[i];
    sumsq += samples[i] * samples[i];
  }

  qentry.mean = sum / qentry.len;
  qentry.sdev = static_cast<float>(sqrt((qentry.len * sumsq - sum * sum) / 
    (qentry.len * (qentry.len-1.f))));
}


unsigned Quiet::annotateList(
  const vector<float>& samples,
  list<QuietData>& quietList) const
{
  unsigned counterAfterAmber = NUM_QUIET_FOLLOWERS;
  unsigned amber = numeric_limits<unsigned>::max();  // Shouldn't matter
  unsigned i = 0;

  // Stop at the first red, or the first amber not followed by
  // at least NUM_QUIET_FOLLOWERS of green intensity.
  for (auto& quiet: quietList)
  {
    Quiet::makeStats(samples, quiet);
    quiet.setGrade(MEAN_SOMEWHAT_QUIET, MEAN_QUIET_LIMIT,
      SDEV_MEAN_FACTOR);

    if (quiet.grade == GRADE_RED)
      return (counterAfterAmber < NUM_QUIET_FOLLOWERS ? amber : i);
    else if (quiet.grade == GRADE_GREEN)
      counterAfterAmber++;
    else if (counterAfterAmber < NUM_QUIET_FOLLOWERS)
      return amber;
    else
    {
      amber = i;
      counterAfterAmber = 0;
    }
    i++;
  }
  return quietList.size();
}


void Quiet::adjustIntervals(
  const bool fromBackFlag,
  Interval& qintCoarse,
  const unsigned index)
{
  if (! fromBackFlag)
  {
    // Shrink or extend the last interval, as the case may be.
    // Could shrink to zero!
    qintCoarse.len = index - qintCoarse.first;
  }
  else
  {
    // Shrink or extend the last interval, as the case may be.
    qintCoarse.len = qintCoarse.first + qintCoarse.len - index;
    qintCoarse.first = index;
  }
}


void Quiet::setFineInterval(
  const Interval& qintCoarse,
  const bool fromBackFlag,
  const unsigned sampleSize,
  Interval& intervalFine) const
{
  if (! fromBackFlag)
    intervalFine.first = qintCoarse.first;
  else if (qintCoarse.first < durationCoarse)
    intervalFine.first = 0;
  else
    intervalFine.first = qintCoarse.first - durationCoarse;

  unsigned last = intervalFine.first + 2 * durationCoarse;
  if (last >= sampleSize)
    last = sampleSize;

  intervalFine.len = last - intervalFine.first;
}


void Quiet::getFinetuneStatistics(
  const vector<float>& samples,
  list<QuietData>& fineStarts,
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


void Quiet::finetune(
  const vector<float>& samples,
  const bool fromBackFlag,
  Interval& qint)
{
  // Attempt to find the point of departure from general noise
  // more accurately.

  Interval intervalFine;
  Quiet::setFineInterval(qint, fromBackFlag, samples.size(), 
    intervalFine);

  // This matters very little, as we add samples to the end anyway.
  list<QuietData> fineStarts;
  Quiet::makeStarts(intervalFine, fromBackFlag, durationFine, fineStarts);

  float sdevThreshold;
  Quiet::getFinetuneStatistics(samples, fineStarts, sdevThreshold);

  // Look for the first fine interval to have above-average activity.
  for (auto& fine: fineStarts)
  {
    if (fine.sdev >= sdevThreshold || ! fine.meanIsQuiet())
    {
      Quiet::adjustIntervals(fromBackFlag, qint, fine.start);
      return;
    }
  }
  // Should not get here.
}


void Quiet::adjustOutputIntervals(
  const Interval& qint,
  const bool fromBackFlag,
  Interval& available)
{
  const unsigned availEnd = available.first + available.len;

  if (! fromBackFlag)
  {
    const unsigned quietEnd = qint.first + qint.len;

    // We don't go earlier than quietEnd, as we would often
    // get into the real transient.
    available.first = quietEnd;
    available.len = availEnd - quietEnd;
  }
  else
  {
    // Here we can go beyond the start of the quiet interval,
    // as nothing much happens here in general.  So we might as well
    // give the filter some quiet data to work with.
    if (qint.first + padSamples < availEnd)
      available.len = qint.first + padSamples - available.first;
  }
}


void Quiet::synthesize(
  const Interval& available,
  const list<Interval>& actives)
{
  synth.clear();
  synth.resize(available.len);

  for (auto& active: actives)
  {
    for (unsigned j = active.first; j < active.first + active.len; j++)
      synth[j - available.first] = ACTIVE_WRITE_LEVEL;
  }
}


void Quiet::detect(
  const vector<float>& samples,
  const float sampleRate,
  const bool fromBackFlag,
  Interval& available)
{
  durationCoarse = 
    static_cast<unsigned>(sampleRate * QUIET_DURATION_COARSE);
  durationFine = 
    static_cast<unsigned>(sampleRate * QUIET_DURATION_FINE);
  padSamples = 
    static_cast<unsigned>(sampleRate * QUIET_DURATION_PADDING);

  // Chop up the interval into chunks of size qstats.len, starting
  // either from the front or the back depending on fromBackFlag.
  Quiet::makeStarts(available, fromBackFlag, durationCoarse, quietCoarse);

  // Fill out statistics of each chunk, ending when we reach a
  // chunk that is clearly not quiet, or when we have a couple of
  // dubious chunks in succession.

  const unsigned n = Quiet::annotateList(samples, quietCoarse);
  if (n == 0)
    return;

  quietCoarse.resize(n);

  Interval qint;
  qint.first = quietCoarse.back().start;
  qint.len = quietCoarse.back().len;

  Quiet::finetune(samples, fromBackFlag, qint);

  // Make output a bit longer in order to better see.
  Quiet::adjustOutputIntervals(qint, fromBackFlag, available);
}


#include <iostream>
void Quiet::detectIntervals(
  const vector<float>& samples,
  const Interval& available,
  list<Interval>& actives)
{
  // Could probably use a stored version from detect() -- no matter.
  quietCoarse.clear();
  Quiet::makeStarts(available, false, durationCoarse, quietCoarse);
  if (quietCoarse.empty())
    return;

  for (auto& quiet: quietCoarse)
  {
    Quiet::makeStats(samples, quiet);
    // TODO Other thresholds
    quiet.setGrade(MEAN_SOMEWHAT_QUIET, MEAN_QUIET_LIMIT,
      SDEV_MEAN_FACTOR);
  }

  actives.clear();
  unsigned start = available.first;
  auto qit = quietCoarse.begin();
  while (true)
  {
    while (qit != quietCoarse.end() && qit->grade != GRADE_GREEN)
      qit++;

    // TODO Lot of copying -- avoid?
    const QuietData& qd = (qit == quietCoarse.end() ?
      quietCoarse.back() : * qit);

    Interval qint;
    qint.first = qd.start;
    qint.len = qd.len;

    Quiet::finetune(samples, true, qint);

    actives.emplace_back(Interval());
    Interval& interval = actives.back();
    interval.first = start;
    interval.len = qint.first - interval.first;

    // Look back.
    Quiet::adjustOutputIntervals(qint, true, interval);

cout << "active " << interval.first << " to " <<
  interval.first + interval.len << endl;

    if (qit == quietCoarse.end())
      break;

    // Found a green.  Look forward.
    do
    {
      qit++;
    }
    while (qit != quietCoarse.end() && qit->grade == GRADE_GREEN);

    if (qit == quietCoarse.end())
      break;
  
    // Find the onset of the next active interval.
    qint.first = qit->start;
    qint.len = qit->len;

    Quiet::finetune(samples, false, qint);
    start = qint.first + qint.len;
  }

  // Make a synthetic step signal to visualize the active levels.
  Quiet::synthesize(available, actives);
}


void Quiet::writeFile(const string& filename) const
{
  writeBinary(filename, writeInterval.first, synth);
}

