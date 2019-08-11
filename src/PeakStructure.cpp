#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "stats/CountStats.h"

#include "PeakStructure.h"
#include "PeakPattern.h"
#include "PeakSpacing.h"
#include "PeakProfile.h"
#include "PeakPool.h"
#include "CarModels.h"
#include "Except.h"

extern CountStats warnStats;
extern CountStats partialStats;
extern CountStats carMethodStats;
extern CountStats modelCountStats;
extern CountStats overallStats;

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

#define GOOD_CAR_DISTANCE 1.5f
#define GREAT_CAR_DISTANCE 0.5f

#define NUM_METHODS 5
#define NUM_METHOD_GROUPS 2


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

  // Exactly 4 peaks that look like a car by quality.
  findMethods[1].push_back(
    { &PeakStructure::findCarByQuality, 
      "by great quality", 1});

  // No great peaks at all.
  findMethods[1].push_back(
    { &PeakStructure::findEmptyRange, 
      "by emptiness", 2});

  // Car-sized gaps based on existing models.
  findMethods[1].push_back(
    { &PeakStructure::findCarByPattern, 
      "by pattern", 3});

  // Car-sized gaps based on existing models.
  findMethods[1].push_back(
    { &PeakStructure::findCarBySpacing, 
      "by spacing", 4});
}


PeakStructure::~PeakStructure()
{
}


void PeakStructure::reset()
{
  models.reset();
  cars.clear();
}


bool PeakStructure::findNumberedWheeler(
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


void PeakStructure::updateCarDistances()
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


void PeakStructure::updateModels(CarDetect& car)
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


FindCarType PeakStructure::findCarByOrder(
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
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
      if (! car.midGapPlausible())
        continue;

      peakPtrsUsed.markup();

      return FIND_CAR_MATCH;
    }
  }

  return FIND_CAR_NO_MATCH;
}


FindCarType PeakStructure::findCarByPeaks(
  const PeakRange& range,
  PeakPtrs& peakPtrs,
  CarDetect& car) const
{
  if (PeakStructure::findNumberedWheeler(range, peakPtrs, 4, car))
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


FindCarType PeakStructure::findCarByQuality(
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  UNUSED(peaks);
  if (range.numGreat() != 4)
    return FIND_CAR_NO_MATCH;

  PeakPtrs peakPtrsUsed, peakPtrsUnused;
  range.split(&Peak::greatQuality, peakPtrsUsed, peakPtrsUnused);

  if (PeakStructure::findCarByPeaks(range, peakPtrsUsed, car) ==
      FIND_CAR_MATCH)
    return FIND_CAR_MATCH;

  return FIND_CAR_NO_MATCH;
}


FindCarType PeakStructure::findEmptyRange(
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  UNUSED(peaks);
  UNUSED(car);

cout << "findEmptyRange" << endl;
cout << range.strFull(offset);
cout << range.strProfile();

  if (range.looksEmpty() || range.looksEmptyLast())
  {
    range.getPeakPtrs().apply(&Peak::markdown);
    return FIND_CAR_DOWNGRADE;
  }
  else
    return FIND_CAR_NO_MATCH;
}


FindCarType PeakStructure::findCarByPattern(
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  // These can in general cover more time than the car we find.
  PeakPtrs peakPtrsUsed, peakPtrsUnused;
  range.split(&Peak::goodQuality, peakPtrsUsed, peakPtrsUnused);

  PeakPattern pattern;
  FindCarType ret = pattern.locate(models, range, offset,
      peaks, peakPtrsUsed, peakPtrsUnused);

  if (ret == FIND_CAR_NO_MATCH)
  {
    return FIND_CAR_NO_MATCH;
  }
  else if (ret == FIND_CAR_DOWNGRADE)
  {
    peakPtrsUnused.apply(&Peak::markdown);
    return FIND_CAR_DOWNGRADE;
  }
  else if (ret == FIND_CAR_MATCH)
  {
    // If it worked, the used and unused peaks have been modified,
    // and some peaks have disappeared from the union of the lists.
    PeakRange rangeLocal;
    rangeLocal.init(peakPtrsUsed);
    car.makeFourWheeler(rangeLocal, peakPtrsUsed);

    peakPtrsUsed.markup();
    peakPtrsUnused.apply(&Peak::markdown);

    return FIND_CAR_MATCH;
  }
  else if (ret == FIND_CAR_PARTIAL)
  {
    car.makeAnyWheeler(range, peakPtrsUsed);
    peakPtrsUsed.markup();
    peakPtrsUnused.apply(&Peak::markdown);
    return FIND_CAR_PARTIAL;
  }
  else
    return FIND_CAR_NO_MATCH;
}


FindCarType PeakStructure::findCarBySpacing(
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  // These can in general cover more time than the car we find.
  PeakPtrs peakPtrsUsed, peakPtrsUnused;
  range.split(&Peak::goodQuality, peakPtrsUsed, peakPtrsUnused);

  PeakSpacing spacing;
  if (! spacing.locate(models, peaks, range, offset,
      peakPtrsUsed, peakPtrsUnused))
  {
    return FIND_CAR_NO_MATCH;
  }
  else if (peakPtrsUsed.empty())
  {
    peakPtrsUnused.apply(&Peak::markdown);
    return FIND_CAR_DOWNGRADE;
  }
  else
  {
    // If it worked, the used and unused peaks have been modified,
    // and some peaks have disappeared from the union of the lists.
    PeakRange rangeLocal;
    rangeLocal.init(peakPtrsUsed);
    car.makeFourWheeler(rangeLocal, peakPtrsUsed);

    peakPtrsUsed.markup();
    peakPtrsUnused.apply(&Peak::markdown);

    return FIND_CAR_MATCH;
  }
}


CarListIter PeakStructure::updateRecords(
  const PeakRange& range,
  const CarDetect& car)
{
  // TODO There is too much updating going on here.

  CarListIter newcit = cars.insert(range.carAfterIter(), car);

  if (newcit != cars.begin())
  {
    auto prevcit = prev(newcit);
    PeakStructure::fillPartialSides(* prevcit, * newcit);
    if (prevcit->hasRightGap())
      PeakStructure::updateModels(* prevcit);
  }

  auto nextCarIt = next(newcit);
  if (nextCarIt != cars.end())
  {
    const bool hasLeft = nextCarIt->hasLeftGap();
    PeakStructure::fillPartialSides(* newcit, * nextCarIt);
    if (nextCarIt->hasLeftGap() != hasLeft)
      PeakStructure::updateModels(* nextCarIt);
  }

  PeakStructure::updateModels(* newcit);

  models.recalculate(cars);

  PeakStructure::updateCarDistances();

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
      findFlag = (this->* fgroup.fptr)(peaks, range, car);
      if (findFlag != FIND_CAR_NO_MATCH)
      {
        cout << "Hit " << fgroup.name << endl;
        cout << range.strFull(offset);
        if (findFlag != FIND_CAR_PARTIAL || range.isFirstCar())
          carMethodStats.log(fgroup.name);
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
      newcit = PeakStructure::updateRecords(range, car);

    rit = PeakStructure::updateRanges(newcit, rit, findFlag);

    PeakStructure::printWheelCount(peaks, "Counting");
    PeakStructure::printCars("after range");
    PeakStructure::printCarStats("after range");

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
  PeakPool& peaks,
  const unsigned offsetIn)
{
  offset = offsetIn;

  ranges.clear();
  ranges.emplace_back(PeakRange());
  ranges.back().init(cars, peaks.candidates());

  // The first group only needs to be run once.
  PeakStructure::loopOverMethods(peaks, findMethods[0], false);

  while (true)
  {
    bool foundFlag = false;
    for (unsigned mg = 1; mg < NUM_METHOD_GROUPS; mg++)
    {
      bool loopFlag = PeakStructure::loopOverMethods(peaks, 
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


  PeakStructure::fixSpuriousInterPeaks(peaks);

  if (! ranges.empty())
  {
    cout << "WARNFINAL: " << ranges.size() << " ranges left\n";
    for (auto& range: ranges)
    {
      cout << range.strFull(offset);
      cout << range.strProfile();
      overallStats.log("warn");
      warnStats.log(range.strWarn());
    }
    cout << endl;
  }
  else
  {
    modelCountStats.logCatchall(models.active());
  }

cout << peaks.candidates().strQuality(
  "All selected peaks at end of PeakStructure", offset, &Peak::isSelected);

  // Output statistics of partials.
  unsigned numTrainsWithWarnings = 0;
  unsigned numTrainsWithPartials = 0;
  unsigned numCarsWithPartials = 0;
  unsigned numPartialPeaks = 0;
  
  if (! ranges.empty())
    numTrainsWithWarnings = 1;

  for (auto& car: cars)
  {
    const unsigned n = car.numNulls();
    if (n == 0)
      continue;

    numTrainsWithPartials = 1;
    numCarsWithPartials++;
    numPartialPeaks += n;
  }

  partialStats.log("Trains with warnings", numTrainsWithWarnings);
  partialStats.log("Trains with partials", numTrainsWithPartials);
  partialStats.log("Cars with partials", numCarsWithPartials);
  partialStats.log("Partial peak count", numPartialPeaks);
}


bool PeakStructure::hasGaps() const
{
  return (! ranges.empty());
}


void PeakStructure::printWheelCount(
  const PeakPool& peaks,
  const string& text) const
{
  cout << text << " " << 
    peaks.candidatesConst().count(&Peak::isSelected) << " peaks" << endl;
}


void PeakStructure::printCars(const string& text) const
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
  const string& text) const
{
  cout << "Car stats " << text << "\n";
  cout << models.str();
}



/////////////////// stuff to clean up /////////////////////////////////////


void PeakStructure::pushInfo(
  Peak const * pptr,
  const double sampleRate,
  const unsigned carNo,
  unsigned& peakNo,
  unsigned& peakNoInCar,
  PeaksInfo& peaksInfo) const
{
  if (pptr)
  {
    peaksInfo.times.push_back(
      pptr->getIndex() / static_cast<float>(sampleRate));
    peaksInfo.carNumbers.push_back(carNo);
    peaksInfo.peakNumbers.push_back(peakNo);
    peaksInfo.peakNumbersInCar.push_back(peakNoInCar);
  }

  peakNo++;
  peakNoInCar++;
}


void PeakStructure::getPeaksInfo(
  const PeakPool& peaks,
  PeaksInfo& peaksInfo,
  const double sampleRate) const
{
  if (cars.empty() || PeakStructure::hasGaps())
  {
    peaksInfo.numCars = 0;
    peaks.topConst().getTimes(&Peak::isSelected, peaksInfo.times);
    peaksInfo.numPeaks = peaksInfo.times.size();
    peaksInfo.numFrontWheels = 
      (cars.empty() ? 0 : cars.front().numFrontWheels());
  }
  else
  {
    peaksInfo.numCars = cars.size();

    unsigned carNo = 0;
    unsigned peakNo = 0;
    unsigned peakNoInCar = 0;

    for (auto& car: cars)
    {
      const CarPeaksPtr cptr = car.getPeaksPtr();

      PeakStructure::pushInfo(cptr.firstBogieLeftPtr, sampleRate,
        carNo, peakNo, peakNoInCar, peaksInfo);
      PeakStructure::pushInfo(cptr.firstBogieRightPtr, sampleRate,
        carNo, peakNo, peakNoInCar, peaksInfo);
      PeakStructure::pushInfo(cptr.secondBogieLeftPtr, sampleRate,
        carNo, peakNo, peakNoInCar, peaksInfo);
      PeakStructure::pushInfo(cptr.secondBogieRightPtr, sampleRate,
        carNo, peakNo, peakNoInCar, peaksInfo);

      carNo++;
      peakNoInCar = 0;
    }

    peaksInfo.numPeaks = peakNo;
    peaksInfo.numFrontWheels = cars.front().numFrontWheels();
  }
}

