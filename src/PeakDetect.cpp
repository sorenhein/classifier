#include <list>
#include <iostream>
#include <iomanip>
#include <functional>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakDetect.h"
#include "PeakSeeds.h"
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
  offset = 0;
  peaks.clear();
  models.reset();
  scales.reset();
  scalesList.reset();
  numCandidates = 0;
  numTentatives = 0;
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
  if (peak1 == peak2)
    return peak1;

  Peak * peak0 = 
    (peak1 == peaks.begin() ? nullptr : &*prev(peak1));
  Peak * peakN = 
    (next(peak2) == peaks.end() ? nullptr : &*next(peak2));

  peak2->update(peak0, peakN);

  return peaks.erase(peak1, peak2);
}


void PeakDetect::reduceSmallRanges(
  const float rangeLimit,
  const bool preserveFlag)
{
  if (peaks.empty())
    THROW(ERR_NO_PEAKS, "Peak list is empty");

  const auto peakLast = prev(peaks.end());
  auto peak = next(peaks.begin());

  while (peak != peaks.end())
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
      if (peak == peaks.end())
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
      if (peakCurrent != peaks.end())
        peaks.erase(peakCurrent, peaks.end());
      break;
    }
/* */
    else if (preserveFlag &&
        peakCurrent->getValue() > 0.f &&
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
  for (auto peak = peaks.begin(), peakNew = peaks.begin(); 
      peak != peaks.end(); peak++, peakNew++)
    peakNew->logOrigPointer(&*peak);
}


void PeakDetect::markPossibleQuiet()
{
  quietCandidates.clear();

  for (auto peak = peaks.begin(); peak != peaks.end(); peak++)
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
      if (peakPrev == peaks.begin())
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
        peakPrev != peaks.begin() && vPrev < value)
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
      if (peakNext == peaks.end())
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
        peakNext != peaks.end() && vNext < value)
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


bool PeakDetect::matchesModel(
  const CarDetect& car, 
  unsigned& index) const
{
  float distance;
  if (! models.findClosest(car, distance, index))
    return false;
  else
    return (distance <= 1.5f);
}


bool PeakDetect::findFourWheeler(
  const unsigned start,
  const unsigned end,
  const bool leftGapPresent,
  const bool rightGapPresent,
  const vector<PeakEntry>& peaksAnnot,
  const vector<unsigned>& peakNos, 
  const vector<unsigned>& peakIndices, 
  CarDetect& car) const
{
  car.setLimits(start, end);

  if (leftGapPresent)
    car.logLeftGap(peakNos[0] - start);

  if (rightGapPresent)
    car.logRightGap(end - peakNos[3]);

  car.logCore(
    peakNos[1] - peakNos[0],
    peakNos[2] - peakNos[1],
    peakNos[3] - peakNos[2]);

  car.logPeakPointers(
    peaksAnnot[peakIndices[0]].peakPtr,
    peaksAnnot[peakIndices[1]].peakPtr,
    peaksAnnot[peakIndices[2]].peakPtr,
    peaksAnnot[peakIndices[3]].peakPtr);

  return models.gapsPlausible(car);
}


bool PeakDetect::findLastTwoOfFourWheeler(
  const unsigned start, 
  const unsigned end,
  const bool rightGapPresent,
  const vector<PeakEntry>& peaksAnnot, 
  const vector<unsigned>& peakNos, 
  const vector<unsigned>& peakIndices,
  CarDetect& car) const
{
  car.setLimits(start, end);

  if (rightGapPresent)
    car.logRightGap(end - peakNos[1]);

  car.logCore(0, 0, peakNos[1] - peakNos[0]);

  car.logPeakPointers(
    nullptr,
    nullptr,
    peaksAnnot[peakIndices[0]].peakPtr,
    peaksAnnot[peakIndices[1]].peakPtr);

  if (! models.sideGapsPlausible(car))
    return false;

  // As we don't have a complete car, we'll at least require the
  // right bogey gap to be similar to something we've seen.

  if (models.rightBogeyPlausible(car))
    return true;
  else
  {
    cout << "Error: Suspect right bogey gap: ";
    cout << car.strGaps() << endl;
    cout << "Checked against " << models.size() << " ref cars\n";
    return false;
  }
}


bool PeakDetect::findLastThreeOfFourWheeler(
  const unsigned start, 
  const unsigned end,
  const vector<PeakEntry>& peaksAnnot, 
  const vector<unsigned>& peakNos, 
  const vector<unsigned>& peakIndices,
  CarDetect& car,
  unsigned& numWheels) const
{
  car.setLimits(start, end);

  car.logCore(
    0, 
    peakNos[1] - peakNos[0],
    peakNos[2] - peakNos[1]);

  car.logRightGap(end - peakNos[2]);

  car.logPeakPointers(
    nullptr,
    peaksAnnot[peakIndices[0]].peakPtr,
    peaksAnnot[peakIndices[1]].peakPtr,
    peaksAnnot[peakIndices[2]].peakPtr);

  if (! models.sideGapsPlausible(car))
    return false;

  // As we don't have a complete car, we'll at least require the
  // right bogey gap to be similar to something we've seen.

  if (! models.rightBogeyPlausible(car))
  {
    cout << "Error: Suspect right bogey gap: ";
    cout << car.strGaps() << endl;
    cout << "Checked against " << models.size() << " ref cars\n";
    return false;
  }

  if (! car.midGapPlausible())
  {
    // Drop the first peak.
    numWheels = 2;
    car.logPeakPointers(
      nullptr,
      nullptr,
      peaksAnnot[peakIndices[1]].peakPtr,
      peaksAnnot[peakIndices[2]].peakPtr);

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
  vector<CarDetect>& cars,
  CarDetect& car)
{
  unsigned index;

  if (! car.isPartial() &&
      PeakDetect::matchesModel(car, index))
  {
    car.logStatIndex(index);

    models.add(car, index);
cout << "Recognized car " << index << endl;
  }
  else
  {
    car.logStatIndex(models.size());

    models.append();
    models += car;
cout << "Created new car\n";
  }

  cars.push_back(car);
}


bool PeakDetect::findCars(
  const unsigned start,
  const unsigned end,
  const bool leftGapPresent,
  const bool rightGapPresent,
  vector<PeakEntry>& peaksAnnot,
  vector<CarDetect>& cars)
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

  unsigned startLocal = start;
  unsigned endLocal = end;
  bool leftFlagLocal = leftGapPresent;
  bool rightFlagLocal = rightGapPresent;

  unsigned np = peakNos.size();
  bool firstRightFlag = true;
  const unsigned firstIndex = peakIndices.front();
  const unsigned npOrig = np;
cout << "Starting out with " << np << " peaks\n";

  while (np >= 8)
  {
    // Might be multiple cars.  Work backwards, as this works best
    // for the leading cars.  Use groups of four good peaks.
    unsigned i = np;
    unsigned seen = 0;
    while (i > 0 && seen < 4)
    {
      i--;
      if (peaksAnnot[peakIndices[i]].qualityShape <= 0.5f)
        seen++;
    }

    if (seen == 4)
    {
      vector<unsigned> peakNosNew, peakIndicesNew;
      for (unsigned j = i; j < np; j++)
      {
        if (peaksAnnot[peakIndices[j]].qualityShape <= 0.5f)
        {
          peakNosNew.push_back(peakNos[j]);
          peakIndicesNew.push_back(peakIndices[j]);
        }

        peakNos.pop_back();
        peakIndices.pop_back();
      }
cout << "Trying a multi-peak split, np was " << np << ", is " << i << endl;
cout << "sizes " << peakNosNew.size() << ", " << peakIndicesNew.size() <<
  ", " << peakNos.size() << ", " << peakIndices.size() << endl;
      np = i;

      if (peakIndicesNew.front() == peakIndices.front())
      {
        startLocal = start;
        leftFlagLocal = leftGapPresent;
      }
      else
      {
        startLocal = peaksAnnot[peakIndicesNew[0]].peakPtr->getIndex();
        leftFlagLocal = false;
      }

      if (firstRightFlag)
      {
        endLocal = end;
        rightFlagLocal = rightGapPresent;
        firstRightFlag = false;
      }
      else
      {
        endLocal = peaksAnnot[peakIndicesNew[3]].peakPtr->getIndex();
        rightFlagLocal = false;
      }
cout << "Range is " << startLocal+offset << "-" << 
  endLocal+offset << endl;

      CarDetect car;
      if (PeakDetect::findFourWheeler(startLocal, endLocal,
        leftFlagLocal, rightFlagLocal,
        peaksAnnot, peakNosNew, peakIndicesNew, car))
      {
        PeakDetect::fixFourWheels(peaksAnnot, peakIndicesNew, 0, 1, 2, 3);
        PeakDetect::updateCars(cars, car);
        // Then go on
      }
      else
      {
cout << "Failed multi-peak split " << start+offset << "-" << end+offset << endl;
        return false;
      }
    }
    else
    {
      // Fall through to the other chances.
cout << "Falling through with remaining peaks, np " << np << endl;
      break;
    }
  }

  if (npOrig > np)
  {
    // Kludge
    if (peakIndices.size() == 0)
      THROW(ERR_NO_PEAKS, "No residual peaks?");

        startLocal = start;
        leftFlagLocal = leftGapPresent;

        endLocal = peaksAnnot[peakIndices.back()].peakPtr->getIndex();
        rightFlagLocal = false;

cout << "Fell through to " << startLocal+offset << "-" << endLocal+offset << endl;

  notTallCount = 0;
  notTallNos.clear();
  notTallFlag = false;
  peakNos.clear();
  peakIndices.clear();

  for (unsigned i = 0; i < peaksAnnot.size(); i++)
  {
    const unsigned index = peaksAnnot[i].peakPtr->getIndex();
    if (index >= startLocal && index <= endLocal)
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

  np = peakNos.size();


  }


  if (np <= 2)
  {
    cout << "Don't know how to do this yet: " << np << "\n";
    return false;
  }

  if (! leftFlagLocal &&
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

    CarDetect car;
    unsigned numWheels;
    if (! PeakDetect::findLastThreeOfFourWheeler(startLocal, endLocal,
        peaksAnnot, peakNos, peakIndices, car, numWheels))
      return false;

    if (numWheels == 2)
      PeakDetect::fixTwoWheels(peaksAnnot, peakIndices, 1, 2);
    else 
      PeakDetect::fixThreeWheels(peaksAnnot, peakIndices, 0, 1, 2);

    PeakDetect::updateCars(cars, car);

    return true;
  }

  if (np == 4)
  {
    CarDetect car;
    if (PeakDetect::findFourWheeler(startLocal, endLocal,
        leftFlagLocal, rightFlagLocal,
        peaksAnnot, peakNos, peakIndices, car))
    {
      PeakDetect::fixFourWheels(peaksAnnot, peakIndices, 0, 1, 2, 3);

      PeakDetect::updateCars(cars, car);

      return true;
    }
    
    // Give up unless this is the first car.
    if (leftFlagLocal)
      return false;

    // Try again without the first peak.
cout << "Trying again without the very first peak of first car\n";
    peakNos.erase(peakNos.begin());
    peakIndices.erase(peakIndices.begin());

    unsigned numWheels;
    if (! PeakDetect::findLastThreeOfFourWheeler(startLocal, endLocal,
        peaksAnnot, peakNos, peakIndices, car, numWheels))
      return false;

    if (numWheels == 2)
      PeakDetect::fixTwoWheels(peaksAnnot, peakIndices, 1, 2);
    else 
      PeakDetect::fixThreeWheels(peaksAnnot, peakIndices, 0, 1, 2);

    PeakDetect::updateCars(cars, car);

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
      
        CarDetect car;
    cout << "Trying the car anyway\n";
        if (! PeakDetect::findFourWheeler(startLocal, endLocal,
            leftFlagLocal, rightFlagLocal,
            peaksAnnot, peakNos, peakIndices, car))
        {
    cout << "Failed the car: " << np << "\n";
          return false;
        }

        PeakDetect::fixFourWheels(peaksAnnot, peakIndices, 0, 1, 2, 3);
        PeakDetect::updateCars(cars, car);
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

    CarDetect car;
    // Try the car.
cout << "Trying the car\n";
    if (! PeakDetect::findFourWheeler(startLocal, endLocal,
        leftFlagLocal, rightFlagLocal,
        peaksAnnot, peakNosNew, peakIndicesNew, car))
    {
cout << "Failed the car: " << np << "\n";
      return false;
    }

    PeakDetect::fixFourWheels(peaksAnnot, peakIndicesNew, 0, 1, 2, 3);
    PeakDetect::updateCars(cars, car);
    return true;
  }

  if (! leftFlagLocal && np == 3)
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

      CarDetect car;
      if (! PeakDetect::findLastTwoOfFourWheeler(startLocal, endLocal,
          rightFlagLocal,
          peaksAnnot, peakNos, peakIndices, car))
      {
cout << "Failed\n";
        return false;
      }

      PeakDetect::fixTwoWheels(peaksAnnot, peakIndices, 0, 1);

      PeakDetect::updateCars(cars, car);

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

      CarDetect car;
      if (! PeakDetect::findLastTwoOfFourWheeler(startLocal, endLocal,
          rightFlagLocal,
          peaksAnnot, peakNos, peakIndices, car))
      {
cout << "Failed\n";
        return false;
      }

      PeakDetect::fixTwoWheels(peaksAnnot, peakIndices, 0, 1);

      PeakDetect::updateCars(cars, car);

      return true;
    }
  }

  cout << "Don't know how to do this yet: " << np << ", notTallCount " <<
    notTallCount << "\n";
  return false;

}


unsigned PeakDetect::countWheels(const vector<PeakEntry>& peaksAnnot) const
{
  unsigned count = 0;
  for (auto& p: peaksAnnot)
  {
    if (p.wheelFlag)
      count++;
  }
  return count;
}


void PeakDetect::printCarStats(const string& text) const
{
  cout << "Car stats " << text << "\n";
  cout << models.str() << endl;
}


void PeakDetect::printCars(
  const vector<CarDetect>& cars,
  const string& text) const
{
  cout << "Cars " << text << "\n";
  cout << cars[0].strHeaderFull();
  for (auto& car: cars)
    cout << car.strFull(offset);
  cout << endl;
}


bool PeakDetect::bothTall(
  const PeakEntry& pe1,
  const PeakEntry& pe2) const
{
  return (pe1.tallFlag && pe2.tallFlag);
}


bool PeakDetect::areBogeyGap(
  const PeakEntry& pe1,
  const PeakEntry& pe2) const
{
  return (pe1.wheelSide == WHEEL_RIGHT && pe2.wheelSide == WHEEL_LEFT);
}


void PeakDetect::guessDistance(
  const vector<PeakEntry>& peaksAnnot, 
  const PeakFncPtr fptr,
  unsigned& distLower, 
  unsigned& distUpper, 
  unsigned& count) const
{
  // Make list of distances between neighbors for which fptr
  // evaluates to true.

  vector<unsigned> dists;
  for (auto pit = peaksAnnot.begin(); pit != prev(peaksAnnot.end());
    pit++)
  {
    auto npit = next(pit);
    if ((this->* fptr)(* pit, * npit))
      dists.push_back(
        npit->peakPtr->getIndex() - pit->peakPtr->getIndex());
  }

  sort(dists.begin(), dists.end());
  PeakDetect::findFirstSize(dists, distLower, distUpper, count);
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

// Try PeakSeeds
PeakSeeds seeds;
seeds.mark(peaks, offset, scalesList.getRange());

  vector<PeakEntry> peaksAnnot;

  // Note which peaks are tall.

  for (auto pit = peaks.begin(); pit != peaks.end(); pit++)
  {
    // Only want the negative minima here.
    if (pit->getValue() >= 0.f || pit->getMaxFlag())
      continue;

    // Exclude tall peaks without a right neighbor.
    auto npit = next(pit);
    if (npit == peaks.end())
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

    pe.tallFlag = pit->isSeed();
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
      if (pa.quality > 0.3f && pa.qualityShape > 0.20f)
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
    else if (pa.qualityShape <= 0.20f)
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

  unsigned wheelDistLower, wheelDistUpper;
  unsigned whcount;

  PeakDetect::guessDistance(peaksAnnot, &PeakDetect::bothTall,
    wheelDistLower, wheelDistUpper, whcount);

cout << "Guessing wheel distance " << wheelDistLower << "-" <<
  wheelDistUpper << endl;

  // Tentatively mark wheel pairs (bogeys).  If there are only 
  // single wheels, we might be marking the wagon gaps instead.

  unsigned numBogeys = 0;
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
        numBogeys++;
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
        numBogeys++;
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
        numBogeys++;
      }
    }
  }

  // Find the average first and second peak in a wheel pair.
  vector<float> values1, 
    rangesLeft1, 
    rangesRight1, 
    gradLeft1, 
    gradRight1;
  vector<float> values2, 
    rangesLeft2, 
    rangesRight2, 
    gradLeft2, 
    gradRight2;
  for (auto& pa: peaksAnnot)
  {
    auto pt = pa.peakPtr;
    auto npt = pa.nextPeakPtr;
    if (npt == nullptr)
      continue;

    if (! pa.tallFlag)
      continue;

    if (! pa.wheelFlag)
      continue;

    if (pa.wheelSide == WHEEL_LEFT)
    {
      values1.push_back(pt->getValue());
      rangesLeft1.push_back(pt->getRange());
      gradLeft1.push_back(10.f * pt->getGradient());

      rangesRight1.push_back(npt->getRange());
      gradRight1.push_back(10.f * npt->getGradient());
    }
    else if (pa.wheelSide == WHEEL_RIGHT)
    {
      values2.push_back(pt->getValue());
      rangesLeft2.push_back(pt->getRange());
      gradLeft2.push_back(10.f * pt->getGradient());

      rangesRight2.push_back(npt->getRange());
      gradRight2.push_back(10.f * npt->getGradient());
    }

  }

  Sharp tallLeftSize, tallRightSize;

  const unsigned nvm1 = values1.size() / 2;
  if (nvm1 == 0)
    THROW(ERR_NO_PEAKS, "No left wheels?");

  nth_element(values1.begin(), values1.begin() + nvm1, values1.end());
  tallLeftSize.level = values1[nvm1];

  nth_element(rangesLeft1.begin(), rangesLeft1.begin() + nvm1, 
    rangesLeft1.end());
  tallLeftSize.range1 = rangesLeft1[nvm1];

  nth_element(gradLeft1.begin(), gradLeft1.begin() + nvm1, gradLeft1.end());
  tallLeftSize.grad1 = gradLeft1[nvm1];

  nth_element(rangesRight1.begin(), rangesRight1.begin() + nvm1, 
    rangesRight1.end());
  tallLeftSize.range2 = rangesRight1[nvm1];

  nth_element(gradRight1.begin(), gradRight1.begin() + nvm1, gradRight1.end());
  tallLeftSize.grad2 = gradRight1[nvm1];

  if (tallLeftSize.range2 != 0.f)
    tallLeftSize.rangeRatio = tallLeftSize.range1 / tallLeftSize.range2;
  if (tallLeftSize.grad2 != 0.f)
    tallLeftSize.gradRatio = tallLeftSize.grad1 / tallLeftSize.grad2;

  // Print scale.
  cout << "Tall left scale" << endl;
  cout << tallLeftSize.strHeader();
  cout << tallLeftSize.str(offset);
  cout << endl;

  const unsigned nvm2 = values2.size() / 2;
  nth_element(values2.begin(), values2.begin() + nvm2, values2.end());
  tallRightSize.level = values2[nvm2];

  nth_element(rangesLeft2.begin(), rangesLeft2.begin() + nvm2, 
    rangesLeft2.end());
  tallRightSize.range1 = rangesLeft2[nvm2];

  nth_element(gradLeft2.begin(), gradLeft2.begin() + nvm2, gradLeft2.end());
  tallRightSize.grad1 = gradLeft2[nvm2];

  nth_element(rangesRight2.begin(), rangesRight2.begin() + nvm2, 
    rangesRight2.end());
  tallRightSize.range2 = rangesRight2[nvm2];

  nth_element(gradRight2.begin(), gradRight2.begin() + nvm2, gradRight2.end());
  tallRightSize.grad2 = gradRight2[nvm2];

  if (tallRightSize.range2 != 0.f)
    tallRightSize.rangeRatio = tallRightSize.range1 / tallRightSize.range2;
  if (tallRightSize.grad2 != 0.f)
    tallRightSize.gradRatio = tallRightSize.grad1 / tallRightSize.grad2;

  // Print scale.
  cout << "Tall right scale" << endl;
  cout << tallRightSize.strHeader();
  cout << tallRightSize.str(offset);
  cout << endl;


  // Recalibrate the peak qualities.
  cout << "Refined peak qualities:\n";
  cout << tallLeftSize.strHeaderQ();
  for (auto& pa: peaksAnnot)
  {
    if (! pa.wheelFlag)
    {
      const float distLeft = pa.sharp.distToScale(tallLeftSize);
      const float distRight = pa.sharp.distToScale(tallRightSize);
      pa.quality = min(distLeft, distRight);

      const float distQLeft = pa.sharp.distToScaleQ(tallLeftSize);
      const float distQRight = pa.sharp.distToScaleQ(tallRightSize);
      pa.qualityShape = min(distQLeft, distQRight);
    }
    else if (pa.wheelSide == WHEEL_LEFT)
    {
      pa.quality = pa.sharp.distToScale(tallLeftSize);
      pa.qualityShape = pa.sharp.distToScaleQ(tallLeftSize);
    }
    else if (pa.wheelSide == WHEEL_RIGHT)
    {
      pa.quality = pa.sharp.distToScale(tallRightSize);
      pa.qualityShape = pa.sharp.distToScaleQ(tallRightSize);
    }

    cout << pa.sharp.strQ(pa.quality, pa.qualityShape, offset);

    if (pa.quality <= 0.3f || pa.qualityShape <= 0.20f)
      pa.tallFlag = true;
    else
      pa.tallFlag = false;
  }

  // Redo the distances.

  unsigned wheelDistLowerNew, wheelDistUpperNew;
  unsigned whcountNew;

  PeakDetect::guessDistance(peaksAnnot, &PeakDetect::bothTall,
    wheelDistLowerNew, wheelDistUpperNew, whcountNew);



  // Could be zero
cout << "Guessing new wheel distance " << wheelDistLowerNew << "-" <<
  wheelDistUpperNew << endl;

  // Mark more bogeys with the refined peak qualities.

  for (auto pit = peaksAnnot.begin(); pit != prev(peaksAnnot.end());
    pit++)
  {
    auto npit = next(pit);
    // Already paired up?
    if (pit->wheelFlag || npit->wheelFlag)
      continue;

    if (pit->tallFlag && npit->tallFlag)
    {
      const unsigned dist = 
        npit->peakPtr->getIndex() - pit->peakPtr->getIndex();
      if (dist >= wheelDistLowerNew && dist <= wheelDistUpperNew)
      {
        if (pit->wheelFlag)
          THROW(ERR_NO_PEAKS, "Triple bogey?!");

        pit->wheelFlag = true;
        pit->wheelSide = WHEEL_LEFT;
        npit->wheelFlag = true;
        npit->wheelSide = WHEEL_RIGHT;
        numBogeys++;
cout << "Marking new bogey at " << pit->peakPtr->getIndex()+offset << "-" <<
  npit->peakPtr->getIndex()+offset << endl;
      }
    }
  }

  // Look for inter-car short gaps.
  unsigned shortGapLower, shortGapUpper;
  unsigned shcount;

  PeakDetect::guessDistance(peaksAnnot, &PeakDetect::areBogeyGap,
    shortGapLower, shortGapUpper, shcount);

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
  vector<unsigned> dists;

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

  vector<CarDetect> cars;

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

  models.append();

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

    const unsigned posLeft1 = pit->prevLargePeakPtr->getIndex();
    const unsigned posRight1 = pit->peakPtr->getIndex();
    const unsigned posLeft2 = npit->peakPtr->getIndex();
    const unsigned posRight2 = npit->nextLargePeakPtr->getIndex();

    const unsigned dist = posLeft2 - posRight1;

    if (dist < longGapLower || dist > longGapUpper)
      continue;

    pit->bogeySide = BOGEY_LEFT;
    npit->bogeySide = BOGEY_RIGHT;
cout << "\nMarking long gap at " << 
  posRight1 + offset << "-" <<
  posLeft2 + offset << endl;

    for (auto it = next(pit); it != npit; it++)
      it->spuriousFlag = true;
      
    // Fill out cars.
    cars.emplace_back(CarDetect());
    CarDetect& car = cars.back();

    car.logPeakPointers(
      pit->prevLargePeakPtr,
      &*pit->peakPtr,
      &*npit->peakPtr,
      npit->nextLargePeakPtr);

    car.logCore(
      posRight1 - posLeft1,
      dist,
      posRight2 - posLeft2);

    car.logStatIndex(0);

    auto ppit = prev(pit);
    Peak * p0 = ppit->prevLargePeakPtr;

    unsigned leftGap;
    if (p0 != nullptr && ppit->bogeySide != BOGEY_SIZE)
    {
      // This rounds to make the cars abut.
      const unsigned d = posLeft1 - p0->getIndex();
      leftGap  = d - (d/2);
    }
    else
      leftGap = 0;

    auto nnpit = next(npit);
    pit = nnpit; // Advance across the peaks that we've marked
    Peak * p1 = nnpit->nextLargePeakPtr;

    unsigned rightGap;
    if (p1 != nullptr && nnpit->bogeySide != BOGEY_SIZE)
      rightGap = (p1->getIndex() - posRight2) / 2;
    else
      rightGap = 0;

    if (car.fillSides(leftGap, rightGap))
      cout << "Filled out complete car: " << car.strLimits(offset) << endl;
    else
      cout << "Could not fill out any limits\n";

cout << "Marking car: " << car.strLimits(offset) << endl;

    models += car;
  }

  cout << "Counting " << PeakDetect::countWheels(peaksAnnot) << 
    " peaks" << endl << endl;

  // Fill out interval ends.
  if (models.size() == 0)
    THROW(ERR_NO_PEAKS, "No first car type found");

  for (auto& car: cars)
  {
    if (models.fillSides(car))
    {
cout << "Filled out complete car: " << car.strLimits(offset) << endl;
    }
  }

  PeakDetect::printCars(cars, "before intra gaps");

  // Check open intervals.  Start with inner ones as they are complete.

  // TODO cars change in the inner loop, not so pretty.
if (cars.size() == 0)
  THROW(ERR_NO_PEAKS, "No cars?");

  const unsigned u1 = cars.back().endValue();

  const unsigned u2 = peaksAnnot.back().peakPtr->getIndex();
  const unsigned csize = cars.size();

  for (unsigned ii = 0; ii+1 < csize; ii++)
  {
    if (cars[ii].endValue() != cars[ii+1].startValue())
    {
      if (! PeakDetect::findCars(
        cars[ii].endValue(), cars[ii+1].startValue(), 
        true, true, peaksAnnot, cars))
      {
        cout << "Couldn't understand intra-gap " <<
          cars[ii].endValue()+offset << "-" << 
          cars[ii+1].startValue()+offset << endl;
      }
      else
      {
cout << "Did intra-gap " << cars[ii].endValue()+offset << "-" << 
  cars[ii+1].startValue()+offset << endl;
      }
    }
  }

  cout << "Counting " << PeakDetect::countWheels(peaksAnnot) << 
    " peaks" << endl << endl;

  PeakDetect::printCars(cars, "after inner gaps");
  PeakDetect::printCarStats("after inner gaps");

  if (u2 > u1)
  {
    if (! PeakDetect::findCars(u1, u2,
      true, false, peaksAnnot, cars))
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

  cout << "Counting " << PeakDetect::countWheels(peaksAnnot) << 
    " peaks" << endl << endl;

  PeakDetect::printCarStats("after trailing gap");


  const unsigned u3 = peaksAnnot.front().peakPtr->getIndex();
  const unsigned u4 = cars.front().startValue();

  if (u3 < u4)
  {
    if (! PeakDetect::findCars(u3, u4,
      false, true, peaksAnnot, cars))
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

  PeakDetect::printCarStats("after leading gap");


  // TODO Check peak quality and deviations in carStats[0].
  // Detect inner and open intervals that are not done.
  // Could be carStats[0] with the right length etc, but missing peaks.
  // Or could be a new type of car (or multiple cars).
  // Ends come later.

  // Put peaks in the global list.
  // peaks.clear();
  for (auto& p: peaksAnnot)
  {
    if (p.wheelFlag)
      p.peakPtr->select();
  }


  cout << "Returning " << PeakDetect::countWheels(peaksAnnot) << 
    " peaks" << endl << endl;

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

cout << "Peak size 1: " << peaks.size() << endl;

/*
float minRange = numeric_limits<float>::max();
for (auto& p: peaks)
{
  const float r = p.getRange();
  if (r != 0.f && r < minRange)
    minRange = r;
}
*/

  // PeakDetect::reduceSmallRanges(0.05f, false);
  // PeakDetect::reduceSmallRanges(scalesList.getRange() / 500.f, false);

  PeakDetect::reduceSmallAreas(0.1f);

cout << "Peak size 2: " << peaks.size() << endl;

/*
float minRange2 = numeric_limits<float>::max();
for (auto& p: peaks)
{
  const float r = p.getRange();
  if (r != 0.f && r < minRange2)
    minRange2 = r;
}
cout << "RANGE: " << fixed <<
  setprecision(6) << minRange << " -> " << minRange2 << endl;
  */

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

cout << "Peak size 3: " << peaks.size() << endl;

  PeakDetect::estimateScales();
  if (debug)
  {
    cout << "Scale list\n";
    cout << scalesList.str(0) << endl;
  }

  PeakDetect::setOrigPointers();

  PeakDetect::reduceSmallRanges(scalesList.getRange() / 10.f, true);

cout << "Peak size 4: " << peaks.size() << endl;

  PeakDetect::markPossibleQuiet();

cout << "Peak size 5: " << peaks.size() << endl;

PeakDetect::reduceNewer();

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
      // peak.select();
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
        peakStats.logSeenMatch(static_cast<unsigned>(m), lt, pt);
        if (seenTrue[m] && debug)
          cout << "Already saw true peak " << m << endl;
        seenTrue[m]++;
        seen++;
      }
      else
      {
        peakStats.logSeenMiss(pt);
      }
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

  for (auto& peak: peaks)
  {
    if (peak.isSelected())
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

