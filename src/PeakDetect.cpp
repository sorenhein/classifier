#include <list>
#include <iostream>
#include <iomanip>
#include <functional>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakDetect.h"
#include "PeakStats.h"
#include "Except.h"


#define SAMPLE_RATE 2000.

#define KINK_RATIO 100.f

#define TIME_PROXIMITY 0.03 // s

#define SCORE_CUTOFF 0.75

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))



PeakDetect::PeakDetect()
{
  PeakDetect::reset();
}


PeakDetect::~PeakDetect()
{
}


void PeakDetect::reset()
{
  len = 0;
  peaks.clear();
  quietCandidates.clear();
}


float PeakDetect::integrate(
  const vector<float>& samples,
  const unsigned i0,
  const unsigned i1) const
{
  float sum = 0.f;
  for (unsigned i = i0; i < i1; i++)
    sum += samples[i];

  return sum;
}


void PeakDetect::annotate()
{
  if (peaks.size() < 2)
    THROW(ERR_NO_PEAKS, "Too few peaks: " + to_string(peaks.size()));

  const auto peakFirst = peaks.begin();
  const auto peakLast = prev(peaks.end());

  for (auto it = peakFirst; it != peaks.end(); it++)
  {
    const Peak * peakPrev = (it == peakFirst ? nullptr : &*prev(it));
    const Peak * peakNext = (it == peakLast ? nullptr : &*next(it));

    it->annotate(peakPrev, peakNext);
  }
}


bool PeakDetect::check(const vector<float>& samples) const
{
  if (samples.size() == 0)
    THROW(ERR_SHORT_ACCEL_TRACE, "Accel trace length: " + to_string(len));

  if (peaks.size() < 2)
    THROW(ERR_NO_PEAKS, "Too few peaks: " + to_string(peaks.size()));

  const auto peakFirst = peaks.begin();
  const auto peakLast = prev(peaks.end());
  bool flag = true;

  for (auto it = peakFirst; it != peaks.end(); it++)
  {
    Peak peakSynth;
    Peak const * peakPrev = nullptr, * peakNext = nullptr;
    float areaFull = 0.f;
    const unsigned index = it->getIndex();

    if (it != peakFirst)
    {
      peakPrev = &*prev(it);
      areaFull = peakPrev->getAreaCum() + 
        PeakDetect::integrate(samples, peakPrev->getIndex(), index);
    }

    if (it != peakLast)
      peakNext = &*next(it);

    peakSynth.log(index, it->getValue(), areaFull, it->getMaxFlag());
    peakSynth.annotate(peakPrev, peakNext);

    if (! it->check(peakSynth, offset))
      flag = false;
  }
  return flag;
}


void PeakDetect::logFirst(const vector<float>& samples)
{
  peaks.emplace_back(Peak());

  // Find the initial peak polarity.
  bool maxFlag = false;
  for (unsigned i = 1; i < len; i++)
  {
    if (samples[i] > samples[0])
    {
      maxFlag = false;
      break;
    }
    else if (samples[i] < samples[0])
    {
      maxFlag = true;
      break;
    }
  }
  peaks.back().logSentinel(samples[0], maxFlag);
}


void PeakDetect::logLast(const vector<float>& samples)
{
  const auto& peakPrev = peaks.back();
  const float areaFull = 
    PeakDetect::integrate(samples, peakPrev.getIndex(), len-1);
  const float areaCumPrev = peakPrev.getAreaCum();

  peaks.emplace_back(Peak());
  peaks.back().log(
    samples.size()-1, 
    samples[samples.size()-1], 
    areaCumPrev + areaFull, 
    ! peakPrev.getMaxFlag());
}


void PeakDetect::log(
  const vector<float>& samples,
  const unsigned offsetSamples)
{
  len = samples.size();
  if (len < 2)
    THROW(ERR_SHORT_ACCEL_TRACE, "Accel trace length: " + to_string(len));

  offset = offsetSamples;
  peaks.clear();

  // The first peak is a dummy extremum at the first sample.
  PeakDetect::logFirst(samples);

  for (unsigned i = 1; i < len-1; i++)
  {
    bool maxFlag;
    while (i < len-1 && samples[i] == samples[i-1])
      i++;

    if (i == len-1)
      break;

    if (samples[i] > samples[i-1])
    {
      // Use the last of equals as the starting point.
      while (i < len-1 && samples[i] == samples[i+1])
        i++;

      if (i == len-1 || samples[i] <= samples[i+1])
        continue;
      else
        maxFlag = true;
    }
    else
    {
      while (i < len-1 && samples[i] == samples[i+1])
        i++;

      if (i == len-1 || samples[i] >= samples[i+1])
        continue;
      else
        maxFlag = false;
    }

    const auto& peakPrev = peaks.back();
    const float areaFull = PeakDetect::integrate(samples, 
      peakPrev.getIndex(), i);
    const float areaCumPrev = peakPrev.getAreaCum();

    // The peak contains data for the interval preceding it.
    peaks.emplace_back(Peak());
    peaks.back().log(i, samples[i], areaCumPrev + areaFull, maxFlag);
  }

  // The last peak is a dummy extremum at the last sample.
  PeakDetect::logLast(samples);

  PeakDetect::annotate();

  PeakDetect::check(samples);
}


const list<Peak>::iterator PeakDetect::collapsePeaks(
  const list<Peak>::iterator peak1,
  const list<Peak>::iterator peak2)
{
  // Analogous to list.erase(), peak1 does not survive, while peak2 does.
if (peak2 == peaks.end())
  cout << "ERROR HOPPLA" << endl;

  if (peak1 == peak2)
    return peak2;

  // Need same polarity.
  if (peak1->getMaxFlag() != peak2->getMaxFlag())
    THROW(ERR_ALGO_PEAK_COLLAPSE, "Peaks with different polarity");

  Peak * peak0 = 
    (peak1 == peaks.begin() ? &*peak1 : &*prev(peak1));
  Peak * peakN = 
    (next(peak2) == peaks.end() ? nullptr : &*next(peak2));

  peak2->update(peak0, peakN);

  return peaks.erase(peak1, peak2);
}


const list<Peak>::iterator PeakDetect::collapsePeaksNew(
  const list<Peak>::iterator peak1,
  const list<Peak>::iterator peak2)
{
  // Analogous to list.erase(), peak1 does not survive, while peak2 does.
  if (peak1 == peak2)
    return peak1;

  Peak * peak0 = 
    (peak1 == peaksNew.begin() ? nullptr : &*prev(peak1));
  Peak * peakN = 
    (next(peak2) == peaksNew.end() ? nullptr : &*next(peak2));

  peak2->update(peak0, peakN);

  return peaksNew.erase(peak1, peak2);
}


void PeakDetect::reduceSmallRanges(const float rangeLimit)
{
  if (peaksNew.empty())
    THROW(ERR_NO_PEAKS, "Peak list is empty");

  const auto peakLast = prev(peaksNew.end());
  auto peak = next(peaksNew.begin());

  while (peak != peaksNew.end())
  {
    const float range = peak->getRange();
    if (range >= rangeLimit)
    {
      peak++;
      continue;
    }

    auto peakCurrent = peak, peakMax = peak;
    const bool maxFlag = peak->getMaxFlag();
    float sumRange = 0.f, lastRange = 0.f;
    float valueMax = numeric_limits<float>::lowest();

    do
    {
      peak++;
      if (peak == peaksNew.end())
        break;

      // sumRange = peak->getArea(* peakCurrent);
      sumRange = abs(peak->getValue() - peakCurrent->getValue());
      lastRange = peak->getRange();
      const float value = peak->getValue();
      if (! maxFlag && value > valueMax)
      {
        valueMax = value;
        peakMax = peak;
      }
      else if (maxFlag && -value > valueMax)
      {
        valueMax = -value;
        peakMax = peak;
      }
    }
    while (abs(sumRange) < rangeLimit || abs(lastRange) < rangeLimit);

    if (abs(sumRange) < rangeLimit || abs(lastRange) < rangeLimit)
    {
      // It's the last set of peaks.  We could keep the largest peak
      // of the same polarity as peakCurrent (instead of peakCurrent).
      // It's a bit random whether or not this would be a "real" peak,
      // and we also don't keep track of this above.  So we just stop.
      if (peakCurrent != peaksNew.end())
        peaksNew.erase(peakCurrent, peaksNew.end());
      break;
    }
/* */
    else if (peakCurrent->getValue() > 0.f &&
             ! peakCurrent->getMaxFlag())
    {
      // Don't connect two positive maxima.  This can mess up
      // the gradient calculation which influences peak perception.
      peak++;
    }
/* */
    else if (peak->getMaxFlag() != maxFlag)
    {
      // Keep from peakCurrent to peak which is also often peakMax.
      peak = PeakDetect::collapsePeaksNew(--peakCurrent, peak);
      peak++;
    }
    else
    {
      // Keep the start, the most extreme peak of opposite polarity,
      // and the end.
/*
cout << "peakCurrent " << peakCurrent->getIndex() + offset << endl;
cout << "peakMax " << peakMax->getIndex() + offset << endl;
cout << "peak " << peak->getIndex() + offset << endl;
cout << peakCurrent->str(offset);
cout << peakMax->str(offset);
cout << peak->str(offset);
*/

      peakMax = PeakDetect::collapsePeaksNew(--peakCurrent, peakMax);
      peak = PeakDetect::collapsePeaksNew(++peakMax, peak);
      peak++;
    }
  }
}


void PeakDetect::reduceSmallAreas(const float areaLimit)
{
  if (peaks.empty())
    THROW(ERR_NO_PEAKS, "Peak list is empty");

  auto peak = next(peaks.begin());

  while (peak != peaks.end())
  {
    const float area = peak->getArea();
    if (area >= areaLimit)
    {
      peak++;
      continue;
    }

    auto peakCurrent = peak, peakMax = peak;
    const bool maxFlag = peak->getMaxFlag();
    float sumArea = 0.f, lastArea = 0.f;
    float valueMax = numeric_limits<float>::lowest();

    do
    {
      peak++;
      if (peak == peaks.end())
        break;

      sumArea = peak->getArea(* peakCurrent);
      lastArea = peak->getArea();
      const float value = peak->getValue();
      if (! maxFlag && value > valueMax)
      {
        valueMax = value;
        peakMax = peak;
      }
      else if (maxFlag && -value > valueMax)
      {
        valueMax = -value;
        peakMax = peak;
      }
    }
    while (abs(sumArea) < areaLimit || abs(lastArea) < areaLimit);

    if (abs(sumArea) < areaLimit || abs(lastArea) < areaLimit)
    {
      // It's the last set of peaks.  We could keep the largest peak
      // of the same polarity as peakCurrent (instead of peakCurrent).
      // It's a bit random whether or not this would be a "real" peak,
      // and we also don't keep track of this above.  So we just stop.
      if (peakCurrent != peaks.end())
        peaks.erase(peakCurrent, peaks.end());
      break;
    }
    else if (peak->getMaxFlag() != maxFlag)
    {
      // Keep from peakCurrent to peak which is also often peakMax.
      peak = PeakDetect::collapsePeaks(--peakCurrent, peak);
      peak++;
    }
    else
    {
      // Keep the start, the most extreme peak of opposite polarity,
      // and the end.
/*
cout << "peakCurrent " << peakCurrent->getIndex() + offset << endl;
cout << "peakMax " << peakMax->getIndex() + offset << endl;
cout << "peak " << peak->getIndex() + offset << endl;
cout << peakCurrent->str(offset);
cout << peakMax->str(offset);
cout << peak->str(offset);
*/

      peakMax = PeakDetect::collapsePeaks(--peakCurrent, peakMax);
      peak = PeakDetect::collapsePeaks(++peakMax, peak);
      peak++;
    }
  }
}


void PeakDetect::eliminateKinks()
{
  if (peaks.size() < 4)
    THROW(ERR_NO_PEAKS, "Peak list is short");

  for (auto peak = next(peaks.begin(), 2); 
      peak != prev(peaks.end()); )
  {
// cout << "peak " << peak->getIndex() << " count " << peaks.size() <<
  // endl;
    const auto peakPrev = prev(peak);
    const auto peakPrevPrev = prev(peakPrev);
    const auto peakNext = next(peak);

    const float areaPrev = peak->getArea(* peakPrev);
    if (areaPrev == 0.f)
      THROW(ERR_ALGO_PEAK_COLLAPSE, "Zero area");

    const float ratioPrev = 
      peakPrev->getArea(* peakPrevPrev) / areaPrev;
    const float ratioNext = peakNext->getArea(* peak) / areaPrev;

    if (ratioPrev > 1.f && 
        ratioNext > 1.f &&
        ratioPrev * ratioNext > KINK_RATIO)
    {
      // Candidate for removal.  But if it changes the gradient
      // too much, don't do it.

      const float grad = peakPrev->getGradient();
      const float lenNew = peakPrev->getLength() +
        peak->getLength() + peakNext->getLength();
      const float rangeNew = 
        abs(peakNext->getValue() - peakPrev->getValue()) +
        peakPrev->getRange();
      const float gradNew = rangeNew / lenNew;

      if (gradNew > 1.1f * grad || gradNew < 0.9f * grad)
        peak++;
      else
        peak = PeakDetect::collapsePeaks(peakPrev, peakNext);
    }
    else
      peak++;
  }
}


void PeakDetect::estimateScales()
{
  scalesList.reset();
  unsigned no = 0;
  for (auto peak = next(peaks.begin()); peak != peaks.end(); peak++)
  {
    if (! peak->getMaxFlag() && peak->getValue() < 0.f)
    {
      scalesList += * peak;
      no++;
    }
  }

  if (no == 0)
    THROW(ERR_NO_PEAKS, "No negative minima");

  scalesList /= no;
}


void PeakDetect::pos2time(
  const vector<PeakPos>& posTrue, 
  const double speed,
  vector<PeakTime>& timesTrue) const
{
  for (unsigned i = 0; i < posTrue.size(); i++)
  {
    timesTrue[i].time = posTrue[i].pos / speed;
    timesTrue[i].value = posTrue[i].value;
  }
}


bool PeakDetect::advance(list<Peak>::iterator& peak) const
{
  while (peak != peaks.end() && ! peak->isSelected())
    peak++;
  return (peak != peaks.end());
}


double PeakDetect::simpleScore(
  const vector<PeakTime>& timesTrue,
  const double offsetScore,
  const bool logFlag,
  double& shift)
{
  // This is similar to Align::simpleScore.
  
  list<Peak>::iterator peak = peaks.begin();
  if (! PeakDetect::advance(peak))
    THROW(ERR_NO_PEAKS, "No peaks present or selected");

  const auto peakFirst = peak;
  list<Peak>::iterator peakBest, peakPrev;
  double score = 0.;
  unsigned scoring = 0;

  for (unsigned tno = 0; tno < timesTrue.size(); tno++)
  {
    double d = TIME_PROXIMITY, dabs = TIME_PROXIMITY;
    const double timeTrue = timesTrue[tno].time;
    double timeSeen = peak->getTime() - offsetScore;

    d = timeSeen - timeTrue;
    if (d >= 0.)
    {
      dabs = d;
      peakBest = peak;
    }
    else
    {
      double dleft = numeric_limits<double>::max();
      double dright = numeric_limits<double>::max();
      while (PeakDetect::advance(peak))
      {
        timeSeen = peak->getTime() - offsetScore;
        dright = timeSeen - timeTrue;
        if (dright >= 0.)
          break;
        else
        {
          peakPrev = peak;
          peak++;
        }
      }

      if (dright < 0.)
      {
        // We reached the end.
        dabs = -dright;
        d = dright;
        peakBest = peakPrev;
      }
      else 
      {
        dleft = timeTrue - (peakPrev->getTime() - offsetScore);
        if (dleft <= dright)
        {
          dabs = dleft;
          d = -dleft;
          peakBest = peakPrev;
        }
        else
        {
          dabs = dright;
          d = dright;
          peakBest = peak;
        }
      }
    }

    if (dabs <= TIME_PROXIMITY)
    {
      score += (TIME_PROXIMITY - dabs) / TIME_PROXIMITY;
      shift += d;
      scoring++;
      if (logFlag)
        peakBest->logMatch(tno);
    }
  }

  if (scoring)
    shift /= scoring;

  return score;
}


void PeakDetect::setOffsets(
  const vector<PeakTime>& timesTrue,
  vector<double>& offsetList) const
{
  const unsigned lp = peaks.size();
  const unsigned lt = timesTrue.size();

  // Probably too many.
  const unsigned maxShiftSeen = 3;
  const unsigned maxShiftTrue = 3;
  if (lp <= maxShiftSeen || lt <= maxShiftTrue)
    THROW(ERR_NO_PEAKS, "Not enough peaks");

  offsetList.resize(maxShiftSeen + maxShiftTrue + 1);

  // Offsets are subtracted from the seen peaks.
  list<Peak>::const_iterator peak = peaks.end();
  for (unsigned i = 0; i <= maxShiftSeen; i++)
  {
    do
    {
      peak = prev(peak);
    }
    while (peak != peaks.begin() && ! peak->isSelected());

    offsetList[i] = peak->getTime() - timesTrue[lt-1].time;
  }

  peak = peaks.end();
  do
  {
    peak = prev(peak);
  }
  while (peak != peaks.begin() && ! peak->isSelected());
  const double lastTime = peak->getTime();

  for (unsigned i = 1; i <= maxShiftTrue; i++)
    offsetList[maxShiftSeen+i] = timesTrue[lt-1-i].time - lastTime;
}


bool PeakDetect::findMatch(
  const vector<PeakTime>& timesTrue,
  double& shift)
{
  vector<double> offsetList;
  PeakDetect::setOffsets(timesTrue, offsetList);

  double score = 0.;
  unsigned ino = 0;
  shift = 0.;
  for (unsigned i = 0; i < offsetList.size(); i++)
  {
    double shiftNew = 0.;
    double scoreNew = 
      PeakDetect::simpleScore(timesTrue, offsetList[i], false, shiftNew);
cout << "i " << i << ": " << scoreNew << endl;
    if (scoreNew > score)
    {
cout << "*\n";
      score = scoreNew;
      shift = shiftNew;
      ino = i;
    }
  }

  // This extra run could be eliminated at the cost of some copying
  // and intermediate storage.
  double tmp;
  shift += offsetList[ino];
  score = PeakDetect::simpleScore(timesTrue, shift, true, tmp);
cout << "Final score " << score << " vs. " <<
  timesTrue.size() << " real and " << 
  numTentatives << " tentatively detected peaks" << endl << endl;

  if (score >= SCORE_CUTOFF * timesTrue.size())
    return true;
  else if (numTentatives >= 0.5 * timesTrue.size() &&
      score >= SCORE_CUTOFF * numTentatives)
    return true;
  else
    return false;
}
  

PeakType PeakDetect::findCandidate(
  const double time,
  const double shift) const
{
  double dist = numeric_limits<double>::max();
  PeakType type = PEAK_TRUE_MISSING;

  for (auto& peak: peaks)
  {
    if (! peak.isCandidate() || peak.isSelected())
      continue;
    
    const double pt = peak.getTime() - shift;
    const double distNew = time - pt;
    if (abs(distNew) < dist)
    {
      dist = abs(distNew);
      if (dist < TIME_PROXIMITY)
        type = peak.getType();
    }

    if (distNew < 0.)
      break;
  }
  return type;
}


void PeakDetect::setOrigPointers()
{
  for (auto peak = peaks.begin(), peakNew = peaksNew.begin(); 
      peak != peaks.end(); peak++, peakNew++)
    peakNew->logOrigPointer(&*peak);
}


void PeakDetect::markPossibleQuiet()
{
  quietCandidates.clear();

  for (auto peak = peaksNew.begin(); peak != peaksNew.end(); peak++)
  {
    // Get a negative minimum.
    const float value = peak->getValue();
    if (value >= 0. || peak->getMaxFlag())
      continue;

    // Find the previous negative minimum that was deeper.
    auto peakPrev = peak;
    bool posFlag = false;
    bool maxPrevFlag;
    float vPrev = 0.;
    float lowerMinValue = 0.f;
    do
    {
      if (peakPrev == peaksNew.begin())
        break;

      peakPrev = prev(peakPrev);
      maxPrevFlag = peakPrev->getMaxFlag();
      vPrev = peakPrev->getValue();

      if (! maxPrevFlag && vPrev > value)
        lowerMinValue = min(vPrev, lowerMinValue);

      if (vPrev >= 0.f)
        posFlag = true;
    }
    while (vPrev >= value || maxPrevFlag);

    if (posFlag && lowerMinValue < 0.f &&
        peakPrev != peaksNew.begin() && vPrev < value)
    {
      quietCandidates.emplace_back(Period());
      Period& p = quietCandidates.back();
      p.start = peakPrev->getIndex();
      p.len = peak->getIndex() - p.start;
      p.depth = (lowerMinValue - vPrev) / scalesList.getRange();
      p.minLevel = lowerMinValue;
    }

    // Same thing forwards.
    auto peakNext = peak;
    posFlag = false;
    bool maxNextFlag;
    float vNext = 0.;
    lowerMinValue = 0.f;
    do
    {
      if (peakNext == peaksNew.end())
        break;

      peakNext = next(peakNext);
      maxNextFlag = peakNext->getMaxFlag();
      vNext = peakNext->getValue();

      if (! maxNextFlag && vNext > value)
        lowerMinValue = min(vNext, lowerMinValue);

      if (vNext >= 0.f)
        posFlag = true;
    }
    while (vNext >= value || maxNextFlag);

    if (posFlag && lowerMinValue < 0.f &&
        peakNext != peaksNew.end() && vNext < value)
    {
      quietCandidates.emplace_back(Period());
      Period& p = quietCandidates.back();
      p.start = peak->getIndex();
      p.len = peakNext->getIndex() - p.start;
      p.depth = (lowerMinValue - vNext) / scalesList.getRange();
      p.minLevel = lowerMinValue;
    }
  }
}


void PeakDetect::findFirstSize(
  const vector<unsigned>& dists,
  unsigned& lower,
  unsigned& upper,
  unsigned& counted,
  const unsigned lowerCount) const
{
  struct DistEntry
  {
    unsigned index;
    int direction;
    unsigned origin;

    bool operator < (const DistEntry& de2)
    {
      return (index < de2.index);
    };
  };

  vector<DistEntry> steps;
// cout << "\nRaw steps\n";
  for (auto d: dists)
  {
    steps.emplace_back(DistEntry());
    DistEntry& de1 = steps.back();
    de1.index = static_cast<unsigned>(0.9f * d);
    de1.direction = 1;
    de1.origin = d;
// cout << steps.size()-1 << ";" << de1.index << ";" << de1.direction << "\n";

    steps.emplace_back(DistEntry());
    DistEntry& de2 = steps.back();
    de2.index = static_cast<unsigned>(1.1f * d);
    de2.direction = -1;
    de2.origin = d;
// cout << steps.size()-1 << ";" << de2.index << ";" << de2.direction << "\n";
  }
// cout << "\n";

  sort(steps.begin(), steps.end());

  int bestCount = 0;
  unsigned bestValue = 0;
  unsigned bestUpperValue = 0;
  unsigned bestLowerValue = 0;
  int count = 0;
  unsigned dindex = steps.size();

// cout << "\nSteps\n";
  unsigned i = 0;
  while (i < dindex)
  {
    const unsigned step = steps[i].index;
    unsigned upperValue = 0;
    while (i < dindex && steps[i].index == step)
    {
      count += steps[i].direction;
      if (steps[i].direction == 1)
        upperValue = steps[i].origin;
      i++;
    }

// cout << step << ";" << count << ";" << i << endl;

    if (count > bestCount)
    {
      bestCount = count;
      bestValue = step;
      bestUpperValue = upperValue;

      if (i == dindex)
        THROW(ERR_NO_PEAKS, "Does not come back to zero");

      unsigned j = i;
      while (j < dindex && steps[j].direction == 1)
        j++;

      if (j == dindex || steps[j].direction == 1)
        THROW(ERR_NO_PEAKS, "Does not come back to zero");

      bestLowerValue = steps[j].origin;
    }
    else if (bestCount > 0 && count == 0)
    {
      // Don't take a "small" maximum.
      // TODO Should really be more discerning.
      if (bestCount < static_cast<int>(lowerCount))
      {
        bestCount = 0;
        bestUpperValue = 0;
        bestLowerValue = 0;
      }
      else
        break;
    }
  }
// cout << "\n";
// cout << "best count " << bestCount << ", " << bestValue << endl;
// cout << "range " << bestLowerValue << "-" << bestUpperValue << endl;

  lower = bestLowerValue;
  upper = bestUpperValue;
  counted = bestCount;
}


bool PeakDetect::checkQuantity(
  const unsigned actual,
  const unsigned reference,
  const float factor,
  const string& text) const
{
  if (actual > factor * reference || actual * factor < reference)
  {
    cout << "Suspect " << text << ": " << actual << " vs. " <<
      reference << endl;
    return false;
  }
  else
    return true;
}


bool PeakDetect::matchesCarStat(
  const Car& car, 
  const vector<CarStat>& carStats, 
  unsigned& index) const
{
  float bestDistance = numeric_limits<float>::max();
  unsigned bestIndex = 0;

  for (unsigned i = 0; i < carStats.size(); i++)
  {
    const float dist = carStats[i].distance(car);
cout << "Matching distance against " << i << ": " << dist << endl;
    if (dist < bestDistance)
    {
      bestDistance = dist;
      bestIndex = i;
    }
  }

  if (bestDistance <= 1.5f)
  {
    index = bestIndex;
    return true;
  }
  else
    return false;
}


bool PeakDetect::findFourWheeler(
  const unsigned start,
  const unsigned end,
  const bool leftGapPresent,
  const bool rightGapPresent,
  const vector<PeakEntry>& peaksAnnot,
  const vector<unsigned>& peakNos, 
  const vector<unsigned>& peakIndices, 
  const Car& carAvg,
  Car& car) const
{
  car.start = start;
  car.end = end;
  car.partialFlag = false;

  if (leftGapPresent)
  {
    car.leftGap = peakNos[0] - start;
    car.leftGapSet = true;
  }
  else
  {
    car.leftGap = 0;
    car.leftGapSet = false;
  }

  if (rightGapPresent)
  {
    car.rightGap = end - peakNos[3];
    car.rightGapSet = true;
  }
  else
  {
    car.rightGap = 0;
    car.rightGapSet = false;
  }

  car.leftBogeyGap = peakNos[1] - peakNos[0];
  car.midGap = peakNos[2] - peakNos[1];
  car.rightBogeyGap = peakNos[3] - peakNos[2];

  car.firstBogeyLeft = peaksAnnot[peakIndices[0]].peakPtr;
  car.firstBogeyRight = peaksAnnot[peakIndices[1]].peakPtr;
  car.secondBogeyLeft = peaksAnnot[peakIndices[2]].peakPtr;
  car.secondBogeyRight = peaksAnnot[peakIndices[3]].peakPtr;

  if (leftGapPresent)
  {
    if (! PeakDetect::checkQuantity(car.leftGap,
        carAvg.leftGap, 2.f, "left gap"))
      return false;

    if (car.leftGap < 0.5f * car.leftBogeyGap)
    {
      cout << "Suspect left gap: " << car.leftGap << 
        " vs. bogey gap " << car.leftBogeyGap << endl;
      return false;
    }
  }

  if (rightGapPresent)
  {
    if (! PeakDetect::checkQuantity(car.rightGap,
        carAvg.rightGap, 2.f, "right gap"))
      return false;
    if (car.rightGap < 0.5f * car.rightBogeyGap)
    {
      cout << "Suspect right gap: " << car.rightGap << 
        " vs. bogey gap " << car.rightBogeyGap << endl;
      return false;
    }
  }

  if (! PeakDetect::checkQuantity(car.leftBogeyGap, 
      car.rightBogeyGap, 1.2f, "bogey size"))
    return false;

  if (car.midGap < 1.5f * car.leftBogeyGap)
  {
    cout << "Suspect mid-gap: " << car.midGap << 
      " vs. bogey gap " << car.leftBogeyGap << endl;
    return false;
  }

  return true;
}


bool PeakDetect::findLastTwoOfFourWheeler(
  const unsigned start, 
  const unsigned end,
  const bool rightGapPresent,
  const vector<PeakEntry>& peaksAnnot, 
  const vector<unsigned>& peakNos, 
  const vector<unsigned>& peakIndices,
  const vector<CarStat>& carStats, 
  Car& car) const
{
  car.start = start;
  car.end = end;
  car.partialFlag = true;

  car.leftGap = 0;
  car.leftGapSet = false;

  if (rightGapPresent)
  {
    car.rightGap = end - peakNos[1];
    car.rightGapSet = true;
  }
  else
  {
    car.rightGap = 0;
    car.rightGapSet = false;
  }

  car.leftBogeyGap = 0;
  car.rightBogeyGap = peakNos[1] - peakNos[0];

  car.firstBogeyLeft = nullptr;
  car.firstBogeyRight = nullptr;
  car.secondBogeyLeft = peaksAnnot[peakIndices[0]].peakPtr;
  car.secondBogeyRight = peaksAnnot[peakIndices[1]].peakPtr;

  if (rightGapPresent)
  {
    if (! PeakDetect::checkQuantity(car.rightGap,
        carStats[0].carAvg.rightGap, 2.f, "right gap"))
      return false;
    if (car.rightGap < 0.5f * car.rightBogeyGap)
    {
      cout << "Suspect right gap: " << car.rightGap << 
        " vs. bogey gap " << car.rightBogeyGap << endl;
      return false;
    }
  }

  // As we don't have a complete car, we'll at least require the
  // right bogey gap to be similar to something we've seen.

  bool foundFlag = false;
  for (unsigned i = 0; i < carStats.size(); i++)
  {
    if (car.rightBogeyGap >= 0.9f * carStats[i].carAvg.rightBogeyGap &&
        car.rightBogeyGap <= 1.1f * carStats[i].carAvg.rightBogeyGap)
    {
      foundFlag = true;
      break;
    }
  }

  if (! foundFlag)
  {
    if (carStats.size() == 1)
    {
      if (car.rightBogeyGap <= 0.75f * carStats[0].carAvg.rightBogeyGap ||
          car.rightBogeyGap >= 1.25f * carStats[0].carAvg.rightBogeyGap)
      {
        cout << "Suspect right bogey gap: " << car.rightBogeyGap << endl;
        return false;
      }
    }
    else if (carStats.size() == 2)
    {
      if (car.rightBogeyGap >= carStats[0].carAvg.rightBogeyGap &&
          car.rightBogeyGap >= carStats[1].carAvg.rightBogeyGap)
      {
        cout << "Suspect right bogey gap: " << car.rightBogeyGap << endl;
        return false;
      }

      if (car.rightBogeyGap <= carStats[0].carAvg.rightBogeyGap &&
          car.rightBogeyGap <= carStats[1].carAvg.rightBogeyGap)
      {
        cout << "Suspect right bogey gap: " << car.rightBogeyGap << endl;
        return false;
      }
    }
    else
    {
      cout << "Suspect right bogey gap: " << car.rightBogeyGap << endl;
      return false;
    }
  }

  return true;
}


bool PeakDetect::findLastThreeOfFourWheeler(
  const unsigned start, 
  const unsigned end,
  const bool rightGapPresent,
  const vector<PeakEntry>& peaksAnnot, 
  const vector<unsigned>& peakNos, 
  const vector<unsigned>& peakIndices,
  const vector<CarStat>& carStats, 
  Car& car,
  unsigned& numWheels) const
{
  car.start = start;
  car.end = end;
  car.partialFlag = true;

  car.leftGap = 0;
  car.leftGapSet = false;

  if (rightGapPresent)
  {
    car.rightGap = end - peakNos[2];
    car.rightGapSet = true;
  }
  else
  {
    car.rightGap = 0;
    car.rightGapSet = false;
  }

  car.leftBogeyGap = 0;
  car.midGap = peakNos[1] - peakNos[0];
  car.rightBogeyGap = peakNos[2] - peakNos[1];

  car.firstBogeyLeft = nullptr;
  car.firstBogeyRight = peaksAnnot[peakIndices[0]].peakPtr;
  car.secondBogeyLeft = peaksAnnot[peakIndices[1]].peakPtr;
  car.secondBogeyRight = peaksAnnot[peakIndices[2]].peakPtr;

  if (rightGapPresent)
  {
    if (! PeakDetect::checkQuantity(car.rightGap,
        carStats[0].carAvg.rightGap, 2.f, "right gap"))
      return false;
    if (car.rightGap < 0.5f * car.rightBogeyGap)
    {
      cout << "Suspect right gap: " << car.rightGap << 
        " vs. bogey gap " << car.rightBogeyGap << endl;
      return false;
    }
  }

  // As we don't have a complete car, we'll at least require the
  // right bogey gap to be similar to something we've seen.

  bool foundFlag = false;
  for (unsigned i = 0; i < carStats.size(); i++)
  {
    if (car.rightBogeyGap >= 0.9f * carStats[i].carAvg.rightBogeyGap &&
        car.rightBogeyGap <= 1.1f * carStats[i].carAvg.rightBogeyGap)
    {
      foundFlag = true;
      break;
    }
  }

  if (! foundFlag)
  {
    if (carStats.size() == 2)
    {
      if (car.rightBogeyGap >= carStats[0].carAvg.rightBogeyGap &&
          car.rightBogeyGap >= carStats[1].carAvg.rightBogeyGap)
      {
        cout << "Suspect right bogey gap: " << car.rightBogeyGap << endl;
        return false;
      }

      if (car.rightBogeyGap <= carStats[0].carAvg.rightBogeyGap &&
          car.rightBogeyGap <= carStats[1].carAvg.rightBogeyGap)
      {
        cout << "Suspect right bogey gap: " << car.rightBogeyGap << endl;
        return false;
      }
    }
    else
    {
      cout << "Suspect right bogey gap: " << car.rightBogeyGap << endl;
      return false;
    }

  }

  if (car.midGap < 1.5f * car.rightBogeyGap)
  {
    // Drop the first peak.
    numWheels = 2;
    car.firstBogeyRight = nullptr;
    cout << "Dropping very first peak as too close\n";
    return true;
  }
  else
  {
    numWheels = 3;
    return true;
  }

  return true;

}


void PeakDetect::fixTwoWheels(
  vector<PeakEntry>& peaksAnnot,
  const vector<unsigned>& peakIndices,
  const unsigned p0,
  const unsigned p1) const
{
  // Assume the two rightmost wheels, as the front ones were lost.
  peaksAnnot[peakIndices[p0]].wheelFlag = true;
  peaksAnnot[peakIndices[p0]].wheelSide = WHEEL_LEFT;
  peaksAnnot[peakIndices[p0]].bogeySide = BOGEY_RIGHT;

  peaksAnnot[peakIndices[p1]].wheelFlag = true;
  peaksAnnot[peakIndices[p1]].wheelSide = WHEEL_RIGHT;
  peaksAnnot[peakIndices[p1]].bogeySide = BOGEY_RIGHT;

  for (unsigned i = 0; i < peakIndices.size(); i++)
  {
    if (i != p0 && i != p1)
      peaksAnnot[peakIndices[i]].wheelFlag = false;
  }
}


void PeakDetect::fixThreeWheels(
  vector<PeakEntry>& peaksAnnot,
  const vector<unsigned>& peakIndices,
  const unsigned p0,
  const unsigned p1,
  const unsigned p2) const
{
  // Assume the two rightmost wheels, as the front one was lost.
  peaksAnnot[peakIndices[p0]].wheelFlag = true;
  peaksAnnot[peakIndices[p0]].wheelSide = WHEEL_RIGHT;
  peaksAnnot[peakIndices[p0]].bogeySide = BOGEY_LEFT;

  peaksAnnot[peakIndices[p1]].wheelFlag = true;
  peaksAnnot[peakIndices[p1]].wheelSide = WHEEL_LEFT;
  peaksAnnot[peakIndices[p1]].bogeySide = BOGEY_RIGHT;

  peaksAnnot[peakIndices[p2]].wheelFlag = true;
  peaksAnnot[peakIndices[p2]].wheelSide = WHEEL_RIGHT;
  peaksAnnot[peakIndices[p2]].bogeySide = BOGEY_RIGHT;

  for (unsigned i = 0; i < peakIndices.size(); i++)
  {
    if (i != p0 && i != p1 && i != p2)
      peaksAnnot[peakIndices[i]].wheelFlag = false;
  }
}


void PeakDetect::fixFourWheels(
  vector<PeakEntry>& peaksAnnot,
  const vector<unsigned>& peakIndices,
  const unsigned p0,
  const unsigned p1,
  const unsigned p2,
  const unsigned p3) const
{
  peaksAnnot[peakIndices[p0]].wheelFlag = true;
  peaksAnnot[peakIndices[p0]].wheelSide = WHEEL_LEFT;
  peaksAnnot[peakIndices[p0]].bogeySide = BOGEY_LEFT;

  peaksAnnot[peakIndices[p1]].wheelFlag = true;
  peaksAnnot[peakIndices[p1]].wheelSide = WHEEL_RIGHT;
  peaksAnnot[peakIndices[p1]].bogeySide = BOGEY_LEFT;

  peaksAnnot[peakIndices[p2]].wheelFlag = true;
  peaksAnnot[peakIndices[p2]].wheelSide = WHEEL_LEFT;
  peaksAnnot[peakIndices[p2]].bogeySide = BOGEY_RIGHT;

  peaksAnnot[peakIndices[p3]].wheelFlag = true;
  peaksAnnot[peakIndices[p3]].wheelSide = WHEEL_RIGHT;
  peaksAnnot[peakIndices[p3]].bogeySide = BOGEY_RIGHT;

  for (unsigned i = 0; i < peakIndices.size(); i++)
  {
    if (i != p0 && i != p1 && i != p2 && i != p3)
      peaksAnnot[peakIndices[i]].wheelFlag = false;
  }
}


void PeakDetect::updateCars(
  vector<Car>& cars,
  vector<CarStat>& carStats,
  Car& car,
  const unsigned numWheels) const
{
  unsigned index;
  if (! car.partialFlag &&
      PeakDetect::matchesCarStat(car, carStats, index))
  {
    car.catStatIndex = index;
    cars.push_back(car);

    carStats[index] += car;
cout << "Recognized car " << index << endl;
  }
  else
  {
    car.catStatIndex = carStats.size();
    cars.push_back(car);

    carStats.emplace_back(CarStat());
    CarStat& cs = carStats.back();
    cs += car;
    cs.numWheels = numWheels;
cout << "Created new car\n";
  }
}


bool PeakDetect::findCars(
  const unsigned start,
  const unsigned end,
  const bool leftGapPresent,
  const bool rightGapPresent,
  vector<PeakEntry>& peaksAnnot,
  vector<Car>& cars,
  vector<CarStat>& carStats) const
{
  vector<unsigned> peakNos, peakIndices;
  unsigned notTallCount = 0;
  vector<unsigned> notTallNos;
  bool notTallFlag = false;

  for (unsigned i = 0; i < peaksAnnot.size(); i++)
  {
    const unsigned index = peaksAnnot[i].peakPtr->getIndex();
    if (index >= start && index <= end)
    {
      if (! peaksAnnot[i].tallFlag)
      {
        notTallFlag = true;
        notTallNos.push_back(peakNos.size());
        notTallCount++;
      }

      peakNos.push_back(index);
      peakIndices.push_back(i);
      
    }
  }

  const unsigned np = peakNos.size();

  if (np >= 8)
  {
    // Might be two cars.
    vector<unsigned> peakNosNew, peakIndicesNew;

    for (unsigned i: peakIndices)
    {
      if (peaksAnnot[i].qualityShape <= 0.5f)
      {
        peakNosNew.push_back(peaksAnnot[i].peakPtr->getIndex());
        peakIndicesNew.push_back(i);
      }
    }

    if (peakNosNew.size() != 8)
    {
      cout << "I don't yet see two cars here: " << np << ", " <<
        peakNosNew.size() << endl;
      return false;
    }

    peakNos.clear();
    peakIndices.clear();
    vector<unsigned> peakNosOther, peakIndicesOther;

    for (unsigned i = 0; i < 4; i++)
    {
      peakNos.push_back(peakNosNew[i]);
      peakIndices.push_back(peakIndicesNew[i]);

      peakNosOther.push_back(peakNosNew[i+4]);
      peakIndicesOther.push_back(peakIndicesNew[i+4]);
    }


    Car car, carOther;
    // Try the first four peaks.
cout << "Trying first of two cars\n";
    if (! PeakDetect::findFourWheeler(start, peakIndices[3],
        leftGapPresent, false,
        peaksAnnot, peakNos, peakIndices,
        carStats[0].carAvg, car))
    {
cout << "Failed first car\n";
      return false;
    }

cout << "Trying second of two cars\n";
    if (! PeakDetect::findFourWheeler(peakIndices[4], end, 
        false, rightGapPresent,
        peaksAnnot, peakNosOther, peakIndicesOther,
        carStats[0].carAvg, carOther))
    {
cout << "Failed second car\n";
      return false;
    }

    PeakDetect::fixFourWheels(peaksAnnot, peakIndices, 0, 1, 2, 3);
    PeakDetect::updateCars(cars, carStats, car, 4);

    PeakDetect::fixFourWheels(peaksAnnot, peakIndicesOther, 0, 1, 2, 3);
    PeakDetect::updateCars(cars, carStats, carOther, 4);

    return true;
  }


  if (np <= 2)
  {
    cout << "Don't know how to do this yet: " << np << "\n";
    return false;
  }

  if (! leftGapPresent &&
      (np == 4 || np == 5) && 
      notTallFlag && 
      notTallCount == np-3)
  {
cout << "4-5 leading wheels: Attempting to drop down to 3: " << np << "\n";

    if (np == 5)
    {
      peakNos.erase(peakNos.begin() + notTallNos[1]);
      peakIndices.erase(peakIndices.begin() + notTallNos[1]);
    }

    peakNos.erase(peakNos.begin() + notTallNos[0]);
    peakIndices.erase(peakIndices.begin() + notTallNos[0]);

    Car car;
    unsigned numWheels;
    if (! PeakDetect::findLastThreeOfFourWheeler(start, end,
        rightGapPresent,
        peaksAnnot, peakNos, peakIndices,
        carStats, car, numWheels))
      return false;

    if (numWheels == 2)
      PeakDetect::fixTwoWheels(peaksAnnot, peakIndices, 1, 2);
    else 
      PeakDetect::fixThreeWheels(peaksAnnot, peakIndices, 0, 1, 2);

    PeakDetect::updateCars(cars, carStats, car, numWheels);

    return true;
  }

  if (np == 4)
  {
    Car car;
    if (PeakDetect::findFourWheeler(start, end, 
        leftGapPresent, rightGapPresent,
        peaksAnnot, peakNos, peakIndices,
        carStats[0].carAvg, car))
    {
      PeakDetect::fixFourWheels(peaksAnnot, peakIndices, 0, 1, 2, 3);

      PeakDetect::updateCars(cars, carStats, car, 4);

      return true;
    }
    
    // Give up unless this is the first car.
    if (leftGapPresent)
      return false;

    // Try again without the first peak.
cout << "Trying again without the very first peak of first car\n";
    peakNos.erase(peakNos.begin());
    peakIndices.erase(peakIndices.begin());

    unsigned numWheels;
    if (! PeakDetect::findLastThreeOfFourWheeler(start, end,
        rightGapPresent,
        peaksAnnot, peakNos, peakIndices,
        carStats, car, numWheels))
      return false;

    if (numWheels == 2)
      PeakDetect::fixTwoWheels(peaksAnnot, peakIndices, 1, 2);
    else 
      PeakDetect::fixThreeWheels(peaksAnnot, peakIndices, 0, 1, 2);

    PeakDetect::updateCars(cars, carStats, car, numWheels);

    return true;
  }

  if (np >= 5)
  {
    // Might be several extra peaks (1-2).
    vector<unsigned> peakNosNew, peakIndicesNew;

    float qmax = 0.f;
    unsigned qindex = 0;

    for (unsigned j = 0; j < peakIndices.size(); j++)
    {
      unsigned i = peakIndices[j];

      if (peaksAnnot[i].qualityShape > qmax)
      {
        qmax = peaksAnnot[i].qualityShape;
        qindex = j;
      }

      if (peaksAnnot[i].qualityShape <= 0.5f)
      {
        peakNosNew.push_back(peaksAnnot[i].peakPtr->getIndex());
        peakIndicesNew.push_back(i);
      }
    }

    if (peakNosNew.size() != 4)
    {
      // If there are five and the middle one stands out, try that.
      if (np == 5 && qindex == 2)
      {
cout << "General try with " << np << " didn't fit -- drop the middle?" <<
 endl;
        peakNos.erase(peakNos.begin() + 2);
        peakIndices.erase(peakIndices.begin() + 2);
      
        Car car;
    cout << "Trying the car anyway\n";
        if (! PeakDetect::findFourWheeler(start, end,
            leftGapPresent, rightGapPresent,
            peaksAnnot, peakNos, peakIndices,
            carStats[0].carAvg, car))
        {
    cout << "Failed the car: " << np << "\n";
          return false;
        }

        PeakDetect::fixFourWheels(peaksAnnot, peakIndices, 0, 1, 2, 3);
        PeakDetect::updateCars(cars, carStats, car, 4);
        return true;
      }
      else
      {
        cout << "I don't yet see one car here: " << np << ", " <<
          peakNosNew.size() << endl;

for (unsigned i: peakIndices)
  cout << "index " << peaksAnnot[i].peakPtr->getIndex() << ", qs " <<
    peaksAnnot[i].qualityShape << endl;

        return false;
      }
    }

    Car car;
    // Try the car.
cout << "Trying the car\n";
    // if (! PeakDetect::findFourWheeler(start, peakNosNew[3],
    if (! PeakDetect::findFourWheeler(start, end,
        leftGapPresent, rightGapPresent,
        // leftGapPresent, false,
        peaksAnnot, peakNosNew, peakIndicesNew,
        carStats[0].carAvg, car))
    {
cout << "Failed the car: " << np << "\n";
      return false;
    }

    PeakDetect::fixFourWheels(peaksAnnot, peakIndicesNew, 0, 1, 2, 3);
    PeakDetect::updateCars(cars, carStats, car, 4);
    return true;
  }

  if (! leftGapPresent && np == 3)
  {
cout << "Trying 3-peak leading car\n";
    // The first car, only three peaks.
    if (notTallCount == 1 && 
      ! peaksAnnot[peakIndices[0]].tallFlag)
    {
cout << "Two talls\n";
      // Skip the first peak.
      peakNos.erase(peakNos.begin() + notTallNos[0]);
      peakIndices.erase(peakIndices.begin() + notTallNos[0]);

      Car car;
      if (! PeakDetect::findLastTwoOfFourWheeler(start, end,
          rightGapPresent,
          peaksAnnot, peakNos, peakIndices,
          carStats, car))
      {
cout << "Failed\n";
        return false;
      }

      PeakDetect::fixTwoWheels(peaksAnnot, peakIndices, 0, 1);

      PeakDetect::updateCars(cars, carStats, car, 2);

      return true;
    }
    else if (peaksAnnot[peakIndices[0]].qualityShape > 0.5f &&
        peaksAnnot[peakIndices[1]].qualityShape <= 0.5f &&
        peaksAnnot[peakIndices[2]].qualityShape <= 0.5f)
    {
cout << "Two good shapes\n";
      // Skip the first peak.
      peakNos.erase(peakNos.begin() + notTallNos[0]);
      peakIndices.erase(peakIndices.begin() + notTallNos[0]);

      Car car;
      if (! PeakDetect::findLastTwoOfFourWheeler(start, end,
          rightGapPresent,
          peaksAnnot, peakNos, peakIndices,
          carStats, car))
      {
cout << "Failed\n";
        return false;
      }

      PeakDetect::fixTwoWheels(peaksAnnot, peakIndices, 0, 1);

      PeakDetect::updateCars(cars, carStats, car, 2);

      return true;
    }
  }

  cout << "Don't know how to do this yet: " << np << ", notTallCount " <<
    notTallCount << "\n";
  return false;

}


void PeakDetect::reduceNewer()
{
  // Here the idea is to use geometrical properties of the peaks
  // to extract some structure.

  struct QuietEntry
  {
    Period * periodPtr;
    unsigned indexLen;
    unsigned indexStart;
    unsigned indexEnd;
    bool usedFlag;
  };

  vector<QuietEntry> quietByLength;
  unsigned i = 0;
  for (auto& q: quietCandidates)
  {
    quietByLength.emplace_back(QuietEntry());
    QuietEntry& qe = quietByLength.back();
    qe.periodPtr = &q;
    qe.usedFlag = false;
  }

  sort(quietByLength.begin(), quietByLength.end(),
    [](const QuietEntry& qe1, const QuietEntry& qe2) -> bool
    { 
      return qe1.periodPtr->len < qe2.periodPtr->len; 
    });

  // These have indices back into quietByLength.
  vector<QuietEntry> quietByStart, quietByEnd;
  i = 0;
  for (auto& qbl: quietByLength)
  {
    quietByStart.emplace_back(qbl);
    QuietEntry& qs = quietByStart.back();
    qs.indexLen = i;

    quietByEnd.emplace_back(qbl);
    QuietEntry& qe = quietByEnd.back();
    qe.indexLen = i++;
  }

  sort(quietByStart.begin(), quietByStart.end(),
    [](const QuietEntry& qe1, const QuietEntry& qe2) -> bool
    { 
      if (qe1.periodPtr->start < qe2.periodPtr->start)
        return true;
      else if (qe1.periodPtr->start > qe2.periodPtr->start)
        return false;
      else
        return qe1.periodPtr->len < qe2.periodPtr->len; 
    });

  sort(quietByEnd.begin(), quietByEnd.end(),
    [](const QuietEntry& qe1, const QuietEntry& qe2) -> bool
    { 
      if (qe1.periodPtr->start + qe1.periodPtr->len < 
          qe2.periodPtr->start + qe2.periodPtr->len)
        return true;
      else if (qe1.periodPtr->start + qe1.periodPtr->len > 
          qe2.periodPtr->start + qe2.periodPtr->len)
        return false;
      else
        return qe1.periodPtr->len < qe2.periodPtr->len; 
    });

  // We can also get from quietByLength to the others.
  i = 0;
  for (auto& qbs: quietByStart)
    quietByLength[qbs.indexLen].indexStart = i++;
  
  i = 0;
  for (auto& qbe: quietByEnd)
    quietByLength[qbe.indexLen].indexEnd = i++;
  
  // Smaller intervals are contained in larger ones.
  vector<list<Period *>> nestedQuiets;

  const unsigned qlen = quietByLength.size();
  for (auto& qbl: quietByLength)
  {
    if (qbl.usedFlag)
      continue;

    nestedQuiets.emplace_back(list<Period *>());
    list<Period *>& li = nestedQuiets.back();

    QuietEntry * qeptr = &qbl;
    while (true)
    {
/*
      if (qeptr->usedFlag == true)
        break;
*/

      li.push_back(qeptr->periodPtr);
      qeptr->usedFlag = true;

      // Look in starts.
      const unsigned snext = qeptr->indexStart+1;
      const unsigned enext = qeptr->indexEnd+1;

      if (snext < qlen && 
          quietByStart[snext].periodPtr->start == qeptr->periodPtr->start)
      {
        // Same start, next up in length.
        qeptr = &quietByLength[quietByStart[snext].indexLen];
      }
      else if (enext < qlen && 
        quietByEnd[enext].periodPtr->start +
        quietByEnd[enext].periodPtr->len ==
        qeptr->periodPtr->start + qeptr->periodPtr->len)
      {
        // Look in ends.
        qeptr = &quietByLength[quietByEnd[enext].indexLen];
      }
      else
        break;
    }
  }

  // Sort the interval lists in order of appearance.
  sort(nestedQuiets.begin(), nestedQuiets.end(),
    [](const list<Period *>& li1, const list<Period *>& li2) -> bool
    { 
      return li1.front()->start < li2.front()->start;
    });

  cout << "Nested intervals before pruning\n";
  for (auto& li: nestedQuiets)
  {
    for (auto& p: li)
    {
      cout << p->start + offset << "-" << p->start + offset + p->len <<
        " (" << p->len << ")" << endl;
    }
    cout << endl;
  }

  // Merge lists that only differ in the first interval.
  // Could maybe mark these.
  for (i = 0; i+1 < nestedQuiets.size(); )
  {
    if (nestedQuiets[i].size() != nestedQuiets[i+1].size())
    {
      i++;
      continue;
    }

    if (nestedQuiets[i].size() <= 1)
    {
      i++;
      continue;
    }

    Period * p1 = nestedQuiets[i].front();
    Period * p2 = nestedQuiets[i+1].front();
    if (p1->start + p1->len == p2->start)
      nestedQuiets.erase(nestedQuiets.begin() + i+1);
    else
      i++;
  }

  // Stop lists when they reach exact overlap.
  for (i = 0; i+1 < nestedQuiets.size(); i++)
  {
    auto& li1 = nestedQuiets[i];
    auto& li2 = nestedQuiets[i+1];

    // Count up in the ends of the first list.
    unsigned index1 = 0;
    list<Period *>::iterator nit1;
    for (auto it = li1.begin(); it != prev(li1.end()); it++)
    {
      nit1 = next(it);
      if ((*nit1)->start + (*nit1)->len > li2.front()->start)
      {
        index1 = (*it)->start + (*it)->len;
        break;
      }
      else if (next(nit1) == li1.end())
      {
        index1 = (*nit1)->start + (*nit1)->len;
        nit1++;
        break;
      }
    }

    bool foundFlag = false;
    unsigned index2 = 0;
    list<Period *>::iterator it2;
   
    // Count down in the starts of the second list.
    for (it2 = li2.begin(); it2 != li2.end(); it2++)
    {
      if ((*it2)->start == index1)
      {
        foundFlag = true;
        index2 = (*it2)->start;
        while (it2 != li2.end() && (*it2)->start == index1)
          it2++;
        break;
      }
    }

    if (! foundFlag)
      continue;

    if (index1 != index2)
      continue;

    if (nit1 != li1.end())
      li1.erase(nit1, li1.end());
    li2.erase(it2, li2.end());
  }

  cout << "Nested intervals after pruning\n";
  for (auto& li: nestedQuiets)
  {
    for (auto& p: li)
    {
      cout << p->start + offset << "-" << p->start + offset + p->len <<
        " (" << p->len << ")" << endl;
    }
    cout << endl;
  }


  vector<PeakEntry> peaksAnnot;
  unsigned nindex = 0;
  unsigned ni1 = nestedQuiets[nindex].back()->start;
  unsigned ni2 = ni1 + nestedQuiets[nindex].back()->len;

  // Note which peaks are tall.

  for (auto pit = peaksNew.begin(); pit != peaksNew.end(); pit++)
  {
    // Only want the negative minima here.
    if (pit->getValue() >= 0.f || pit->getMaxFlag())
      continue;

    // Exclude tall peaks without a right neighbor.
    auto npit = next(pit);
    if (npit == peaksNew.end())
      continue;

    peaksAnnot.emplace_back(PeakEntry());
    PeakEntry& pe = peaksAnnot.back();

    pe.peakPtr = &*pit;
    pe.nextPeakPtr = &*npit;

    pe.wheelFlag = false;
    pe.wheelSide = WHEEL_SIZE;
    pe.bogeySide = BOGEY_SIZE;
    pe.spuriousFlag = false;

    const unsigned index = pit->getIndex();

    if (index < ni1)
      pe.tallFlag = false;
    else if (index == ni1)
      pe.tallFlag = true;
    else if (index < ni2)
      pe.tallFlag = false;
    else
    {
      if (index == ni2)
        pe.tallFlag = true;
      else
        pe.tallFlag = false;

      if (nindex+1 < nestedQuiets.size())
      {
        nindex++;
        ni1 = nestedQuiets[nindex].back()->start;
        ni2 = ni1 + nestedQuiets[nindex].back()->len;
      }
    }
  }

  for (auto pit = peaksAnnot.begin(); pit != peaksAnnot.end(); pit++)
  {
    pit->prevLargePeakPtr = (pit == peaksAnnot.begin() ? nullptr : 
      prev(pit)->peakPtr);
    pit->nextLargePeakPtr = (next(pit) == peaksAnnot.end() ? nullptr : 
      next(pit)->peakPtr);
  }

  // Find typical sizes of "tall" peaks.

  const unsigned np = peaksAnnot.size();
  if (np == 0)
    THROW(ERR_NO_PEAKS, "No tall peaks");
  vector<float> values, rangesLeft, rangesRight, gradLeft, gradRight;

  for (auto& pa: peaksAnnot)
  {
    auto pt = pa.peakPtr;
    auto npt = pa.nextPeakPtr;
    if (npt == nullptr)
      continue;

    pa.sharp.index = pt->getIndex();
    pa.sharp.level = pt->getValue();
    pa.sharp.range1 = pt->getRange();
    pa.sharp.grad1 = 10.f * pt->getGradient();
    pa.sharp.range2 = npt->getRange();
    pa.sharp.grad2 = 10.f * npt->getGradient();

    if (pa.sharp.range2 != 0.f)
      pa.sharp.rangeRatio = pa.sharp.range1 / pa.sharp.range2; 
    if (pa.sharp.grad2 != 0.f)
      pa.sharp.gradRatio = pa.sharp.grad1 / pa.sharp.grad2; 

    if (! pa.tallFlag)
      continue;

    values.push_back(pt->getValue());
    rangesLeft.push_back(pt->getRange());
    gradLeft.push_back(10.f * pt->getGradient());

    rangesRight.push_back(npt->getRange());
    gradRight.push_back(10.f * npt->getGradient());
  }

  Sharp tallSize;
  const unsigned nvm = values.size() / 2;
  nth_element(values.begin(), values.begin() + nvm, values.end());
  tallSize.level = values[nvm];

  nth_element(rangesLeft.begin(), rangesLeft.begin() + nvm, 
    rangesLeft.end());
  tallSize.range1 = rangesLeft[nvm];

  nth_element(gradLeft.begin(), gradLeft.begin() + nvm, gradLeft.end());
  tallSize.grad1 = gradLeft[nvm];

  nth_element(rangesRight.begin(), rangesRight.begin() + nvm, 
    rangesRight.end());
  tallSize.range2 = rangesRight[nvm];

  nth_element(gradRight.begin(), gradRight.begin() + nvm, gradRight.end());
  tallSize.grad2 = gradRight[nvm];

  if (tallSize.range2 != 0.f)
    tallSize.rangeRatio = tallSize.range1 / tallSize.range2;
  if (tallSize.grad2 != 0.f)
    tallSize.gradRatio = tallSize.grad1 / tallSize.grad2;

  // Print scale.
  cout << "Tall scale" << endl;
  cout << tallSize.strHeader();
  cout << tallSize.str(offset);
  cout << endl;

  // Look at the "tall" peaks.
  cout << tallSize.strHeaderQ();
  for (auto& pa: peaksAnnot)
  {
    pa.quality = pa.sharp.distToScale(tallSize);
    pa.qualityShape = pa.sharp.distToScaleQ(tallSize);

    if (! pa.tallFlag)
      continue;
    
    if (pa.sharp.range2 != 0.f)
      pa.sharp.rangeRatio = pa.sharp.range1 / pa.sharp.range2; 
    if (pa.sharp.grad2 != 0.f)
      pa.sharp.gradRatio = pa.sharp.grad1 / pa.sharp.grad2; 

    cout << pa.sharp.strQ(pa.quality, pa.qualityShape, offset);
  }
  cout << endl;

  cout << "All peaks\n";
  cout << tallSize.strHeaderQ();
  for (auto& pa: peaksAnnot)
    cout << pa.sharp.strQ(pa.quality, pa.qualityShape, offset);

  // Find a reasonable peak quality.
  vector<float> qualities;
  const unsigned nq = qualities.size() / 2;
  nth_element(qualities.begin(), qualities.begin() + nq, qualities.end());
  const float qlevel = qualities[nq];

  // Prune peaks that deviate too much.
  // Add other peaks that are as good.
  for (auto& pa: peaksAnnot)
  {
    if (pa.tallFlag)
    {
      if (pa.quality > 0.3f && pa.qualityShape > 0.15f)
      {
cout << "Removing tallFlag from " << 
  pa.peakPtr->getIndex()+offset << endl;
        pa.tallFlag = false;
      }
    }
    else if (pa.quality <= 0.3f)
    {
cout << "Adding tallFlag to " << 
  pa.peakPtr->getIndex()+offset << endl;
      pa.tallFlag = true;
    }
    else if (pa.qualityShape <= 0.15f)
    {
cout << "Adding tallFlag(shape) to " << 
  pa.peakPtr->getIndex()+offset << endl;
      pa.tallFlag = true;
    }
  }

  cout << "Selected peaks\n";
  for (auto& pa: peaksAnnot)
  {
    if (pa.tallFlag)
      cout << pa.sharp.strQ(pa.quality, pa.qualityShape, offset);
  }


  // Find the worst remaining peak quality.
  float worstQuality = 0.f;
  for (auto& pa: peaksAnnot)
  {
    if (! pa.tallFlag)
      continue;

    if (pa.quality > worstQuality)
      worstQuality = pa.quality;
  }
  cout << "Worst quality: " << worstQuality << endl;

  // Make list of distances between tall neighbors.
  vector<unsigned> dists;
  for (auto pit = peaksAnnot.begin(); pit != prev(peaksAnnot.end());
    pit++)
  {
    auto npit = next(pit);
    // if (pit->tallFlag || npit->tallFlag)
    if (pit->tallFlag && npit->tallFlag)
      dists.push_back(
        npit->peakPtr->getIndex() - pit->peakPtr->getIndex());
  }

  sort(dists.begin(), dists.end());
  unsigned wheelDistLower, wheelDistUpper;
  unsigned whcount;
  PeakDetect::findFirstSize(dists, wheelDistLower, wheelDistUpper,
    whcount);
  // Could be zero
cout << "Guessing wheel distance " << wheelDistLower << "-" <<
  wheelDistUpper << endl;

  // Tentatively mark wheel pairs (bogeys).  If there are only 
  // single wheels, we might be marking the wagon gaps instead.

  for (auto pit = peaksAnnot.begin(); pit != prev(peaksAnnot.end());
    pit++)
  {
    auto npit = next(pit);
    if (pit->tallFlag && npit->tallFlag)
    {
      const unsigned dist = 
        npit->peakPtr->getIndex() - pit->peakPtr->getIndex();
      if (dist >= wheelDistLower && dist <= wheelDistUpper)
      {
        if (pit->wheelFlag)
          THROW(ERR_NO_PEAKS, "Triple bogey?!");

        pit->wheelFlag = true;
        pit->wheelSide = WHEEL_LEFT;
        npit->wheelFlag = true;
        npit->wheelSide = WHEEL_RIGHT;
cout << "Marking bogey at " << pit->peakPtr->getIndex()+offset << "-" <<
  npit->peakPtr->getIndex()+offset << endl;
      }
    }
  }

  // Look for unpaired wheels where there is a nearby peak that is
  // not too bad.
  // TODO There could be a spurious peak in between.

  for (auto pit = peaksAnnot.begin(); pit != prev(peaksAnnot.end());
    pit++)
  {
    auto npit = next(pit);
    if (pit->tallFlag && ! npit->tallFlag)
    {
      const unsigned dist = 
        npit->peakPtr->getIndex() - pit->peakPtr->getIndex();

      if (dist < wheelDistLower || dist > wheelDistUpper)
        continue;

      if (npit->qualityShape <= 0.75f)
      {
cout << "Adding " <<
  npit->peakPtr->getIndex()+offset << 
  " as partner to " << 
  pit->peakPtr->getIndex()+offset << endl;
        
        npit->tallFlag = true;
        
        pit->wheelFlag = true;
        pit->wheelSide = WHEEL_LEFT;
        npit->wheelFlag = true;
        npit->wheelSide = WHEEL_RIGHT;
      }
    }
    else if (! pit->tallFlag && npit->tallFlag)
    {
      const unsigned dist = 
        npit->peakPtr->getIndex() - pit->peakPtr->getIndex();

      if (dist < wheelDistLower || dist > wheelDistUpper)
        continue;

      if (pit->qualityShape <= 0.75f)
      {
cout << "Adding " <<
  pit->peakPtr->getIndex()+offset << 
  " as partner to " << 
  npit->peakPtr->getIndex()+offset << endl;
        
        pit->tallFlag = true;
        
        pit->wheelFlag = true;
        pit->wheelSide = WHEEL_LEFT;
        npit->wheelFlag = true;
        npit->wheelSide = WHEEL_RIGHT;
      }
    }
  }

  // Make a list of likely short gaps.
  dists.clear();
  for (auto pit = peaksAnnot.begin(); pit != prev(peaksAnnot.end());
    pit++)
  {
    auto npit = next(pit);
    if (pit->wheelSide == WHEEL_RIGHT &&
        npit->wheelSide == WHEEL_LEFT)
    {
      dists.push_back(
        npit->peakPtr->getIndex() - pit->peakPtr->getIndex());
    }
  }

  // Look for inter-car gaps.
  sort(dists.begin(), dists.end());
  unsigned shortGapLower, shortGapUpper;
  unsigned shcount;
  PeakDetect::findFirstSize(dists, shortGapLower, shortGapUpper,
    shcount, 0);
  // Could be zero
cout << "Guessing short gap " << shortGapLower << "-" <<
  shortGapUpper << endl;

  // Tentatively mark short gaps (between cars).

  for (auto pit = peaksAnnot.begin(); pit != prev(peaksAnnot.end());
    pit++)
  {
    auto npit = next(pit);
    if (pit->wheelSide != WHEEL_RIGHT || npit->wheelSide != WHEEL_LEFT)
      continue;

    const unsigned dist = 
      npit->peakPtr->getIndex() - pit->peakPtr->getIndex();
    if (dist >= shortGapLower && dist <= shortGapUpper)
    {
      pit->bogeySide = BOGEY_RIGHT;
      npit->bogeySide = BOGEY_LEFT;
cout << "Marking car gap at " << pit->peakPtr->getIndex()+offset << "-" <<
  npit->peakPtr->getIndex()+offset << endl;
    }
  }


  // Look for unpaired short gaps.
  // TODO There could be a spurious peak in between.

  for (auto pit = peaksAnnot.begin(); pit != prev(peaksAnnot.end());
    pit++)
  {
    auto npit = next(pit);
    if (pit->wheelSide == WHEEL_RIGHT && npit->wheelSide != WHEEL_LEFT)
    {
      const unsigned dist = 
        npit->peakPtr->getIndex() - pit->peakPtr->getIndex();

      if (dist < shortGapLower || dist > shortGapUpper)
        continue;

      if (npit->qualityShape <= 0.3f)
      {
cout << "Adding " <<
  npit->peakPtr->getIndex()+offset << 
  " as short-gap left partner to " << 
  pit->peakPtr->getIndex()+offset << endl;
        
        npit->bogeySide = BOGEY_LEFT;
      }
    }
    else if (pit->wheelSide != WHEEL_RIGHT && npit->wheelSide == WHEEL_LEFT)
    {
      const unsigned dist = 
        npit->peakPtr->getIndex() - pit->peakPtr->getIndex();

      if (dist < shortGapLower || dist > shortGapUpper)
        continue;

      if (pit->qualityShape <= 0.3f)
      {
cout << "Adding " <<
  pit->peakPtr->getIndex()+offset << 
  " as short-gap right partner to " << 
  npit->peakPtr->getIndex()+offset << endl;
        
        npit->bogeySide = BOGEY_RIGHT;
      }
    }
  }

  // Look for smallest large gaps.
  dists.clear();
  for (auto pit = peaksAnnot.begin(); pit != prev(peaksAnnot.end());
    pit++)
  {
    if (pit->wheelSide != WHEEL_RIGHT || pit->bogeySide != BOGEY_SIZE)
      continue;

    auto npit = next(pit);
    while (npit != peaksAnnot.end() && 
        npit->wheelSide != WHEEL_LEFT &&
        npit->wheelSide != WHEEL_RIGHT)
    {
      npit = next(npit);
    }

    if (npit == peaksAnnot.end() || 
        npit->wheelSide == WHEEL_RIGHT)
      continue;

    dists.push_back(
      npit->peakPtr->getIndex() - pit->peakPtr->getIndex());
  }

  vector<CarStat> carStats;
  vector<Car> cars;

  // Look for intra-car gaps.

  sort(dists.begin(), dists.end());
  unsigned longGapLower, longGapUpper;
  unsigned lcount;
  PeakDetect::findFirstSize(dists, longGapLower, longGapUpper,
    lcount, shcount / 2);
  // Could be zero
cout << "Guessing long gap " << longGapLower << "-" <<
  longGapUpper << endl;

  // Label intra-car gaps.

  carStats.emplace_back(CarStat());
  CarStat& cs = carStats.back();

  for (auto pit = peaksAnnot.begin(); pit != prev(peaksAnnot.end());
    pit++)
  {
    if (pit->wheelSide != WHEEL_RIGHT || pit->bogeySide != BOGEY_SIZE)
      continue;

    auto npit = next(pit);
    while (npit != peaksAnnot.end() && 
        npit->wheelSide != WHEEL_LEFT &&
        npit->wheelSide != WHEEL_RIGHT)
    {
      npit = next(npit);
    }

    if (npit == peaksAnnot.end() || 
        npit->wheelSide == WHEEL_RIGHT)
      continue;

    const unsigned dist = 
      npit->peakPtr->getIndex() - pit->peakPtr->getIndex();

    if (dist < longGapLower || dist > longGapUpper)
      continue;

    pit->bogeySide = BOGEY_LEFT;
    npit->bogeySide = BOGEY_RIGHT;
cout << "\nMarking long gap at " << pit->peakPtr->getIndex()+offset << "-" <<
  npit->peakPtr->getIndex()+offset << endl;

    for (auto it = next(pit); it != npit; it++)
      it->spuriousFlag = true;
      
    // Fill out cars.
    cars.emplace_back(Car());
    Car& car = cars.back();

    car.firstBogeyRight = &*pit->peakPtr;
    car.firstBogeyLeft = pit->prevLargePeakPtr;
    car.secondBogeyLeft = &*npit->peakPtr;
    car.secondBogeyRight = npit->nextLargePeakPtr;

    car.midGap = dist;

    car.leftBogeyGap = car.firstBogeyRight->getIndex() -
      car.firstBogeyLeft->getIndex();
    car.rightBogeyGap = car.secondBogeyRight->getIndex() -
      car.secondBogeyLeft->getIndex();

    car.catStatIndex = 0;

    auto ppit = prev(pit);
    Peak * p0 = ppit->prevLargePeakPtr;
    if (p0 != nullptr && ppit->bogeySide != BOGEY_SIZE)
    {
      const unsigned b = car.firstBogeyLeft->getIndex();
      const unsigned d = b - p0->getIndex();

      car.start = p0->getIndex() + (d/2);
      car.leftGapSet = true;
      car.leftGap = b - car.start;
    }
    else
      car.leftGapSet = false;

    auto nnpit = next(npit);
    pit = nnpit; // Advance across the peaks that we've marked

    Peak * p1 = nnpit->nextLargePeakPtr;
    if (p1 != nullptr && nnpit->bogeySide != BOGEY_SIZE)
    {
      const unsigned e = car.secondBogeyRight->getIndex();
      const unsigned d = p1->getIndex() - e;

      car.end = e + (d/2);
      car.rightGapSet = true;
      car.rightGap = car.end - e;
    }
    else
      car.rightGapSet = false;

    car.partialFlag = false;

cout << "Marking car:\n";
cout << "start " << car.start+offset << ", end " << car.end+offset << endl;
cout << "Wheels " << car.firstBogeyLeft->getIndex()+offset << ", " <<
  car.firstBogeyRight->getIndex()+offset << ", " <<
  car.secondBogeyLeft->getIndex()+offset << ", " <<
  car.secondBogeyRight->getIndex()+offset << endl;

    // Fill out carStats[0].

/*
    cs.carSum += car;
    cs.count++;

    cs.firstBogeyLeftSum += * car.firstBogeyLeft;
    cs.firstBogeyRightSum += * car.firstBogeyRight;
    cs.secondBogeyLeftSum += * car.secondBogeyLeft;
    cs.secondBogeyRightSum += * car.secondBogeyRight;
*/
    cs += car;

    cs.numWheels = 2;
    if (car.leftBogeyGap > 0)
      cs.numWheels++;
    if (car.rightBogeyGap > 0)
      cs.numWheels++;
  }

unsigned wcount = 0;
  for (auto& p: peaksAnnot)
  {
    if (p.wheelFlag)
    {
      wcount++;
    }
  }
cout << "Counting " << wcount << " peaks" << endl << endl;


  // Fill out interval ends.
  carStats[0].avg();

  for (auto& car: cars)
  {
    if (! car.leftGapSet)
    {
      // TODO Could be nullptr.  The first real one.
      if (carStats[0].carAvg.leftGap <= car.firstBogeyLeft->getIndex())
      {
        car.start = car.firstBogeyLeft->getIndex() - 
          carStats[0].carAvg.leftGap;
        car.leftGap = carStats[0].carAvg.leftGap;
cout << "Filling out car left:\n";
cout << "start " << car.start+offset << ", end " << car.end+offset << endl;
      }
    }

    if (! car.rightGapSet)
    {
      car.end = car.secondBogeyRight->getIndex() +
        carStats[0].carAvg.rightGap;
        car.rightGap = carStats[0].carAvg.rightGap;
cout << "Filling out car right:\n";
cout << "start " << car.start+offset << ", end " << car.end+offset << endl;
    }
  }

  // Print cars in tight table.
  cout << "Cars before intra-gaps:\n";
  cout << cars[0].strHeader();
  for (auto& car: cars)
    cout << car.strLine(offset);
  cout << endl;


  // Check open intervals.  Start with inner ones as they are complete.

  // TODO cars change in the inner loop, not so pretty.
if (cars.size() == 0)
  THROW(ERR_NO_PEAKS, "No cars?");

  const unsigned u1 = cars.back().end;

  const unsigned u2 = peaksAnnot.back().peakPtr->getIndex();
  const unsigned csize = cars.size();

  for (unsigned ii = 0; ii+1 < csize; ii++)
  {
    if (cars[ii].end != cars[ii+1].start)
    {
      if (! PeakDetect::findCars(cars[ii].end, cars[ii+1].start, 
        true, true, peaksAnnot, cars, carStats))
      {
        cout << "Couldn't understand intra-gap " <<
          cars[ii].end+offset << "-" << cars[ii+1].start+offset << endl;
      }
      else
      {
cout << "Did intra-gap " << cars[ii].end+offset << "-" << 
  cars[ii+1].start+offset << endl;
      }
    }
  }
wcount = 0;
  for (auto& p: peaksAnnot)
  {
    if (p.wheelFlag)
      wcount++;
  }
cout << "Counting " << wcount << " peaks" << endl;

  cout << "Cars after inner gaps:\n";
  cout << cars[0].strHeader();
  for (auto& car: cars)
    cout << car.strLine(offset);
  cout << endl;

  cout << "After inner gaps\n";
  for (auto& csl: carStats)
  {
    csl.avg();
    cout << "Car stats:\n";
    cout << csl.str() << "\n";
  }

  if (u2 > u1)
  {
    if (! PeakDetect::findCars(u1, u2,
      true, false, peaksAnnot, cars, carStats))
    {
      cout << "Couldn't understand trailing gap " <<
        u1+offset << "-" << u2+offset << endl;
    }
    else
    {
      cout << "Did trailing gap " <<
        u1+offset << "-" << u2+offset << endl;
    }
  }
wcount = 0;
  for (auto& p: peaksAnnot)
  {
    if (p.wheelFlag)
      wcount++;
  }
cout << "Counting " << wcount << " peaks" << endl;


  cout << "After trailing gap\n";
  for (auto& csl: carStats)
  {
    csl.avg();
    cout << "Car stats:\n";
    cout << csl.str() << "\n";
  }


  const unsigned u3 = peaksAnnot.front().peakPtr->getIndex();
  const unsigned u4 = cars.front().start;

  if (u3 < u4)
  {
    if (! PeakDetect::findCars(u3, u4,
      false, true, peaksAnnot, cars, carStats))
    {
      cout << "Couldn't understand leading gap " <<
        u3+offset << "-" << u4+offset << endl;
    }
    else
    {
      cout << "Did leading gap " <<
        u3+offset << "-" << u4+offset << endl;
    }
  }

  cout << "After leading gap\n";
  for (auto& csl: carStats)
  {
    csl.avg();
    cout << "Car stats:\n";
    cout << csl.str() << "\n";
  }


  // TODO Check peak quality and deviations in carStats[0].
  // Detect inner and open intervals that are not done.
  // Could be carStats[0] with the right length etc, but missing peaks.
  // Or could be a new type of car (or multiple cars).
  // Ends come later.

  // Put peaks in the global list.
  peaksNewer.clear();
wcount = 0;
  for (auto& p: peaksAnnot)
  {
    // if (p.tallFlag)
    if (p.wheelFlag)
    {
wcount++;
      peaksNewer.push_back(* p.peakPtr);
    }
  }
cout << "Returning " << wcount << " peaks" << endl;

}


void PeakDetect::reduce()
{
if (peaks.size() == 0)
  return;

  const bool debug = true;
  const bool debugDetails = false;

  if (debugDetails)
  {
    cout << "Original peaks: " << peaks.size() << "\n";
    PeakDetect::print();
  }

  PeakDetect::reduceSmallAreas(0.1f);
  if (debugDetails)
  {
    cout << "Non-tiny list peaks: " << peaks.size() << "\n";
    PeakDetect::print();
  }


  PeakDetect::eliminateKinks();
  if (debugDetails)
  {
    cout << "Non-kinky list peaks: " << peaks.size() << "\n";
    PeakDetect::print();
  }

  PeakDetect::estimateScales();
  if (debug)
  {
    cout << "Scale list\n";
    cout << scalesList.str(0) << endl;
  }

  peaksNew = peaks;
  PeakDetect::setOrigPointers();

  PeakDetect::reduceSmallRanges(scalesList.getRange() / 10.f);

  PeakDetect::markPossibleQuiet();

PeakDetect::reduceNewer();

peaks = peaksNewer;

  bool firstSeen = false;
  unsigned firstTentativeIndex = peaks.front().getIndex();
  unsigned lastTentativeIndex = peaks.back().getIndex();
  numCandidates = 0;
  numTentatives = 0;

  for (auto& peak: peaks)
  {
    if (! peak.isCandidate())
      continue;

    numCandidates++;

    PeakType ptype = PEAK_TENTATIVE; // Only type now

    if (ptype == PEAK_TENTATIVE)
    {
      numTentatives++;
      peak.select();
      if (! firstSeen)
      {
        firstTentativeIndex = peak.getIndex();
        firstSeen = true;
      }
      lastTentativeIndex = peak.getIndex();
    }

    peak.logType(ptype);
  }

  // Fix the early and late peaks to different types.
  for (auto& peak: peaks)
  {
    if (! peak.isCandidate() || 
        peak.getType() == PEAK_TENTATIVE)
      continue;

    if (peak.getIndex() < firstTentativeIndex)
      peak.logType(PEAK_TRANS_FRONT);
    else if (peak.getIndex() > lastTentativeIndex)
      peak.logType(PEAK_TRANS_BACK);
  }

  cout << numCandidates << " candidate peaks" << endl;
  if (numTentatives == 0)
    THROW(ERR_NO_PEAKS, "No tentative peaks");

}


void PeakDetect::logPeakStats(
  const vector<PeakPos>& posTrue,
  const string& trainTrue,
  const double speedTrue,
  PeakStats& peakStats)
{
  const bool debug = true;
  const bool debugDetails = false;

  const unsigned lt = posTrue.size();

  // Scale true positions to true times.
  vector<PeakTime> timesTrue;
  timesTrue.resize(lt);
  PeakDetect::pos2time(posTrue, speedTrue, timesTrue);

  if (debugDetails)
    PeakDetect::printPeaks(timesTrue);

  // Find a good line-up.
  double shift = 0.;
  if (! PeakDetect::findMatch(timesTrue, shift))
  {
    if (debug)
    {
      cout << "No good match to real " << posTrue.size() << " peaks.\n";
      cout << "\nTrue train " << trainTrue << " at " << 
        fixed << setprecision(2) << speedTrue << " m/s" << endl << endl;
    }
    return;
  }

  // Make statistics.
  vector<unsigned> seenTrue(posTrue.size(), 0);
  unsigned seen = 0;
  for (auto& peak: peaks)
  {
    if (peak.isCandidate())
    {
      const int m = peak.getMatch();
      const PeakType pt = peak.getType();
      if (m >= 0)
      {
if (m >= static_cast<int>(posTrue.size()))
  cout << "HOPPLA5" << endl;
        peakStats.logSeenMatch(static_cast<unsigned>(m), lt, pt);
        if (seenTrue[m] && debug)
          cout << "Already saw true peak " << m << endl;
        seenTrue[m]++;
        seen++;
      }
      else
        peakStats.logSeenMiss(pt);
    }
  }

  if (debug)
  {
    cout << "True train " << trainTrue << " at " << 
      fixed << setprecision(2) << speedTrue << " m/s\n\n";

    cout << seen << " of the observed peaks are close to true ones (" << 
      posTrue.size() << " true peaks)" << endl;
  }

  for (unsigned m = 0; m < posTrue.size(); m++)
  {
    if (seenTrue[m])
      continue;

    // Look for posTrue[m].time vs (peaks time - shift),
    // must be within TIME_PROXIMITY.
    PeakType ctype = PeakDetect::findCandidate(timesTrue[m].time, shift);
    if (ctype == PEAK_TENTATIVE)
    {
      // TODO Should either not happen, or be thrown.
      cout << "Odd: Tentative matched again\n";
    }

    // The missed ones are detected as early/late/missing.
    if (ctype == PEAK_TRUE_MISSING)
      peakStats.logTrueReverseMiss(m, lt);
    else
      peakStats.logTrueReverseMatch(m, lt, ctype);
  }
}


void PeakDetect::makeSynthPeaks(vector<float>& synthPeaks) const
{
  for (unsigned i = 0; i < synthPeaks.size(); i++)
    synthPeaks[i] = 0;

  // PeakDetect::makeSynthPeaksSharp(synthPeaks);

// PeakDetect::makeSynthPeaksQuietNew(synthPeaks);

  // PeakDetect::makeSynthPeaksLines(synthPeaks);

  // PeakDetect::makeSynthPeaksQuiet(synthPeaks);

  // PeakDetect::makeSynthPeaksLines(synthPeaks);

  // PeakDetect::makeSynthPeaksClassical(synthPeaks);

PeakDetect::makeSynthPeaksClassicalNewer(synthPeaks);
}


void PeakDetect::makeSynthPeaksQuietNew(vector<float>& synthPeaks) const
{
  for (auto& qc: quietCandidates)
  {
    if (qc.clusterNo != quietFavorite)
      continue;

    for (unsigned i = qc.start; i <= qc.start + qc.len; i++)
      synthPeaks[i] = qc.minLevel;
  }
}


void PeakDetect::makeSynthPeaksQuiet(vector<float>& synthPeaks) const
{
  unsigned index1 = 0;
  float value1 = 0.f;
  bool activeFlag = false;

  for (auto peak = peaksNew.begin(); peak != peaksNew.end(); peak++)
  {
    if (activeFlag)
    {
      if (peak->getQuietBegin())
        THROW(ERR_ALGO_PEAK_INTERVALS, "Two begins");
      else if (peak->getQuietEnd())
      {
        activeFlag = false;

        const unsigned index2 = peak->getIndex();
        const float value2 = peak->getValue();
        const float grad = (value2 - value1) / (index2 - index1);

        for (unsigned i = index1; i <= index2; i++)
          synthPeaks[i] = value1 + grad * (i-index1);
      }
    }
    else if (peak->getQuietEnd())
        THROW(ERR_ALGO_PEAK_INTERVALS, "Two ends");
    else if (peak->getQuietBegin())
    {
      activeFlag = true;
      index1 = peak->getIndex();
      value1 = peak->getValue();
    }
  }
}


void PeakDetect::makeSynthPeaksSharp(vector<float>& synthPeaks) const
{
  // Draw lines from sharp peaks.

  for (auto peak = peaksNew.begin(); peak != peaksNew.end(); peak++)
  {
    if (! peak->getSharp())
      continue;

    // Otherwise it will be an interior peak.
    auto peakPrev = prev(peak);
    auto peakNext = next(peak);

    unsigned index1 = peak->getIndex();
    unsigned index2 = peakNext->getIndex();
    float value1 = peak->getValue();
    float value2 = peakNext->getValue();
    float grad = (value2 - value1) / (index2 - index1);

    // Put zeroes at the peaks themselves.
    for (unsigned i = index1+1; i < index2; i++)
      synthPeaks[i] = value1 + grad * (i-index1);

    index1 = peakPrev->getIndex();
    index2 = peak->getIndex();
    value1 = peakPrev->getValue();
    value2 = peak->getValue();
    grad = (value2 - value1) / (index2 - index1);

    // Put zeroes at the peaks themselves.
    for (unsigned i = index1+1; i < index2; i++)
      synthPeaks[i] = value1 + grad * (i-index1);
  }
}


void PeakDetect::makeSynthPeaksLines(vector<float>& synthPeaks) const
{
  // Draw lines between all peaks.

  for (auto peak = peaksNew.begin(); peak != peaksNew.end(); peak++)
  {
    const auto peakNext = next(peak);
    if (peakNext == peaksNew.end())
     break;

    const float value1 = peak->getValue();
    const float value2 = peakNext->getValue();

    unsigned index1 = peak->getIndex();
    unsigned index2 = peakNext->getIndex();
    const float grad = (value2 - value1) / (index2 - index1);

    // Put zeroes at the peaks themselves.
    for (unsigned i = index1+1; i < index2; i++)
      synthPeaks[i] = value1 + grad * (i-index1);
  }
}


void PeakDetect::makeSynthPeaksPosLines(vector<float>& synthPeaks) const
{
  // Show lines whenever a positive maximum is involved.

  for (auto peak = peaksNew.begin(); peak != peaksNew.end(); peak++)
  {
    const auto peakNext = next(peak);
    if (peakNext == peaksNew.end())
     break;

    const float value1 = peak->getValue();
    const float value2 = peakNext->getValue();
    if (value1 < 0.f && value2 < 0.f)
      continue;

    unsigned index1 = peak->getIndex();
    unsigned index2 = peakNext->getIndex();
    const float grad = (value2 - value1) / (index2 - index1);

    // Only put zeroes at positive maxima.
    unsigned i1 = (peak->getValue() >= 0.f ? index1+1 : index1);
    if (peakNext->getValue() >= 0.f)
      index2--;

    for (unsigned i = i1; i <= index2; i++)
      synthPeaks[i] = value1 + grad * (i-index1);
  }
}


void PeakDetect::makeSynthPeaksClassical(vector<float>& synthPeaks) const
{
  for (auto& peak: peaks)
  {
    if (peak.isSelected())
      synthPeaks[peak.getIndex()] = peak.getValue();
  }
}


void PeakDetect::makeSynthPeaksClassicalNewer(vector<float>& synthPeaks) const
{
  for (auto& peak: peaksNewer)
  {
// cout << "Setting " << peak.getIndex() << " to " << peak.getValue() << endl;
    synthPeaks[peak.getIndex()] = peak.getValue();
  }
}


float PeakDetect::getFirstPeakTime() const
{
  for (auto& peak: peaks)
  {
    if (peak.isSelected())
      return peak.getIndex() / static_cast<float>(SAMPLE_RATE);
  }

  THROW(ERR_NO_PEAKS, "No output peaks");
}


void PeakDetect::getPeakTimes(vector<PeakTime>& times) const
{
  times.clear();
  const float t0 = PeakDetect::getFirstPeakTime();

  for (auto& peak: peaks)
  {
    if (peak.isSelected())
    {
      times.emplace_back(PeakTime());
      PeakTime& p = times.back();
      p.time = peak.getIndex() / SAMPLE_RATE - t0;
      p.value = peak.getValue();
    }
  }
}


void PeakDetect::print() const
{
  cout << peaks.front().strHeader();

  for (auto& peak: peaks)
    cout << peak.str(offset);
  cout << "\n";
}


void PeakDetect::printPeaks(const vector<PeakTime>& timesTrue) const
{
  cout << "true\n";
  for (unsigned i = 0; i < timesTrue.size(); i++)
    cout << i << ";" << 
      fixed << setprecision(6) << timesTrue[i].time << "\n";

  cout << "\nseen\n";
  unsigned pp = 0;
  for (auto& peak: peaks)
  {
    if (peak.isCandidate() && peak.getType() == PEAK_TENTATIVE)
    {
      cout << pp << ";" << 
        fixed << setprecision(6) << peak.getTime() << endl;
      pp++;
    }
  }
  cout << "\n";
}

