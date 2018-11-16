#include <list>
#include <iostream>
#include <iomanip>
#include <functional>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakDetect.h"
#include "PeakSeeds.h"
#include "PeakMatch.h"
#include "PeakStats.h"
#include "Except.h"


#define SAMPLE_RATE 2000.

#define KINK_RATIO 100.f

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


// TMP
#define TIME_PROXIMITY 0.03
#define SCORE_CUTOFF 0.75


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

  for (auto it = peakFirst; it != peaks.end(); it++)
  {
    const Peak * peakPrev = (it == peakFirst ? nullptr : &*prev(it));

    it->annotate(peakPrev);
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
    Peak const * peakPrev = nullptr;
    float areaFull = 0.f;
    const unsigned index = it->getIndex();

    if (it != peakFirst)
    {
      peakPrev = &*prev(it);
      areaFull = peakPrev->getAreaCum() + 
        PeakDetect::integrate(samples, peakPrev->getIndex(), index);
    }

    peakSynth.log(index, it->getValue(), areaFull, it->getMaxFlag());
    peakSynth.annotate(peakPrev);

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

  peak2->update(peak0);

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

      if (peakPrev->similarGradient(* peak, * peakNext))
        peak = PeakDetect::collapsePeaks(peakPrev, peakNext);
      else
        peak++;
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


void PeakDetect::findFirstSize(
  const vector<unsigned>& dists,
  Gap& gap,
  const unsigned lowerCount) const
{
  PeakDetect::findFirstSize(dists,
    gap.lower, gap.upper, gap.count, lowerCount);
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
  PeakDetect::markWheelPair(
    * peaksAnnot[peakIndices[p0]].peakPtr,
    * peaksAnnot[peakIndices[p1]].peakPtr,
    "");

  peaksAnnot[peakIndices[p0]].peakPtr->markBogey(BOGEY_RIGHT);
  peaksAnnot[peakIndices[p1]].peakPtr->markBogey(BOGEY_RIGHT);

  for (unsigned i = 0; i < peakIndices.size(); i++)
  {
    if (i != p0 && i != p1)
      peaksAnnot[peakIndices[i]].peakPtr->markNoWheel();
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
  peaksAnnot[peakIndices[p0]].peakPtr->markWheel(WHEEL_RIGHT);

  PeakDetect::markWheelPair(
    * peaksAnnot[peakIndices[p1]].peakPtr,
    * peaksAnnot[peakIndices[p2]].peakPtr,
    "");

  peaksAnnot[peakIndices[p0]].peakPtr->markBogey(BOGEY_LEFT);
  peaksAnnot[peakIndices[p1]].peakPtr->markBogey(BOGEY_RIGHT);
  peaksAnnot[peakIndices[p2]].peakPtr->markBogey(BOGEY_RIGHT);

  for (unsigned i = 0; i < peakIndices.size(); i++)
  {
    if (i != p0 && i != p1 && i != p2)
      peaksAnnot[peakIndices[i]].peakPtr->markNoWheel();
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
  PeakDetect::markWheelPair(
    * peaksAnnot[peakIndices[p0]].peakPtr,
    * peaksAnnot[peakIndices[p1]].peakPtr,
    "");

  PeakDetect::markWheelPair(
    * peaksAnnot[peakIndices[p2]].peakPtr,
    * peaksAnnot[peakIndices[p3]].peakPtr,
    "");

  peaksAnnot[peakIndices[p0]].peakPtr->markBogey(BOGEY_LEFT);
  peaksAnnot[peakIndices[p1]].peakPtr->markBogey(BOGEY_LEFT);
  peaksAnnot[peakIndices[p2]].peakPtr->markBogey(BOGEY_RIGHT);
  peaksAnnot[peakIndices[p3]].peakPtr->markBogey(BOGEY_RIGHT);

  for (unsigned i = 0; i < peakIndices.size(); i++)
  {
    if (i != p0 && i != p1 && i != p2 && i != p3)
      peaksAnnot[peakIndices[i]].peakPtr->markNoWheel();
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
      if (! peaksAnnot[i].peakPtr->isSeed())
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
      if (peaksAnnot[peakIndices[i]].peakPtr->goodQuality())
        seen++;
    }

    if (seen == 4)
    {
      vector<unsigned> peakNosNew, peakIndicesNew;
      for (unsigned j = i; j < np; j++)
      {
        if (peaksAnnot[peakIndices[j]].peakPtr->goodQuality())
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
      if (! peaksAnnot[i].peakPtr->isSeed())
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

      if (peaksAnnot[i].peakPtr->getQualityShape() > qmax)
      {
        qmax = peaksAnnot[i].peakPtr->getQualityShape();
        qindex = j;
      }

      if (peaksAnnot[i].peakPtr->goodQuality())
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
    peaksAnnot[i].peakPtr->getQualityShape() << endl;

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
      ! peaksAnnot[peakIndices[0]].peakPtr->isSeed())
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
    else if (! peaksAnnot[peakIndices[0]].peakPtr->goodQuality() &&
        peaksAnnot[peakIndices[1]].peakPtr->goodQuality() &&
        peaksAnnot[peakIndices[2]].peakPtr->goodQuality())
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
    if (p.peakPtr->isWheel())
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
  return (pe1.peakPtr->isSeed() && pe2.peakPtr->isSeed());
}


bool PeakDetect::bothSeed(
  const Peak * p1,
  const Peak * p2) const
{
  return (p1->isSeed() && p2->isSeed());
}


bool PeakDetect::areBogeyGap(
  const PeakEntry& pe1,
  const PeakEntry& pe2) const
{
  return (pe1.peakPtr->isRightWheel() && pe2.peakPtr->isLeftWheel());
}


bool PeakDetect::formBogeyGap(
  const Peak * p1,
  const Peak * p2) const
{
  return (p1->isRightWheel() && p2->isLeftWheel());
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


void PeakDetect::guessDistance(
  const list<Peak *>& candidates,
  const CandFncPtr fptr,
  Gap& gap) const
{
  // Make list of distances between neighbors for which fptr
  // evaluates to true.

  vector<unsigned> dists;
  for (auto pit = candidates.begin(); pit != prev(candidates.end()); pit++)
  {
    auto npit = next(pit);
    if ((this->* fptr)(* pit, * npit))
      dists.push_back(
        (*npit)->getIndex() - (*pit)->getIndex());
  }

  sort(dists.begin(), dists.end());
  PeakDetect::findFirstSize(dists, gap);
}


void PeakDetect::markWheelPair(
  Peak& p1,
  Peak& p2,
  const string& text) const
{
  if (text != "")
    cout << text << " wheel pair at " << p1.getIndex() + offset <<
      "-" << p2.getIndex() + offset << "\n";
    
  p1.setSeed();
  p2.setSeed();

  p1.markWheel(WHEEL_LEFT);
  p2.markWheel(WHEEL_RIGHT);
}



void PeakDetect::markBogeyShortGap(
  Peak& p1,
  Peak& p2,
  const string& text) const
{
  if (text != "")
    cout << text << " short car gap at " << p1.getIndex() + offset <<
      "-" << p2.getIndex() + offset << "\n";
  
  p1.markBogey(BOGEY_RIGHT);
  p2.markBogey(BOGEY_LEFT);
}


void PeakDetect::markBogeyLongGap(
  Peak& p1,
  Peak& p2,
  const string& text) const
{
  if (text != "")
    cout << text << " long car gap at " << p1.getIndex() + offset <<
      "-" << p2.getIndex() + offset << "\n";
  
  p1.markBogey(BOGEY_LEFT);
  p2.markBogey(BOGEY_RIGHT);
}


void PeakDetect::printScale(
  const Peak& scale,
  const string& text) const
{
  cout << text << "\n";
  cout << scale.strHeaderSum();
  cout << scale.strSum(offset);
  cout << endl;
}


void PeakDetect::reduceNewer()
{
  // Mark some tall peaks as seeds.
  PeakSeeds seeds;
  seeds.mark(peaks, offset, scalesList.getRange());
  cout << seeds.str(offset, "after pruning");

  vector<PeakEntry> peaksAnnot;
  list<Peak *> candidates;

  // Note which peaks are tall.
  for (auto pit = peaks.begin(); pit != peaks.end(); pit++)
  {
    if (! pit->isCandidate())
      continue;

    // Exclude tall peaks without a right neighbor.
    auto npit = next(pit);
    if (npit == peaks.end())
      continue;

    peaksAnnot.emplace_back(PeakEntry());
    PeakEntry& pe = peaksAnnot.back();

    pe.peakPtr = &*pit;
    pit->logNextPeak(&*npit);
  }

  for (auto pit = peaksAnnot.begin(); pit != peaksAnnot.end(); pit++)
  {
    pit->prevLargePeakPtr = (pit == peaksAnnot.begin() ? nullptr : 
      prev(pit)->peakPtr);
    pit->nextLargePeakPtr = (next(pit) == peaksAnnot.end() ? nullptr : 
      next(pit)->peakPtr);
  }

  for (auto& peak: peaks)
  {
    if (peak.isCandidate())
      candidates.push_back(&peak);
  }

  // Find typical sizes of "tall" peaks.

  const unsigned np = peaksAnnot.size();
  if (np == 0)
    THROW(ERR_NO_PEAKS, "No tall peaks");

  // Will need one later on for each wheel/bogey combination.
  vector<Peak> candidateSize(1);
  vector<unsigned> candidateCount(1);
  for (auto candidate: candidates)
  {
    if (candidate->isSeed())
    {
      candidateSize[0] += * candidate;
      candidateCount[0]++;
    }
  }
  candidateSize[0] /= candidateCount[0];

  // Use this as a first yardstick.
  for (auto candidate: candidates)
    candidate->calcQualities(candidateSize[0]);


  cout << "All negative minima\n";
  cout << candidateSize[0].strHeaderQuality();
  for (auto candidate: candidates)
    cout << candidate->strQuality(offset);
  cout << endl;

  PeakDetect::printScale(candidateSize[0], "Single seed scale");

  cout << "Seeds\n";
  cout << candidateSize[0].strHeaderQuality();
  for (auto candidate: candidates)
  {
    if (candidate->isSeed())
      cout << candidate->strQuality(offset);
  }
  cout << endl;


  // Modify selection based on quality.
  for (auto candidate: candidates)
  {
    if (candidate->greatQuality())
    {
      candidate->setSeed();
      cout << "Adding tallFlag to " << 
        candidate->getIndex() + offset << endl;
    }
    else
    {
      candidate->unsetSeed();
      cout << "Removing tallFlag from " << 
        candidate->getIndex() + offset << endl;
    }
  }
  cout << "\n";

  cout << "Great-quality seeds\n";
  cout << candidateSize[0].strHeaderQuality();
  for (auto candidate: candidates)
  {
    if (candidate->isSeed())
      cout << candidate->strQuality(offset);
  }
  cout << endl;


  Gap wheelGap;
  PeakDetect::guessDistance(candidates, &PeakDetect::bothSeed, wheelGap);

  cout << "Guessing wheel distance " << wheelGap.lower << "-" <<
    wheelGap.upper << endl;

  // Tentatively mark wheel pairs (bogeys).  If there are only 
  // single wheels, we might be marking the wagon gaps instead.
  for (auto cit = candidates.begin(); cit != prev(candidates.end()); cit++)
  {
    Peak * cand = * cit;
    Peak * nextCand = * next(cit);
    if (PeakDetect::bothSeed(cand, nextCand))
    {
      const unsigned dist = nextCand->getIndex() - cand->getIndex();
      if (dist >= wheelGap.lower && dist <= wheelGap.upper)
      {
        if (cand->isWheel())
          THROW(ERR_NO_PEAKS, "Triple bogey?!");

        PeakDetect::markWheelPair(* cand, * nextCand, "Marking");
      }
    }
  }

  // Look for unpaired wheels where there is a nearby peak that is
  // not too bad.  If there is a spurious peak in between, we'll fail...
  for (auto cit = candidates.begin(); cit != prev(candidates.end()); cit++)
  {
    Peak * cand = * cit;
    Peak * nextCand = * next(cit);
    // If neither is set, or both are set, there is nothing to repair.
    if (cand->isSeed() == nextCand->isSeed())
      continue;

    const unsigned dist = nextCand->getIndex() - cand->getIndex();
    if (dist < wheelGap.lower || dist > wheelGap.upper)
      continue;

    if ((cand->isSeed() && nextCand->acceptableQuality()) ||
        (nextCand->isSeed() && cand->acceptableQuality()))
      PeakDetect::markWheelPair(* cand, * nextCand, "Adding");
  }

  candidateSize.clear();
  candidateSize.resize(2);
  candidateCount.clear();
  candidateCount.resize(2);
  for (auto candidate: candidates)
  {
    if (candidate->isLeftWheel())
    {
      candidateSize[0] += * candidate;
      candidateCount[0]++;
    }
    else if (candidate->isRightWheel())
    {
      candidateSize[1] += * candidate;
      candidateCount[1]++;
    }
  }
  candidateSize[0] /= candidateCount[0];
  candidateSize[1] /= candidateCount[1];

  PeakDetect::printScale(candidateSize[0], "Left-wheel scale");
  PeakDetect::printScale(candidateSize[1], "Right-wheel scale");

  // Recalculate the peak qualities using both left and right peaks.
  for (auto cand: candidates)
  {
    if (! cand->isWheel())
      cand->calcQualities(candidateSize);
    else if (cand->isLeftWheel())
      cand->calcQualities(candidateSize[0]);
    else if (cand->isRightWheel())
      cand->calcQualities(candidateSize[1]);

    if (cand->greatQuality())
      cand->setSeed();
    else
      cand->unsetSeed();
  }

  cout << "All peak qualities using left and right scales:\n";
  cout << candidateSize[0].strHeaderQuality();
  for (auto& pa: peaksAnnot)
    cout << pa.peakPtr->strQuality(offset);


  // Redo the distances using the new qualities (left and right peaks).
  PeakDetect::guessDistance(candidates, &PeakDetect::bothSeed, wheelGap);
  cout << "Guessing new wheel distance " << wheelGap.lower << "-" <<
    wheelGap.upper << endl;


  // Mark more bogeys with the refined peak qualities.
  for (auto cit = candidates.begin(); cit != prev(candidates.end()); cit++)
  {
    Peak * cand = * cit;
    if (cand->isWheel())
      continue;

    Peak * nextCand = * next(cit);
    if (PeakDetect::bothSeed(cand, nextCand))
    {
      const unsigned dist = nextCand->getIndex() - cand->getIndex();
      if (dist >= wheelGap.lower && dist <= wheelGap.upper)
      {
        if (cand->isWheel())
          THROW(ERR_NO_PEAKS, "Triple bogey?!");

        PeakDetect::markWheelPair(* cand, * nextCand, "Marking");
      }
    }
  }


  // Look for inter-car short gaps.
  Gap shortGap;
  PeakDetect::guessDistance(candidates, &PeakDetect::formBogeyGap, 
    shortGap);

  cout << "Guessing short gap " << shortGap.lower << "-" <<
    shortGap.upper << endl;


  // Tentatively mark short gaps (between cars).
  for (auto cit = candidates.begin(); cit != prev(candidates.end()); cit++)
  {
    Peak * cand = * cit;
    Peak * nextCand = * next(cit);

    if (! cand->isRightWheel() || ! nextCand->isLeftWheel())
      continue;

    const unsigned dist = nextCand->getIndex() - cand->getIndex();
    if (dist >= shortGap.lower && dist <= shortGap.upper)
      PeakDetect::markBogeyShortGap(* cand, * nextCand, "Marking");
  }


  // Look for unpaired short gaps.  If there is a spurious peak
  // in between, we will fail.
  for (auto cit = candidates.begin(); cit != prev(candidates.end()); cit++)
  {
    Peak * cand = * cit;
    Peak * nextCand = * next(cit);

    // If neither is set, or both are set, there is nothing to repair.
    if (cand->isRightWheel() == nextCand->isLeftWheel())
      continue;

    const unsigned dist = nextCand->getIndex() - cand->getIndex();
    if (dist < shortGap.lower || dist > shortGap.upper)
      continue;

    if ((cand->isRightWheel() && nextCand->greatQuality()) ||
        (nextCand->isLeftWheel() && cand->greatQuality()))
      PeakDetect::markBogeyShortGap(* cand, * nextCand, "Marking");
  }


  // Look for smallest large gaps.
  vector<unsigned> dists;

  for (auto pit = peaksAnnot.begin(); pit != prev(peaksAnnot.end());
    pit++)
  {
    if (! pit->peakPtr->isRightWheel() || pit->peakPtr->isBogey())
      continue;

    auto npit = next(pit);
    while (npit != peaksAnnot.end() && 
        ! npit->peakPtr->isLeftWheel() &&
        ! npit->peakPtr->isRightWheel() &&
        ! npit->peakPtr->isSeed())
    {
      npit = next(npit);
    }


    if (npit == peaksAnnot.end() || ! npit->peakPtr->isLeftWheel())
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
    lcount, shortGap.count / 2);
  // Could be zero
cout << "Guessing long gap " << longGapLower << "-" <<
  longGapUpper << endl;

  // Label intra-car gaps.

  models.append();

  for (auto pit = peaksAnnot.begin(); pit != prev(peaksAnnot.end());
    pit++)
  {
    if (! pit->peakPtr->isRightWheel()|| pit->peakPtr->isBogey())
      continue;

    auto npit = next(pit);
    while (npit != peaksAnnot.end() && 
        ! npit->peakPtr->isLeftWheel() &&
        ! npit->peakPtr->isRightWheel())
    {
      npit = next(npit);
    }

    if (npit == peaksAnnot.end() || npit->peakPtr->isRightWheel())
      continue;

    const unsigned posLeft1 = pit->prevLargePeakPtr->getIndex();
    const unsigned posRight1 = pit->peakPtr->getIndex();
    const unsigned posLeft2 = npit->peakPtr->getIndex();
    const unsigned posRight2 = npit->nextLargePeakPtr->getIndex();

    const unsigned dist = posLeft2 - posRight1;

    if (dist < longGapLower || dist > longGapUpper)
      continue;

cout << "\nMarking long gap at " << 
  posRight1 + offset << "-" <<
  posLeft2 + offset << 
  " (" << posLeft1+offset << "-" << posRight2+offset << ")" << endl;

    PeakDetect::markBogeyLongGap(* pit->peakPtr, * npit->peakPtr,
        "Marking");

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
    if (p0 != nullptr && ppit->peakPtr->isBogey())
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
    if (p1 != nullptr && nnpit->peakPtr->isBogey())
      rightGap = (p1->getIndex() - posRight2) / 2;
    else
      rightGap = 0;

    if (car.fillSides(leftGap, rightGap))
      cout << "Filled out complete car: " << car.strLimits(offset) << endl;
    else
    {
      cout << "Could not fill out any limits: " <<
        leftGap << ", " << rightGap << endl;
    }

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
  for (auto& p: peaksAnnot)
  {
    if (p.peakPtr->isWheel())
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

  PeakDetect::estimateScales();
  if (debug)
  {
    cout << "Scale list\n";
    cout << scalesList.str(0) << endl;
  }

  PeakDetect::reduceSmallRanges(scalesList.getRange() / 10.f, true);

PeakDetect::reduceNewer();

  bool firstSeen = false;
  unsigned firstTentativeIndex = peaks.front().getIndex();
  unsigned lastTentativeIndex = peaks.back().getIndex();
  numCandidates = 0;
  numTentatives = 0;

// cout << "Starting FRONT " << firstTentativeIndex << " LAST " <<
  // lastTentativeIndex << endl;

  for (auto& peak: peaks)
  {
    if (peak.isSelected())
    {
      if (! firstSeen)
      {
        firstTentativeIndex = peak.getIndex();
        firstSeen = true;
      }
      lastTentativeIndex = peak.getIndex();
    }
  }
}


void PeakDetect::logPeakStats(
  const vector<PeakPos>& posTrue,
  const string& trainTrue,
  const double speedTrue,
  PeakStats& peakStats)
{
  PeakMatch matches;
  matches.logPeakStats(peaks, posTrue, trainTrue, speedTrue, peakStats);
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
    if (peak.isSelected())
    {
      cout << pp << ";" << 
        fixed << setprecision(6) << peak.getTime() << endl;
      pp++;
    }
  }
  cout << "\n";
}

