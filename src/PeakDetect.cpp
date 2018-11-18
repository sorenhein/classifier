#include <list>
#include <iostream>
#include <iomanip>
#include <functional>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakDetect.h"
#include "PeakSeeds.h"
#include "PeakMinima.h"
#include "PeakMatch.h"
#include "PeakStats.h"
#include "Except.h"


#define SAMPLE_RATE 2000.

#define KINK_RATIO 100.f

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
  candidates.clear();
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


void PeakDetect::estimateScale(Peak& scale)
{
  scale.reset();
  unsigned no = 0;
  for (auto peak = next(peaks.begin()); peak != peaks.end(); peak++)
  {
    if (! peak->getMaxFlag() && peak->getValue() < 0.f)
    {
      scale += * peak;
      no++;
    }
  }

  if (no == 0)
    THROW(ERR_NO_PEAKS, "No negative minima");

  scale/= no;
}


bool PeakDetect::matchesModel(
  const CarDetect& car, 
  unsigned& index,
  float& distance) const
{
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
  const vector<unsigned>& peakNos, 
  const vector<Peak *>& peakPtrs,
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
    peakPtrs[0], peakPtrs[1], peakPtrs[2], peakPtrs[3]);

  return models.gapsPlausible(car);
}


bool PeakDetect::findLastTwoOfFourWheeler(
  const unsigned start, 
  const unsigned end,
  const bool rightGapPresent,
  const vector<unsigned>& peakNos, 
  const vector<Peak *>& peakPtrs,
  CarDetect& car) const
{
  car.setLimits(start, end);

  if (rightGapPresent)
    car.logRightGap(end - peakNos[1]);

  car.logCore(0, 0, peakNos[1] - peakNos[0]);

  car.logPeakPointers(
    nullptr, nullptr, peakPtrs[0], peakPtrs[1]);

  if (! models.sideGapsPlausible(car))
    return false;

  // As we don't have a complete car, we'll at least require the
  // right bogey gap to be similar to something we've seen.

  if (models.rightBogeyPlausible(car))
    return true;
  else
  {
    cout << "Error: Suspect right bogey gap: ";
    cout << car.strGaps(0) << endl;
    cout << "Checked against " << models.size() << " ref cars\n";
    return false;
  }
}


bool PeakDetect::findLastThreeOfFourWheeler(
  const unsigned start, 
  const unsigned end,
  const vector<unsigned>& peakNos, 
  const vector<Peak *>& peakPtrs,
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
    nullptr, peakPtrs[0], peakPtrs[1], peakPtrs[2]);

  if (! models.sideGapsPlausible(car))
    return false;

  // As we don't have a complete car, we'll at least require the
  // right bogey gap to be similar to something we've seen.

  if (! models.rightBogeyPlausible(car))
  {
    cout << "Error: Suspect right bogey gap: ";
    cout << car.strGaps(0) << endl;
    cout << "Checked against " << models.size() << " ref cars\n";
    return false;
  }

  if (! car.midGapPlausible())
  {
    // Drop the first peak.
    numWheels = 2;
    car.logPeakPointers(
      nullptr, nullptr, peakPtrs[1], peakPtrs[2]);

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


void PeakDetect::markWheelPair(
  Peak& p1,
  Peak& p2,
  const string& text) const
{
  if (text != "")
    cout << text << " wheel pair at " << p1.getIndex() + offset <<
      "-" << p2.getIndex() + offset << "\n";

  p1.select();
  p2.select();

  p1.markWheel(WHEEL_LEFT);
  p2.markWheel(WHEEL_RIGHT);
}


void PeakDetect::fixTwoWheels(
  Peak& p1,
  Peak& p2) const
{
  // Assume the two rightmost wheels, as the front ones were lost.
  PeakDetect::markWheelPair(p1, p2, "");

  p1.markBogey(BOGEY_RIGHT);
  p2.markBogey(BOGEY_RIGHT);
}


void PeakDetect::fixThreeWheels(
  Peak& p1,
  Peak& p2,
  Peak& p3) const
{
  // Assume the two rightmost wheels, as the front one was lost.
  p1.select();
  p1.markWheel(WHEEL_RIGHT);
  PeakDetect::markWheelPair(p2, p3, "");

  p1.markBogey(BOGEY_LEFT);
  p2.markBogey(BOGEY_RIGHT);
  p3.markBogey(BOGEY_RIGHT);
}


void PeakDetect::fixFourWheels(
  Peak& p1,
  Peak& p2,
  Peak& p3,
  Peak& p4) const
{
  PeakDetect::markWheelPair(p1, p2, "");
  PeakDetect::markWheelPair(p3, p4, "");

  p1.markBogey(BOGEY_LEFT);
  p2.markBogey(BOGEY_LEFT);
  p3.markBogey(BOGEY_RIGHT);
  p4.markBogey(BOGEY_RIGHT);
}


void PeakDetect::updateCars(
  vector<CarDetect>& cars,
  CarDetect& car)
{
  unsigned index;
  float distance;

  if (! car.isPartial() &&
      PeakDetect::matchesModel(car, index, distance))
  {
    car.logStatIndex(index);
    car.logDistance(distance);

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
  // list<Peak *>& candidates,
  vector<CarDetect>& cars)
{
  vector<unsigned> peakNos;
  vector<Peak *> peakPtrs;
  unsigned notTallCount = 0;
  vector<unsigned> notTallNos;
  bool notTallFlag = false;

  unsigned i = 0;
  for (auto& cand: candidates)
  {
    const unsigned index = cand->getIndex();
    if (index >= start && index <= end)
    {
      if (! cand->isSelected())
      {
        notTallFlag = true;
        notTallNos.push_back(peakNos.size());
        notTallCount++;
      }

      peakNos.push_back(index);
      peakPtrs.push_back(cand);
      
    }
    i++;
  }

  unsigned startLocal = start;
  unsigned endLocal = end;
  bool leftFlagLocal = leftGapPresent;
  bool rightFlagLocal = rightGapPresent;

  unsigned np = peakNos.size();
  bool firstRightFlag = true;
  const unsigned npOrig = np;
cout << "Starting out with " << np << " peaks\n";

  while (np >= 8)
  {
    // Might be multiple cars.  Work backwards, as this works best
    // for the leading cars.  Use groups of four good peaks.
    i = np;
    unsigned seen = 0;
    while (i > 0 && seen < 4)
    {
      i--;
      if (peakPtrs[i]->goodQuality())
        seen++;
    }

    if (seen == 4)
    {
      vector<unsigned> peakNosNew;
      vector<Peak *> peakPtrsNew;
      for (unsigned j = i; j < np; j++)
      {
        if (peakPtrs[j]->goodQuality())
        {
          peakNosNew.push_back(peakNos[j]);
          peakPtrsNew.push_back(peakPtrs[j]);
        }

        peakNos.pop_back();
        peakPtrs.pop_back();
      }
cout << "Trying a multi-peak split, np was " << np << ", is " << i << endl;
cout << "sizes " << peakNosNew.size() << ", " << peakPtrsNew.size() <<
  ", " << peakNos.size() << ", " << peakPtrs.size() << endl;
      np = i;

      if (peakPtrsNew.front() == peakPtrs.front())
      {
        startLocal = start;
        leftFlagLocal = leftGapPresent;
      }
      else
      {
        startLocal = peakPtrsNew[0]->getIndex();
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
        endLocal = peakPtrsNew[3]->getIndex();
        rightFlagLocal = false;
      }
cout << "Range is " << startLocal+offset << "-" << 
  endLocal+offset << endl;

      CarDetect car;
      if (PeakDetect::findFourWheeler(startLocal, endLocal,
        leftFlagLocal, rightFlagLocal, peakNosNew, peakPtrsNew, car))
      {
        PeakDetect::fixFourWheels(
          * peakPtrsNew[0], * peakPtrsNew[1], 
          * peakPtrsNew[2], * peakPtrsNew[3]);
        for (unsigned k = 4; k < peakPtrsNew.size(); k++)
        {
          peakPtrsNew[k]->markNoWheel();
          peakPtrsNew[k]->unselect();
        }
        
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
    if (peakPtrs.size() == 0)
      THROW(ERR_NO_PEAKS, "No residual peaks?");

        startLocal = start;
        leftFlagLocal = leftGapPresent;

        endLocal = peakPtrs.back()->getIndex();
        rightFlagLocal = false;

cout << "Fell through to " << startLocal+offset << "-" << endLocal+offset << endl;

  notTallCount = 0;
  notTallNos.clear();
  notTallFlag = false;
  peakNos.clear();
  peakPtrs.clear();

  i = 0;
  for (auto& cand: candidates)
  {
    const unsigned index = cand->getIndex();
    if (index >= startLocal && index <= endLocal)
    {
      if (! cand->isSelected())
      {
        notTallFlag = true;
        notTallNos.push_back(peakNos.size());
        notTallCount++;
      }

      peakNos.push_back(index);
      peakPtrs.push_back(cand);
      
    }
    i++;
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
      peakPtrs.erase(peakPtrs.begin() + notTallNos[1]);
    }

    peakNos.erase(peakNos.begin() + notTallNos[0]);
    peakPtrs.erase(peakPtrs.begin() + notTallNos[0]);

    CarDetect car;
    unsigned numWheels;
    if (! PeakDetect::findLastThreeOfFourWheeler(startLocal, endLocal,
        peakNos, peakPtrs, car, numWheels))
      return false;

    if (numWheels == 2)
    {
      PeakDetect::fixTwoWheels(* peakPtrs[1], * peakPtrs[2]);
      peakPtrs[0]->markNoWheel();
      peakPtrs[0]->unselect();
if (peakPtrs.size() != 3)
  cout << "ERRORW " << peakPtrs.size() << endl;
    }
    else 
      PeakDetect::fixThreeWheels(
        * peakPtrs[0], * peakPtrs[1], * peakPtrs[2]);
if (peakPtrs.size() != 3)
  cout << "ERRORa " << peakPtrs.size() << endl;

    PeakDetect::updateCars(cars, car);

    return true;
  }

  if (np == 4)
  {
    CarDetect car;
    if (PeakDetect::findFourWheeler(startLocal, endLocal,
        leftFlagLocal, rightFlagLocal, peakNos, peakPtrs, car))
    {
        PeakDetect::fixFourWheels(
          * peakPtrs[0], * peakPtrs[1], * peakPtrs[2], * peakPtrs[3]);
if (peakPtrs.size() != 4)
  cout << "ERROR1 " << peakPtrs.size() << endl;

      PeakDetect::updateCars(cars, car);

      return true;
    }
    
    // Give up unless this is the first car.
    if (leftFlagLocal)
      return false;

    // Try with only the acceptable-quality peaks.
    vector<unsigned> peakNosNew;
    vector<Peak *> peakPtrsNew;
    for (i = 0; i < peakPtrs.size(); i++)
    {
      if (peakPtrs[i]->acceptableQuality())
      {
        peakNosNew.push_back(peakNos[i]);
        peakPtrsNew.push_back(peakPtrs[i]);
      }
    }

    if (peakPtrsNew.size() <= 1)
    {
      cout << "Failed first cars: Only " << peakPtrsNew.size() <<
        " peaks\n";
      return false;
    }
    else if (peakPtrsNew.size() == 2)
    {
      if (! PeakDetect::findLastTwoOfFourWheeler(startLocal, endLocal,
          rightFlagLocal, peakNosNew, peakPtrsNew, car))
      {
        cout << "Failed first cars: " << peakPtrsNew.size() <<
          " peaks\n";
        return false;
      }

      cout << "Hit first car with 2 peaks\n";
      PeakDetect::fixTwoWheels(* peakPtrsNew[0], * peakPtrsNew[1]);

      for (i = 0; i < peakPtrs.size(); i++)
      {
        if (! peakPtrs[i]->acceptableQuality())
        {
          peakPtrs[i]->markNoWheel();
          peakPtrs[i]->unselect();
        }
      }

      PeakDetect::updateCars(cars, car);

      return true;
    }


    // Try again without the first peak.
cout << "Trying again without the very first peak of first car\n";
    Peak * peakErased = peakPtrs.front();
    peakNos.erase(peakNos.begin());
    peakPtrs.erase(peakPtrs.begin());

    unsigned numWheels;
    if (! PeakDetect::findLastThreeOfFourWheeler(startLocal, endLocal,
        peakNos, peakPtrs, car, numWheels))
      return false;

    if (numWheels == 2)
    {
      PeakDetect::fixTwoWheels(* peakPtrs[1], * peakPtrs[2]);
      peakPtrs[0]->markNoWheel();
      peakPtrs[0]->unselect();
if (peakPtrs.size() != 3)
  cout << "ERRORX " << peakPtrs.size() << endl;
    }
    else 
      PeakDetect::fixThreeWheels(
        * peakPtrs[0], * peakPtrs[1], * peakPtrs[2]);

if (peakPtrs.size() != 3)
  cout << "ERRORb " << peakPtrs.size() << endl;

      peakErased->markNoWheel();
      peakErased->unselect();

    PeakDetect::updateCars(cars, car);

    return true;
  }

  if (np >= 5)
  {
    // Might be several extra peaks (1-2).
    vector<unsigned> peakNosNew;
    vector<Peak *> peakPtrsNew;

    float qmax = 0.f;
    unsigned qindex = 0;

    unsigned j = 0;
    for (auto& peakPtr: peakPtrs)
    {
      if (peakPtr->getQualityShape() > qmax)
      {
        qmax = peakPtr->getQualityShape();
        qindex = j;
      }

      if (peakPtr->goodQuality())
      {
        peakNosNew.push_back(peakPtr->getIndex());
        peakPtrsNew.push_back(peakPtr);
      }
      j++;
    }

    if (peakNosNew.size() != 4)
    {
      // If there are five and the middle one stands out, try that.
      if (np == 5 && qindex == 2)
      {
cout << "General try with " << np << " didn't fit -- drop the middle?" <<
 endl;
        peakNos.erase(peakNos.begin() + 2);
        peakPtrs.erase(peakPtrs.begin() + 2);
      
        CarDetect car;
    cout << "Trying the car anyway\n";
        if (! PeakDetect::findFourWheeler(startLocal, endLocal,
            leftFlagLocal, rightFlagLocal, peakNos, peakPtrs, car))
        {
    cout << "Failed the car: " << np << "\n";
          return false;
        }

        PeakDetect::fixFourWheels(
          * peakPtrs[0], * peakPtrs[1], * peakPtrs[2], * peakPtrs[3]);
if (peakPtrs.size() != 4)
  cout << "ERROR2 " << peakPtrs.size() << endl;

        PeakDetect::updateCars(cars, car);
        return true;
      }
      else
      {
        cout << "I don't yet see one car here: " << np << ", " <<
          peakNosNew.size() << endl;

for (auto peakPtr: peakPtrs)
  cout << "index " << peakPtr->getIndex() << ", qs " <<
    peakPtr->getQualityShape() << endl;

        return false;
      }
    }

    CarDetect car;
    // Try the car.
cout << "Trying the car\n";
    if (! PeakDetect::findFourWheeler(startLocal, endLocal,
        leftFlagLocal, rightFlagLocal, peakNosNew, peakPtrsNew, car))
    {
cout << "Failed the car: " << np << "\n";
      return false;
    }

    PeakDetect::fixFourWheels(
      * peakPtrsNew[0], * peakPtrsNew[1], 
      * peakPtrsNew[2], * peakPtrsNew[3]);
    for (unsigned k = 4; k < peakPtrsNew.size(); k++)
    {
      peakPtrsNew[k]->markNoWheel();
      peakPtrsNew[k]->unselect();
    }

    PeakDetect::updateCars(cars, car);
    return true;
  }

  if (! leftFlagLocal && np == 3)
  {
cout << "Trying 3-peak leading car\n";
    // The first car, only three peaks.
    if (notTallCount == 1 && 
      ! peakPtrs[0]->isSelected())
    {
cout << "Two talls\n";
      // Skip the first peak.
      peakNos.erase(peakNos.begin() + notTallNos[0]);
      peakPtrs.erase(peakPtrs.begin() + notTallNos[0]);

      // TODO In all these erase's, need to unsetSeed and markNoWheel.
      // There should be a more elegant, central way of doing this.

      CarDetect car;
      if (! PeakDetect::findLastTwoOfFourWheeler(startLocal, endLocal,
          rightFlagLocal, peakNos, peakPtrs, car))
      {
cout << "Failed\n";
        return false;
      }

      PeakDetect::fixTwoWheels(* peakPtrs[0], * peakPtrs[1]);

      PeakDetect::updateCars(cars, car);

      return true;
    }
    else if (! peakPtrs[0]->goodQuality() &&
        peakPtrs[1]->goodQuality() &&
        peakPtrs[2]->goodQuality())
    {
cout << "Two good shapes\n";
      // Skip the first peak.
      peakNos.erase(peakNos.begin() + notTallNos[0]);
      peakPtrs.erase(peakPtrs.begin() + notTallNos[0]);

      CarDetect car;
      if (! PeakDetect::findLastTwoOfFourWheeler(startLocal, endLocal,
          rightFlagLocal, peakNos, peakPtrs, car))
      {
cout << "Failed\n";
        return false;
      }

      PeakDetect::fixTwoWheels(* peakPtrs[0], * peakPtrs[1]);
      peakPtrs[2]->markNoWheel();
      peakPtrs[2]->unselect();
if (peakPtrs.size() != 3)
  cout << "ERRORZ " << peakPtrs.size() << endl;

      PeakDetect::updateCars(cars, car);

      return true;
    }
  }

  cout << "Don't know how to do this yet: " << np << ", notTallCount " <<
    notTallCount << "\n";
  return false;

}


void PeakDetect::findWholeCars(vector<CarDetect>& cars)
{
  // Set up a sliding vector of running peaks.
  vector<list<Peak *>::iterator> runIter;
  vector<Peak *> runPtr;
  auto cit = candidates.begin();
  for (unsigned i = 0; i < 4; i++)
  {
    while (cit != candidates.end() && ! (* cit)->isSelected())
      cit++;

    if (cit == candidates.end())
      THROW(ERR_NO_PEAKS, "Not enough peaks in train");
    else
    {
      runIter.push_back(cit);
      runPtr.push_back(* cit);
    }
  }

  while (cit != candidates.end())
  {
    if (runPtr[0]->isLeftWheel() && runPtr[0]->isLeftBogey() &&
        runPtr[1]->isRightWheel() && runPtr[1]->isLeftBogey() &&
        runPtr[2]->isLeftWheel() && runPtr[2]->isRightBogey() &&
        runPtr[3]->isRightWheel() && runPtr[3]->isRightBogey())
    {
      // Fill out cars.
      cars.emplace_back(CarDetect());
      CarDetect& car = cars.back();

      car.logPeakPointers(
        runPtr[0], runPtr[1], runPtr[2], runPtr[3]);

      car.logCore(
        runPtr[1]->getIndex() - runPtr[0]->getIndex(),
        runPtr[2]->getIndex() - runPtr[1]->getIndex(),
        runPtr[3]->getIndex() - runPtr[2]->getIndex());

      car.logStatIndex(0); 

      // Check for gap in front of run[0].
      unsigned leftGap = 0;
      if (runIter[0] != candidates.begin())
      {
        Peak * prevPtr = * prev(runIter[0]);
        if (prevPtr->isBogey())
        {
          // This rounds to make the cars abut.
          const unsigned d = runPtr[0]->getIndex() - prevPtr->getIndex();
          leftGap  = d - (d/2);
        }
      }

      // Check for gap after run[3].
      unsigned rightGap = 0;
      auto postIter = next(runIter[3]);
      if (postIter != candidates.end())
      {
        Peak * nextPtr = * postIter;
        if (nextPtr->isBogey())
          rightGap = (nextPtr->getIndex() - runPtr[3]->getIndex()) / 2;
      }


      if (! car.fillSides(leftGap, rightGap))
      {
        cout << car.strLimits(offset, "Incomplete car");
        cout << "Could not fill out any limits: " <<
          leftGap << ", " << rightGap << endl;
      }

      models += car;

    }

    for (unsigned i = 0; i < 3; i++)
    {
      runIter[i] = runIter[i+1];
      runPtr[i] = runPtr[i+1];
    }

    do
    {
      cit++;
    } 
    while (cit != candidates.end() && ! (* cit)->isSelected());
    runIter[3] = cit;
    runPtr[3] = * cit;
  }
}


void PeakDetect::findWholeInnerCars(vector<CarDetect>& cars)
{
  const unsigned csize = cars.size(); // As cars grows in the loop
  for (unsigned cno = 0; cno+1 < csize; cno++)
  {
    const unsigned end = cars[cno].endValue();
    const unsigned start = cars[cno+1].startValue();
    if (end == start)
      continue;

    if (PeakDetect::findCars(end, start, true, true, cars))
      PeakDetect::printRange(end, start, "Did intra-gap");
    else
      PeakDetect::printRange(end, start, "Didn't do intra-gap");
  }
}


void PeakDetect::findWholeFirstCar(vector<CarDetect>& cars)
{
  const unsigned u1 = candidates.front()->getIndex();
  const unsigned u2 = cars.front().startValue();
  if (u1 < u2)
  {
    if (PeakDetect::findCars(u1, u2, false, true, cars))
      PeakDetect::printRange(u1, u2, "Did first whole-car gap");
    else
      PeakDetect::printRange(u1, u2, "Didn't do first whole-car gap");
    cout << endl;
  }
}


void PeakDetect::findWholeLastCar(vector<CarDetect>& cars)
{
  const unsigned u1 = cars.back().endValue();
  const unsigned u2 = candidates.back()->getIndex();
  if (u2 > u1)
  {
    if (PeakDetect::findCars(u1, u2, true, false, cars))
      PeakDetect::printRange(u1, u2, "Did last whole-car gap");
    else
      PeakDetect::printRange(u1, u2, "Didn't do last whole-car gap");
    cout << endl;
  }
}


void PeakDetect::updateCarDistances(vector<CarDetect>& cars) const
{
  for (auto& car: cars)
  {
    unsigned index;
    float distance;
    if (! PeakDetect::matchesModel(car, index, distance))
    {
      cout << "WARNING: Car doesn't match any model.\n";
      index = 0;
      distance = 0.f;
    }

    car.logStatIndex(index);
    car.logDistance(distance);
  }
}


void PeakDetect::reduce()
{
  if (peaks.empty())
    return;

  const bool debug = true;
  const bool debugDetails = false;

  if (debugDetails)
    PeakDetect::printAllPeaks("Original peaks");

  PeakDetect::reduceSmallAreas(0.1f);

  if (debugDetails)
    PeakDetect::printAllPeaks("Non-tiny peaks");

  PeakDetect::eliminateKinks();
  if (debugDetails)
    PeakDetect::printAllPeaks("Non-kinky list peaks");

  Peak scale;
  PeakDetect::estimateScale(scale);
  if (debug)
    PeakDetect::printPeak(scale, "Scale");

  PeakDetect::reduceSmallRanges(scale.getRange() / 10.f, true);

  // Mark some tall peaks as seeds.
  PeakSeeds seeds;
  seeds.mark(peaks, offset, scale.getRange());
  cout << seeds.str(offset, "after pruning");

  PeakMinima minima;
  minima.mark(peaks, candidates, offset);

  vector<CarDetect> cars;
  models.append(); // Make room for initial model
  PeakDetect::findWholeCars(cars);

  if (cars.size() == 0)
    THROW(ERR_NO_PEAKS, "No cars?");

  PeakDetect::printCars(cars, "after inner gaps");

  for (auto& car: cars)
    models.fillSides(car);

  // TODO Could actually be multiple cars in vector, e.g. same wheel gaps
  // but different spacing between cars, ICET_DEU_56_N.

  PeakDetect::updateCarDistances(cars);

  PeakDetect::printCars(cars, "before intra gaps");

  // Check open intervals.  Start with inner ones as they are complete.

  PeakDetect::findWholeInnerCars(cars);
  PeakDetect::updateCarDistances(cars);
  sort(cars.begin(), cars.end());

  PeakDetect::printWheelCount("Counting");
  PeakDetect::printCars(cars, "after whole-car gaps");
  PeakDetect::printCarStats("after whole-car inner gaps");

  PeakDetect::findWholeLastCar(cars);
  PeakDetect::updateCarDistances(cars);

  PeakDetect::printWheelCount("Counting");
  PeakDetect::printCars(cars, "after trailing whole car");
  PeakDetect::printCarStats("after trailing whole car");

  PeakDetect::findWholeFirstCar(cars);
  PeakDetect::updateCarDistances(cars);
  sort(cars.begin(), cars.end());

  PeakDetect::printWheelCount("Counting");
  PeakDetect::printCars(cars, "after leading whole car");
  PeakDetect::printCarStats("after leading whole car");


  // TODO Check peak quality and deviations in carStats[0].
  // Detect inner and open intervals that are not done.
  // Could be carStats[0] with the right length etc, but missing peaks.
  // Or could be a new type of car (or multiple cars).
  // Ends come later.

  PeakDetect::printWheelCount("Returning");

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

  THROW(ERR_NO_PEAKS, "No peaks selected");
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


void PeakDetect::printPeak(
  const Peak& peak,
  const string& text) const
{
  cout << text << "\n";
  cout << peak.strHeader();
  cout << peak.str(0) << endl;
}


void PeakDetect::printRange(
  const unsigned start,
  const unsigned end,
  const string& text) const
{
  cout << text << " " << start + offset << "-" << end + offset << endl;
}


void PeakDetect::printWheelCount(const string& text) const
{
  unsigned count = 0;
  for (auto cand: candidates)
  {
    if (cand->isWheel())
      count++;
  }
  cout << text << " " << count << " peaks" << endl;
}


void PeakDetect::printCars(
  const vector<CarDetect>& cars,
  const string& text) const
{
  if (cars.empty())
    return;

  cout << "Cars " << text << "\n";
  cout << cars.front().strHeaderFull();

  for (unsigned cno = 0; cno < cars.size(); cno++)
  {
    cout << cars[cno].strFull(cno, offset);
    if (cno+1 < cars.size() &&
        cars[cno].endValue() != cars[cno+1].startValue())
      cout << string(56, '.') << "\n";
  }
  cout << endl;
}


void PeakDetect::printCarStats(const string& text) const
{
  cout << "Car stats " << text << "\n";
  cout << models.str();
}


void PeakDetect::printPeaksCSV(const vector<PeakTime>& timesTrue) const
{
  cout << "true\n";
  unsigned i = 0;
  for (auto tt: timesTrue)
    cout << i++ << ";" << fixed << setprecision(6) << tt.time << "\n";

  cout << "\nseen\n";
  i = 0;
  for (auto& peak: peaks)
  {
    if (peak.isSelected())
      cout << i++ << ";" << 
        fixed << setprecision(6) << peak.getTime() << "\n";
  }
  cout << endl;
}


void PeakDetect::printAllPeaks(const string& text) const
{
  if (peaks.empty())
    return;

  if (text != "")
    cout << text << ": " << peaks.size() << "\n";
  cout << peaks.front().strHeader();

  for (auto& peak: peaks)
    cout << peak.str(offset);
  cout << endl;
}

