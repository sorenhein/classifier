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
  unsigned counterAfterRed = NUM_QUIET_FOLLOWERS;
  unsigned red = numeric_limits<unsigned>::max();  // Shouldn't matter
  unsigned i = 0;

  // Stop at the first deep red, or the first red not followed by
  // at least NUM_QUIET_FOLLOWERS of below-red intensity.
  for (auto& quiet: quietList)
  {
    Quiet::makeStats(samples, quiet);
    quiet.setGrade();

    if (quiet.grade == GRADE_DEEP_RED)
      return (counterAfterRed < NUM_QUIET_FOLLOWERS ? red : i);
    else if (quiet.grade != GRADE_RED)
      counterAfterRed++;
    else if (counterAfterRed < NUM_QUIET_FOLLOWERS)
      return red;
    else
    {
      red = i;
      counterAfterRed = 0;
    }
    i++;
  }
  return quietList.size();
}


void Quiet::adjustIntervals(
  const bool fromBackFlag,
  QuietData& qstatsCoarse,
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
  const QuietData& qstatsCoarse,
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
  QuietData& qstats)
{
  // Attempt to find the point of departure from general noise
  // more accurately.

  Interval intervalFine;
  Quiet::setFineInterval(qstats, fromBackFlag, samples.size(), 
    intervalFine);

  // This matters very little, as we add samples to the end anyway.
  list<QuietData> fineStarts;
  Quiet::makeStarts(intervalFine, fromBackFlag, durationFine, fineStarts);

  float sdevThreshold;
  Quiet::getFinetuneStatistics(samples, fineStarts, sdevThreshold);

  // Look for the first fine interval to have above-average activity.
  for (auto& fine: fineStarts)
  {
    if (fine.sdev >= sdevThreshold || ! fine.isVeryQuiet())
    {
      Quiet::adjustIntervals(fromBackFlag, qstats, fine.start);
      return;
    }
  }
  // Should not get here.
}


void Quiet::adjustOutputIntervals(
  const QuietData& quiet,
  const bool fromBackFlag,
  Interval& available)
{
  const unsigned availEnd = available.first + available.len;

  if (! fromBackFlag)
  {
    const unsigned quietEnd = quiet.start + quiet.len;

    writeInterval.first = available.first;
    writeInterval.len = quietEnd - available.first;

    // We also write some samples after the quiet interval for clarity.
    if (quietEnd + padSamples < availEnd)
      writeInterval.len += padSamples;
    else
      writeInterval.len = available.len;

    // We don't go earlier than quietEnd, as we would often
    // get into the real transient.
    available.first = quietEnd;
    available.len = availEnd - quietEnd;
  }
  else
  {
    writeInterval.first = quiet.start;

    // We also write some samples before the quiet interval for clarity.
    if (writeInterval.first >= padSamples)
      writeInterval.first -= padSamples;
    else
      writeInterval.first = 0;

    writeInterval.len = availEnd - writeInterval.first;

    // Here we can go beyond the start of the quiet interval,
    // as nothing much happens here in general.  So we might as well
    // give the filter some quiet data to work with.
    if (quiet.start + padSamples < availEnd)
      available.len = quiet.start + padSamples - available.first;
  }
}


void Quiet::synthesize(const list<QuietData>& quietList)
{
  synth.clear();
  synth.resize(writeInterval.len);

  for (auto& quiet: quietList)
  {
    const float g = quiet.writeLevel();

    for (unsigned j = quiet.start; j < quiet.start + quiet.len; j++)
      synth[j - writeInterval.first] = g;
  }
}


bool Quiet::detect(
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
  quietCoarse.resize(n);

  if (n > 0)
  {
    Quiet::finetune(samples, fromBackFlag, quietCoarse.back());

    // Make output a bit longer in order to better see.
    Quiet::adjustOutputIntervals(quietCoarse.back(), fromBackFlag,
      available);
  }
  else
  {
    writeInterval.first = available.first;
    writeInterval.len = 0;
  }

  // Make a synthetic step signal to visualize the quietness levels.
  Quiet::synthesize(quietCoarse);

  return true;
}


void Quiet::writeFile(const string& filename) const
{
  writeBinary(filename, writeInterval.first, synth);
}

