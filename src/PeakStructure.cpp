#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakStructure.h"
#include "PeakPattern.h"
#include "PeakShort.h"
#include "PeakRepair.h"
#include "PeakProfile.h"
#include "CarModels.h"
#include "Except.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

#define GOOD_CAR_DISTANCE 1.5f
#define GREAT_CAR_DISTANCE 0.5f

#define NUM_METHODS 10
#define NUM_METHOD_GROUPS 3


struct WheelSpec
{
  BogieType bogie;
  WheelType wheel;
};

static const vector<WheelSpec> wheelSpecs =
{
  {BOGIE_LEFT, WHEEL_LEFT},
  {BOGIE_LEFT, WHEEL_RIGHT},
  {BOGIE_RIGHT, WHEEL_LEFT},
  {BOGIE_RIGHT, WHEEL_RIGHT}
};


PeakStructure::PeakStructure()
{
  PeakStructure::reset();

  findMethods.resize(NUM_METHOD_GROUPS);

  // 4 wheels, of which at least 2 are labeled as bogies (ideally 1234).
  findMethods[0].push_back(
    { &PeakStructure::findCarByOrder, 
      "by 1234 order", 0});

  // Exactly 4 great peaks that look like a car.
  findMethods[1].push_back(
    { &PeakStructure::findCarByGreatQuality, 
      "by great quality", 1});

  // Exactly 4 good peaks that look like a car.
  findMethods[1].push_back(
    { &PeakStructure::findCarByGoodQuality, 
      "by good quality", 2});

  // No great peaks at all.
  findMethods[1].push_back(
    { &PeakStructure::findEmptyRange, 
      "by emptiness", 4});

  // Car-sized gaps based on existing models.
  findMethods[1].push_back(
    { &PeakStructure::findCarByPattern, 
      "by pattern", 3});

  findMethods[2].push_back(
    { &PeakStructure::findPartialFirstCarByQuality, 
      "first partial by quality", 5});

  hits.resize(NUM_METHODS);
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
  PeakPtrs& peakPtrs,
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
    if (! car.corePlausible())
      return false;
    return models.gapsPlausible(car);
  }
  else
    return false;
}


void PeakStructure::updateCarDistances(
  const CarModels& models,
  list<CarDetect>& cars) const
{
  // TODO Move to car.updateDistance.  Warning shouldn't happen?
  MatchData match;
  for (auto& car: cars)
  {
    if (! models.matchesDistance(car, GOOD_CAR_DISTANCE, false, match))
    {
      cout << "WARNING: Car doesn't match any model.\n";
      match.distance = numeric_limits<float>::max();
    }

    car.logMatchData(match);
  }
}


void PeakStructure::updateModels(
  CarModels& models,
  CarDetect& car) const
{
  MatchData match;

  if (models.matchesDistance(car, GOOD_CAR_DISTANCE, false, match))
  {
    car.logMatchData(match);

    models.add(car, match.index);
    cout << "Recognized model " << match.strIndex() << endl;
  }
  else
  {
    match.distance = 0.f;
    match.index = models.available();
    match.reverseFlag = false;
    car.logMatchData(match);

    models.add(car, match.index);
    cout << "Created new model\n";
  }
}


bool PeakStructure::fillPartialSides(
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
  const unsigned mg1 = car1.getMidGap();
  if (fpm1 - lpp1 > (mg1 == 0 ? car2.getMidGap() : mg1))
    return false;

  const unsigned mid = (lpp1 + fpm1) / 2;

  car1.setEndAndGap(mid);
  car2.setStartAndGap(mid);
  return true;
}


PeakStructure::FindCarType PeakStructure::findCarByOrder(
  const CarModels& models,
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  UNUSED(models);
  UNUSED(peaks);

  if (range.looksEmpty())
    return FIND_CAR_NO_MATCH;

  // Set up a sliding vector of 4 running peaks.
  PeakPtrs peakPtrsUsed;
  peakPtrsUsed.assign(4, ** range.begin());

  PeakRange rangeLocal;

  for (auto pit: range)
  {
    if (! (* pit)->isSelected())
      continue;

    peakPtrsUsed.shift_down(* pit);

    if (peakPtrsUsed.isFourWheeler())
    {
      // We deal with the edges later.
      rangeLocal.init(peakPtrsUsed);
      car.makeFourWheeler(rangeLocal, peakPtrsUsed);

      peakPtrsUsed.markup();

      return FIND_CAR_MATCH;
    }
  }

  return FIND_CAR_NO_MATCH;
}


PeakStructure::FindCarType PeakStructure::findPartialCarByQuality(
  const CarModels& models,
  const PeakFncPtr& fptr,
  const CarPosition carpos,
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  PeakPtrs peakPtrsUsed, peakPtrsUnused;
  range.split(fptr, peakPtrsUsed, peakPtrsUnused);

  if (peakPtrsUsed.size() == 0)
    return FIND_CAR_NO_MATCH;

// cout << "PP BEFORE\n";
// for (auto& peak: peakPtrsUsed)
  // cout << peak->strQuality(offset);

const unsigned numOrig = peakPtrsUsed.size();

  PeakRepair repair;
  if (repair.edgeCar(models, offset, carpos, 
      peaks, range, peakPtrsUsed, peakPtrsUnused))
  {
    if (carpos == CARPOSITION_FIRST)
      cout << "Hit an anywheeler\n";
    else if (carpos == CARPOSITION_INNER_SINGLE ||
             carpos == CARPOSITION_INNER_MULTI)
    {

cout << "Hit a midwheeler\n";
unsigned num = 0;
for (auto& peak: peakPtrsUsed)
{
  if (peak != nullptr)
    num++;
}
cout << "PP AFTER " << numOrig << " " << num << "\n";
  /*
for (auto& peak: peakPtrsUsed)
  if (peak)
    cout << peak->strQuality(offset);
  cout << "MIDCOUNT " << num << "\n";
  */

if (carpos == CARPOSITION_INNER_MULTI)
{
  if (num == 4)
    cout << "MMULT hit\n";
  else
    cout << "MMULT nohit\n";
}


      if (num != 4)
        return FIND_CAR_NO_MATCH;
    }
    else
      cout << "Hit a lastwheeler\n";


    if (carpos == CARPOSITION_INNER_MULTI)
    {
      PeakRange rangeLocal;
      rangeLocal.init(peakPtrsUsed);
      car.makeAnyWheeler(rangeLocal, peakPtrsUsed);

      peakPtrsUsed.markup();
      peakPtrsUnused.apply(&Peak::markdown);
      return FIND_CAR_MATCH;
    }
    else
    {
      car.makeAnyWheeler(range, peakPtrsUsed);
      peakPtrsUsed.markup();
      peakPtrsUnused.apply(&Peak::markdown);
      return FIND_CAR_PARTIAL;
    }
  }
  else
    return FIND_CAR_NO_MATCH;
}


PeakStructure::FindCarType PeakStructure::findPartialFirstCarByQuality(
  const CarModels& models,
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  if (! range.isFirstCar())
    return FIND_CAR_NO_MATCH;

  if (range.numGreat() > 4)
    return FIND_CAR_NO_MATCH;

  // Have to refill range if we pruned transients.
  if (peaks.pruneTransients(range.endValue()))
    range.fill(peaks);

  // TODO Should the next car then actively become the first one?
  if (range.numGood() == 0)
    return FIND_CAR_DOWNGRADE;

  return PeakStructure::findPartialCarByQuality(models, 
      &Peak::goodQuality, CARPOSITION_FIRST, peaks, range, car);
}


PeakStructure::FindCarType PeakStructure::findCarByPeaks(
  const CarModels& models,
  const PeakRange& range,
  PeakPtrs& peakPtrs,
  CarDetect& car) const
{
  if (PeakStructure::findNumberedWheeler(models, range, peakPtrs, 4, car))
  {
    // We deal with the edges later.
    PeakRange rangeLocal;
    rangeLocal.init(peakPtrs);
    car.makeFourWheeler(rangeLocal, peakPtrs);

    peakPtrs.markup();

    return FIND_CAR_MATCH;
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
  PeakPtrs peakPtrsUsed, peakPtrsUnused;
  range.split(fptr, peakPtrsUsed, peakPtrsUnused);

  if (PeakStructure::findCarByPeaks(models, range, peakPtrsUsed, car) ==
      FIND_CAR_MATCH)
    return FIND_CAR_MATCH;

  return FIND_CAR_NO_MATCH;
}


PeakStructure::FindCarType PeakStructure::findCarByGreatQuality(
  const CarModels& models,
  PeakPool& peaks,
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
  PeakPool& peaks,
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


PeakStructure::FindCarType PeakStructure::findEmptyRange(
  const CarModels& models,
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  UNUSED(models);
  UNUSED(peaks);
  UNUSED(car);

  if (range.looksEmpty() || range.looksEmptyLast())
  {
    range.getPeakPtrs().apply(&Peak::markdown);
    return FIND_CAR_DOWNGRADE;
  }
  else
    return FIND_CAR_NO_MATCH;
}


PeakStructure::FindCarType PeakStructure::findCarByPattern(
  const CarModels& models,
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  // These can in general cover more time than the car we find.
  PeakPtrs peakPtrsUsed, peakPtrsUnused;
  range.split(&Peak::goodQuality, peakPtrsUsed, peakPtrsUnused);

  PeakPattern pattern;
  if (! pattern.locate(models, peaks, range, offset,
      peakPtrsUsed, peakPtrsUnused))
    return FIND_CAR_NO_MATCH;

// cout << "findCarByPattern:\n";
// cout << peakPtrsUsed.strQuality("Used", offset);
// cout << peakPtrsUnused.strQuality("Unused", offset);

  // If it worked, the used and unused peaks have been modified,
  // and some peaks have disappeared from the union of the lists.
  PeakRange rangeLocal;
  rangeLocal.init(peakPtrsUsed);
  car.makeFourWheeler(rangeLocal, peakPtrsUsed);

  peakPtrsUsed.markup();
  peakPtrsUnused.apply(&Peak::markdown);

  return FIND_CAR_MATCH;
}


CarListIter PeakStructure::updateRecords(
  const PeakRange& range,
  const CarDetect& car,
  CarModels& models,
  list<CarDetect>& cars)
{
  // TODO There is too much updating going on here.

  CarListIter newcit = cars.insert(range.carAfterIter(), car);

  if (newcit != cars.begin())
  {
    auto prevcit = prev(newcit);
    PeakStructure::fillPartialSides(* prevcit, * newcit);
    if (prevcit->hasRightGap())
      PeakStructure::updateModels(models, * prevcit);
  }

  auto nextCarIt = next(newcit);
  if (nextCarIt != cars.end())
  {
    const bool hasLeft = nextCarIt->hasLeftGap();
    PeakStructure::fillPartialSides(* newcit, * nextCarIt);
    if (nextCarIt->hasLeftGap() != hasLeft)
      PeakStructure::updateModels(models, * nextCarIt);
  }

  PeakStructure::updateModels(models, * newcit);

  models.recalculate(cars);

  PeakStructure::updateCarDistances(models, cars);

  return newcit;
}


list<PeakRange>::iterator PeakStructure::updateRanges(
  const CarListIter& carIt,
  list<PeakRange>::iterator& rit,
  const FindCarType& findFlag)
{
  if (findFlag == FIND_CAR_DOWNGRADE ||
      findFlag == FIND_CAR_PARTIAL)
    return ranges.erase(rit);

  PeakRange& range = * rit;
  const CarDetect& car = * carIt;
  if (car.hasLeftGap() || car.startValue() <= range.startValue())
  {
    if (car.hasRightGap() || car.endValue() >= range.endValue())
      return ranges.erase(rit);
    else
    {
      // Shorten the range on the left to make room for the new
      // car preceding it.
      // This does not change any carAfter values.
      range.shortenLeft(car);
      return rit;
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


bool PeakStructure::loopOverMethods(
  CarModels& models,
  list<CarDetect>& cars,
  PeakPool& peaks,
  const list<FncGroup>& findCarMethods,
  const bool onceFlag)
{
  if (ranges.empty())
    return false;

  CarDetect car;
  bool anyChangeFlag = false;

  auto rit = ranges.begin();
  auto ritFirstAfterChange = ranges.begin();
  while (true)
  {
    PeakRange& range = * rit;
    cout << "Range: " << range.strFull(offset);

    // Set up some useful stuff for all recognizers.
    range.fill(peaks);

    FindCarType findFlag = FIND_CAR_SIZE;
    for (auto& fgroup: findCarMethods)
    {
      cout << "Try " << fgroup.name << endl;
      findFlag = (this->* fgroup.fptr)(models, peaks, range, car);
      if (findFlag != FIND_CAR_NO_MATCH)
      {
        cout << "Hit " << fgroup.name << endl;
        cout << range.strFull(offset);
        if (findFlag != FIND_CAR_PARTIAL || range.isFirstCar())
          hits[fgroup.number]++;
        anyChangeFlag = true;
        break;
      }
    }

    if (findFlag == FIND_CAR_NO_MATCH)
    {
      rit++;
      if (rit == ranges.end())
        rit = ranges.begin();

      // If we've gone full circle, there's nothing more to be had.
      if (rit == ritFirstAfterChange)
        break;
      else
        continue;
    }

    CarListIter newcit;
    if (findFlag == FIND_CAR_MATCH ||
        findFlag == FIND_CAR_PARTIAL)
      newcit = PeakStructure::updateRecords(range, car, models, cars);

    rit = PeakStructure::updateRanges(newcit, rit, findFlag);

    PeakStructure::printWheelCount(peaks, "Counting");
    PeakStructure::printCars(cars, "after range");
    PeakStructure::printCarStats(models, "after range");

    if (ranges.empty())
      break;
    else if (onceFlag)
      break;
    else if (rit == ranges.end())
      rit = ranges.begin();

    ritFirstAfterChange = rit;
  }
  return anyChangeFlag;
}


void PeakStructure::fixSpuriousInterPeaks(
  const list<CarDetect>& cars,
  PeakPool& peaks) const
{
  // It can happen that there are spurious peaks between the starting
  // and/or ending bogie of a car and that car's start or end.
  // We could potentially do this, more efficiently, by storing
  // iterators in the cars, and then looping directly when we
  // fill the ends.

  CarListConstIter cit = cars.cbegin();
  PeakPtrs& candidates = peaks.candidates();
  PPLiterator pbegin = candidates.begin();
  PPLiterator pend = candidates.end();
  PPLiterator pit = pbegin;

  while (pit != pend && cit != cars.cend())
  {
    const unsigned pindex = (* pit)->getIndex();
    const unsigned cstart = cit->startValue();
    const unsigned cfirst = cit->firstPeakMinus1();
    const unsigned clast = cit->lastPeakPlus1();
    const unsigned cend = cit->endValue();

    if (! (* pit)->isSelected() || 
        pindex < cstart ||
        (pindex > cfirst && pindex < clast))
      pit++;
    else if (pindex <= cfirst ||
        (pindex >= clast && pindex <= cend))
    {
      (* pit)->markNoBogie();
      (* pit)->markNoWheel();
      (* pit)->unselect();
      pit++;
    }
    else
      cit++;
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
  ranges.back().init(cars, peaks.candidates());

  for (unsigned i = 0; i < NUM_METHODS; i++)
    hits[i] = 0;

  // The first group only needs to be run once.
  PeakStructure::loopOverMethods(models, cars, peaks, findMethods[0], false);

  while (true)
  {
    bool foundFlag = false;
    for (unsigned mg = 1; mg < NUM_METHOD_GROUPS; mg++)
    {
      bool loopFlag = PeakStructure::loopOverMethods(models, cars, peaks, 
          findMethods[mg], mg == 3); // As little geometry as possible
      if (loopFlag)
        foundFlag = true;
      if (ranges.empty())
        break;
      if (mg > 1 && loopFlag)
      {
        // Go back as soon as a method group hits.
        break;
      }
    }

    if (! foundFlag || ranges.empty())
      break;
  }


  PeakStructure::fixSpuriousInterPeaks(cars, peaks);

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

cout << peaks.candidates().strQuality(
  "All selected peaks at end of PeakStructure", offset, &Peak::isSelected);


  cout << "HITS\n";
  for (unsigned i = 0; i < NUM_METHODS; i++)
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
    peaks.candidatesConst().count(&Peak::isSelected) << " peaks" << endl;
}


void PeakStructure::printCars(
  const list<CarDetect>& cars,
  const string& text) const
{
  if (cars.empty())
    return;

  cout << "Cars " << text << "\n";
  cout << cars.front().strHeaderFull();

  unsigned cno = 1;
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

