#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakStructure.h"
#include "PeakProfile.h"
#include "CarModels.h"
#include "Except.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

#define GREAT_CAR_DISTANCE 1.5f

static unsigned hitSize;


struct WheelSpec
{
  BogeyType bogey;
  WheelType wheel;
};

static const vector<WheelSpec> wheelSpecs =
{
  {BOGEY_LEFT, WHEEL_LEFT},
  {BOGEY_LEFT, WHEEL_RIGHT},
  {BOGEY_RIGHT, WHEEL_LEFT},
  {BOGEY_RIGHT, WHEEL_RIGHT}
};


PeakStructure::PeakStructure()
{
  PeakStructure::reset();

  findCarFunctions.push_back(
    { &PeakStructure::findCarByOrder, 
      "by 1234 order", 0});
  findCarFunctions.push_back(
    { &PeakStructure::findPartialFirstCarByQuality, 
      "partial by quality", 1});
  findCarFunctions.push_back(
    { &PeakStructure::findCarByGreatQuality, 
      "by great quality", 2});
  findCarFunctions.push_back(
    { &PeakStructure::findCarByGoodQuality, 
      "by good quality", 3});
  findCarFunctions.push_back(
    { &PeakStructure::findCarByGeometry, 
      "by geometry", 4});
  findCarFunctions.push_back(
    { &PeakStructure::findEmptyRange, 
      "by emptiness", 5});
  
  hitSize = 6;
  hits.resize(6);
}


PeakStructure::~PeakStructure()
{
}


void PeakStructure::reset()
{
}


bool PeakStructure::findNumberedWheeler(
  const CarModels& models,
  const PeakRange& range,
  const PeakPtrVector& peakPtrs,
  const unsigned numWheels,
  CarDetect& car) const
{
  if (numWheels == 2)
  {
    car.makeLastTwoOfFourWheeler(range, peakPtrs);
    return models.twoWheelerPlausible(car);
  }
  else if (numWheels == 3)
  {
    car.makeLastThreeOfFourWheeler(range, peakPtrs);
    return models.threeWheelerPlausible(car);
  }
  else if (numWheels == 4)
  {
    car.makeFourWheeler(range, peakPtrs);
    return models.gapsPlausible(car);
  }
  else
    return false;
}


void PeakStructure::markUpPeaks(
  PeakPtrVector& peakPtrsNew,
  const unsigned numPeaks) const
{
  if (numPeaks > 4)
    return;

  // Always take the last numPeaks wheels.
  for (unsigned i = 0; i < numPeaks; i++)
    peakPtrsNew[i]->markBogeyAndWheel(
      wheelSpecs[i+4-numPeaks].bogey, 
      wheelSpecs[i+4-numPeaks].wheel);
}


void PeakStructure::markDownPeaks(PeakPtrVector& peakPtrs) const
{
  for (auto& pp: peakPtrs)
  {
    pp->markNoBogey();
    pp->markNoWheel();
    pp->unselect();
  }
}


void PeakStructure::updateCarDistances(
  const CarModels& models,
  list<CarDetect>& cars) const
{
  for (auto& car: cars)
  {
    unsigned index;
    float distance;
    if (! models.matchesDistance(car, GREAT_CAR_DISTANCE, distance, index))
    {
      cout << "WARNING: Car doesn't match any model.\n";
      index = 0;
      distance = 0.f;
    }

    car.logStatIndex(index);
    car.logDistance(distance);
  }
}


void PeakStructure::updateModels(
  CarModels& models,
  CarDetect& car) const
{
  unsigned index;
  float distance;

  if (models.matchesDistance(car, GREAT_CAR_DISTANCE, distance, index))
  {
    car.logStatIndex(index);
    car.logDistance(distance);

    models.add(car, index);
    cout << "Recognized model " << index << endl;
  }
  else
  {
    car.logStatIndex(models.size());

    models.append();
    models += car;
    cout << "Created new model\n";
  }
}


bool PeakStructure::isConsistent(const PeakPtrVector& closestPeaks) const
{
  for (unsigned i = 0; i < 4; i++)
  {
    if (! closestPeaks[i]->fitsType(
        wheelSpecs[i].bogey, wheelSpecs[i].wheel))
      return false;
  }
  return true;
}


bool PeakStructure::fillPartialSides(
  CarModels& models,
  CarDetect& car1,
  CarDetect& car2) const
{
  // Some cars may have been partial (i.e., missing a side gap) on 
  // either or both sides.
  // Now that we have filled in the inner cars, we can guess those
  // gaps and also check the partial car types.

  if (car1.hasRightGap() && car2.hasLeftGap())
    return false;

  const unsigned lpp1 = car1.lastPeakPlus1();
  const unsigned fpm1 = car2.firstPeakMinus1();
  if (fpm1 - lpp1 > car1.getMidGap())
    return false;

  const unsigned mid = (lpp1 + fpm1) / 2;

  car1.setEndAndGap(mid);
  car2.setStartAndGap(mid);

  PeakStructure::updateModels(models, car1);
  PeakStructure::updateModels(models, car2);
  return true;
}


bool PeakStructure::isWholeCar(const PeakPtrVector& pv) const
{
  bool isLeftPair = (pv[0]->isLeftWheel() && pv[1]->isRightWheel());
  bool isRightPair = (pv[2]->isLeftWheel() && pv[3]->isRightWheel());
  bool isLeftBogey = (pv[0]->isLeftBogey() && pv[1]->isLeftBogey());
  bool isRightBogey = (pv[2]->isRightBogey() && pv[3]->isRightBogey());

  // True if we have two pairs, and at least one of them is
  // recognized as a sided bogey.
  return (isLeftPair && isRightPair && (isLeftBogey || isRightBogey));
}


PeakStructure::FindCarType PeakStructure::findCarByOrder(
  const CarModels& models,
  const PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  UNUSED(models);
  UNUSED(peaks);

  if (range.looksEmpty())
    return FIND_CAR_NO_MATCH;

  // Set up a sliding vector of 4 running peaks.
  PeakPtrVector runPtr;

  auto cit = * range.begin();
  for (unsigned i = 0; i < 4; i++)
    runPtr.push_back(* cit);

  PeakRange rangeLocal;

  for (auto pit: range)
  {
    if (! (* pit)->isSelected())
      continue;

    for (unsigned i = 0; i < 3; i++)
      runPtr[i] = runPtr[i+1];

    runPtr[3] = * pit;

    if (PeakStructure::isWholeCar(runPtr))
    {
      // We deal with the edges later.
      rangeLocal.init(runPtr);
      car.makeFourWheeler(rangeLocal, runPtr);

      PeakStructure::markUpPeaks(runPtr, 4);

      return FIND_CAR_MATCH;
    }
  }

  return FIND_CAR_NO_MATCH;
}


PeakStructure::FindCarType PeakStructure::findPartialCarByQuality(
  const CarModels& models,
  const PeakFncPtr& fptr,
  const unsigned numWheels,
  PeakRange& range,
  CarDetect& car) const
{
  PeakPtrVector peakPtrsUsed, peakPtrsUnused;
  range.splitByQuality(fptr, peakPtrsUsed, peakPtrsUnused);

  if (peakPtrsUsed.size() != numWheels)
    THROW(ERR_ALGO_PEAK_STRUCTURE, 
      "Not " + to_string(numWheels) + " peaks after all");

  if (PeakStructure::findNumberedWheeler(models, range,
      peakPtrsUsed, numWheels, car))
  {
    PeakStructure::markUpPeaks(peakPtrsUsed, numWheels);
    PeakStructure::markDownPeaks(peakPtrsUnused);
    return FIND_CAR_MATCH;
  }
  else
    return FIND_CAR_NO_MATCH;
}


PeakStructure::FindCarType PeakStructure::findPartialFirstCarByQuality(
  const CarModels& models,
  const PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  UNUSED(peaks);

  if (! range.isFirstCar())
    return FIND_CAR_NO_MATCH;

  if (range.numGreat() == 3)
  {
    return PeakStructure::findPartialCarByQuality(models,
        &Peak::greatQuality, 3, range, car);
  }
  else if (range.numGood() == 3)
  {
    return PeakStructure::findPartialCarByQuality(models,
        &Peak::goodQuality, 3, range, car);
  }
  else if (range.numGreat() == 2)
  {
    return PeakStructure::findPartialCarByQuality(models,
        &Peak::greatQuality, 2, range, car);
  }
  else
    return FIND_CAR_NO_MATCH;
}


PeakStructure::FindCarType PeakStructure::findCarByQuality(
  const CarModels& models,
  const PeakFncPtr& fptr,
  PeakRange& range,
  CarDetect& car) const
{
  PeakPtrVector peakPtrsUsed, peakPtrsUnused;
  range.splitByQuality(fptr, peakPtrsUsed, peakPtrsUnused);

  if (peakPtrsUsed.size() != 4)
    THROW(ERR_ALGO_PEAK_STRUCTURE, "Not 4 peaks after all");

  if (PeakStructure::findNumberedWheeler(models, range,
      peakPtrsUsed, 4, car))
  {
    PeakStructure::markUpPeaks(peakPtrsUsed, 4);
    PeakStructure::markDownPeaks(peakPtrsUnused);
    return FIND_CAR_MATCH;
  }
  else
    return FIND_CAR_NO_MATCH;
}


PeakStructure::FindCarType PeakStructure::findCarByGreatQuality(
  const CarModels& models,
  const PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  UNUSED(peaks);
  if (range.numGreat() != 4)
    return FIND_CAR_NO_MATCH;
  else
    return PeakStructure::findCarByQuality(models,
      &Peak::greatQuality, range, car);
}


PeakStructure::FindCarType PeakStructure::findCarByGoodQuality(
  const CarModels& models,
  const PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  UNUSED(peaks);
  if (range.numGood() != 4)
    return FIND_CAR_NO_MATCH;
  else
    return PeakStructure::findCarByQuality(models,
      &Peak::goodQuality, range, car);
}


PeakStructure::FindCarType PeakStructure::findCarByGeometry(
  const CarModels& models,
  const PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  CarDetect carModel;
  list<unsigned> carPoints;
  PeakPtrVector closestPeaks, skippedPeaks;
  unsigned matchIndex;
  float distance;
  PeakRange rangeLocal;

  for (unsigned mno = 0; mno < models.size(); mno++)
  {
    models.getCar(carModel, mno);

    // 6 elements: Left edge, 4 wheels, right edge.
    carModel.getCarPoints(carPoints);

    for (auto pit: range)
    {
      if (! (* pit)->goodQuality())
        continue;

      // pit corresponds to the first wheel.
      if (! peaks.getClosest(carPoints, &Peak::goodQuality, pit, 4,
          closestPeaks, skippedPeaks))
        continue;

      // We deal with the edges later.
      rangeLocal.init(closestPeaks);
      car.makeFourWheeler(rangeLocal, closestPeaks);
      if (! models.gapsPlausible(car))
        continue;
      
      if (! models.matchesDistance(car, GREAT_CAR_DISTANCE, 
          distance, matchIndex))
        continue;
      
      if (matchIndex != mno)
        continue;

      if (! PeakStructure::isConsistent(closestPeaks))
      {
        cout << "WARNING: Peaks inconsistent with car #" <<
          matchIndex << " found (distance " << distance << ")\n";
        cout << range.strFull(offset) << endl;
      }

      PeakStructure::markUpPeaks(closestPeaks, 4);
      PeakStructure::markDownPeaks(skippedPeaks);

      return FIND_CAR_MATCH;
    }
  }
  return FIND_CAR_NO_MATCH;
}


PeakStructure::FindCarType PeakStructure::findEmptyRange(
  const CarModels& models,
  const PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  UNUSED(models);
  UNUSED(peaks);
  UNUSED(car);

  if (range.looksEmpty())
  {
    PeakStructure::markDownPeaks(range.getPeakPtrs());
    return FIND_CAR_DOWNGRADE;
  }
  else
    return FIND_CAR_NO_MATCH;
}



list<PeakRange>::iterator PeakStructure::updateRanges(
  CarModels& models,
  const list<CarDetect>& cars,
  const CarListIter& carIt,
  list<PeakRange>::iterator& rit,
  const FindCarType& findFlag)
{
  PeakRange& range = * rit;

  if (findFlag == FIND_CAR_DOWNGRADE)
    return ranges.erase(rit);

  CarDetect& car = * carIt;
  if (carIt != cars.begin())
    PeakStructure::fillPartialSides(models, * prev(carIt), car);

  auto nextCarIt = next(carIt);
  if (nextCarIt != cars.end())
    PeakStructure::fillPartialSides(models, car, * nextCarIt);

  if (car.hasLeftGap() || car.startValue() == range.startValue())
  {
    if (car.hasRightGap() || car.endValue() == range.endValue())
      return ranges.erase(rit);
    else
    {
      // Shorten the range on the left to make room for the new
      // car preceding it.
      // This does not change any carAfter values.
      range.shortenLeft(car);
      return ++rit;
    }
  }
  else if (car.hasRightGap() || car.endValue() == range.endValue())
  {
    // Shorten the range on the right to make room for the new
    // car following it.
    range.shortenRight(car, carIt);
    return ++rit;
  }
  else
  {
    // Recognized a car in the middle of the range.
    // The new order is rangeNew - car - range.
    PeakRange rangeNew = range;
    range.shortenLeft(car);
    rangeNew.shortenRight(car, carIt);
    return ranges.insert(rit, rangeNew);
  }
}


void PeakStructure::markCars(
  CarModels& models,
  list<CarDetect>& cars,
  PeakPool& peaks,
  const unsigned offsetIn)
{
  offset = offsetIn;

  ranges.clear();
  ranges.emplace_back(PeakRange());
  ranges.back().init(cars, peaks);

  CarDetect car;

  for (unsigned i = 0; i < 6; i++)
    hits[i] = 0;

  bool changeFlag = true;
  while (changeFlag && ! ranges.empty())
  {
    changeFlag = false;
    auto rit = ranges.begin();
    while (rit != ranges.end())
    {
      PeakRange& range = * rit;

      cout << "Range: " << range.strFull(offset);

      // Set up some useful stuff for all recognizers.
      range.fill(peaks);

      FindCarType findFlag = FIND_CAR_SIZE;
      for (auto& fgroup: findCarFunctions)
      {
        findFlag = (this->* fgroup.fptr)(models, peaks, range, car);
        if (findFlag != FIND_CAR_NO_MATCH)
        {
          cout << "Hit " << fgroup.name << "\n";
          cout << range.strFull(offset);
          hits[fgroup.number]++;
          break;
        }
      }

      if (findFlag == FIND_CAR_NO_MATCH)
      {
        rit++;
        continue;
      }

      changeFlag = true;

      CarListIter newcit;
      if (findFlag == FIND_CAR_MATCH)
      {
        PeakStructure::updateModels(models, car);
        newcit = cars.insert(range.carAfterIter(), car);
        PeakStructure::updateCarDistances(models, cars);
      }

      rit = PeakStructure::updateRanges(models, cars, newcit, 
        rit, findFlag);

      PeakStructure::printWheelCount(peaks, "Counting");
      PeakStructure::printCars(cars, "after range");
      PeakStructure::printCarStats(models, "after range");
    }
  }

  if (! ranges.empty())
  {
    cout << "WARNFINAL: " << ranges.size() << " ranges left\n";
    for (auto& range: ranges)
    {
      cout << range.strFull(offset);
      cout << range.strProfile();
    }
    cout << endl;
  }

  cout<< "HITS\n";
  for (unsigned i = 0; i < 6; i++)
    cout << i << " " << hits[i] << endl;
  cout << endl;

}


bool PeakStructure::markImperfections(
  const list<CarDetect>& cars,
  Imperfections& imperf) const
{
  // Based on the cars we recognized, make an educated guess of the
  // number of peaks of different types that we may not have got
  // exactly right.
  
  // For now we assume cars of exactly 4 peaks each.
  // This doesn't really work well yet.
  // TODO

  imperf.reset();
  for (auto& range: ranges)
    range.updateImperfections(cars, imperf);

  cout << "IMPERF " <<
    imperf.numSkipsOfReal << "-" << imperf.numSkipsOfSeen << ", " <<
    imperf.numSpuriousLater << "-" << imperf.numMissingLater << endl;
  return true;
}


void PeakStructure::printWheelCount(
  const PeakPool& peaks,
  const string& text) const
{
  cout << text << " " << 
    peaks.countCandidates(&Peak::isSelected) << 
    " peaks" << endl;
}


void PeakStructure::printCars(
  const list<CarDetect>& cars,
  const string& text) const
{
  if (cars.empty())
    return;

  cout << "Cars " << text << "\n";
  cout << cars.front().strHeaderFull();

  unsigned cno = 0;
  for (auto cit = cars.begin(); cit != cars.end(); cit++)
  {
    cout << cit->strFull(cno++, offset);
    auto ncit = next(cit);
    if (ncit == cars.end())
      break;
    else if (cit->endValue() != ncit->startValue())
      cout << string(56, '.') << "\n";
  }
  cout << endl;
}


void PeakStructure::printCarStats(
  const CarModels& models,
  const string& text) const
{
  cout << "Car stats " << text << "\n";
  cout << models.str();
}

