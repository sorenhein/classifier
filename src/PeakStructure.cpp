#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakStructure.h"
#include "PeakRepair.h"
#include "PeakProfile.h"
#include "CarModels.h"
#include "Except.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

#define GOOD_CAR_DISTANCE 1.5f
#define GREAT_CAR_DISTANCE 0.5f

#define NUM_METHODS 10

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
  findCarFunctions.push_back(
    { &PeakStructure::findCarByLeveledPeaks, 
      "by leveled peaks", 6});

  findCarFallbacks.push_back(
    { &PeakStructure::findPartialFirstCarByQuality, 
      "first partial by quality", 1});
  findCarFallbacks.push_back(
    { &PeakStructure::findPartialLastCarByQuality, 
      "last partial by quality", 9});
  findCarFallbacks.push_back(
    { &PeakStructure::findCarByEmptyLast, 
      "by empty last", 7});
  findCarFallbacks.push_back(
    { &PeakStructure::findCarByThreePeaks, 
      "by three peaks", 8});
  
  hitSize = NUM_METHODS;
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
PeakPtrVector ppv;
peakPtrs.flattenTODO(ppv);
    car.makeLastTwoOfFourWheeler(range, ppv);
    return models.twoWheelerPlausible(car);
  }
  else if (numWheels == 3)
  {
PeakPtrVector ppv;
peakPtrs.flattenTODO(ppv);
    car.makeLastThreeOfFourWheeler(range, ppv);
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


void PeakStructure::markUpPeaks(
  PeakPtrVector& peakPtrsNew,
  const unsigned numPeaks) const
{
  if (numPeaks > 4)
    return;

  // Always take the last numPeaks wheels.
  for (unsigned i = 0; i < numPeaks; i++)
  {
    if (! peakPtrsNew[i])
      continue;

    peakPtrsNew[i]->markBogeyAndWheel(
      wheelSpecs[i+4-numPeaks].bogey, 
      wheelSpecs[i+4-numPeaks].wheel);
  }
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
  PeakPtrs runPtr;
  runPtr.assign(4, ** range.begin());

  PeakRange rangeLocal;

  for (auto pit: range)
  {
    if (! (* pit)->isSelected())
      continue;

    runPtr.shift_down(* pit);

    if (runPtr.isFourWheeler())
    {
      // We deal with the edges later.
      // PeakPtrVector rp;
      // runPtr.flattenTODO(rp);
      rangeLocal.init(runPtr);
      car.makeFourWheeler(rangeLocal, runPtr);

      runPtr.markup();

      return FIND_CAR_MATCH;
    }
  }

  return FIND_CAR_NO_MATCH;
}


PeakStructure::FindCarType PeakStructure::findPartialCarByQuality(
  const CarModels& models,
  const PeakFncPtr& fptr,
  const bool firstFlag,
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{

  PeakPtrs ppu, ppunu;
  range.split(fptr, ppu, ppunu);

  PeakRepair repair;
  if (repair.edgeCar(models, offset, firstFlag, 
      peaks, range, ppu, ppunu))
  {
if (firstFlag)
  cout << "Hit an anywheeler\n";
else
  cout << "Hit a lastwheeler\n";

  PeakPtrVector peakPtrsUsed, peakPtrsUnused;
  ppu.flattenTODO(peakPtrsUsed);
  ppunu.flattenTODO(peakPtrsUnused);

    car.makeAnyWheeler(range, peakPtrsUsed);
    // PeakStructure::markUpPeaks(peakPtrsUsed, 4);
    // PeakStructure::markDownPeaks(peakPtrsUnused);
      ppu.markup();
      ppunu.apply(&Peak::markdown);
    return FIND_CAR_PARTIAL;
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
      &Peak::goodQuality, true, peaks, range, car);
}


PeakStructure::FindCarType PeakStructure::findPartialLastCarByQuality(
  const CarModels& models,
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  if (! range.isLastCar())
    return FIND_CAR_NO_MATCH;

  if (range.numGreat() > 4)
    return FIND_CAR_NO_MATCH;

  // TODO Should the previous car then actively become the last one?
  if (range.numGood() == 0)
    return FIND_CAR_DOWNGRADE;

cout << "LASTLAST\n";
  return PeakStructure::findPartialCarByQuality(models, 
      &Peak::goodQuality, false, peaks, range, car);
}


PeakStructure::FindCarType PeakStructure::findCarByPeaks(
  const CarModels& models,
  const PeakRange& range,
  PeakPtrs& peakPtrs,
  CarDetect& car) const
{
  PeakPtrVector pv;
  peakPtrs.flattenTODO(pv);

  if (PeakStructure::findNumberedWheeler(models, range, peakPtrs, 4, car))
  {
    // We deal with the edges later.
    PeakRange rangeLocal;
    rangeLocal.init(peakPtrs);
    car.makeFourWheeler(rangeLocal, peakPtrs);

    PeakStructure::markUpPeaks(pv, 4);

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


PeakStructure::FindCarType PeakStructure::findCarByGeometry(
  const CarModels& models,
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  CarDetect carModel;
  list<unsigned> carPoints;
  PeakPtrs peakPtrsUsed, peakPtrsUnused;
  PeakPtrVector closestPeaks, skippedPeaks;
  MatchData match;
  PeakRange rangeLocal;

  for (unsigned mno = 0; mno < models.size(); mno++)
  {
    if (models.empty(mno))
      continue;

    models.getCar(carModel, mno);

    // 6 elements: Left edge, 4 wheels, right edge.
    carModel.getCarPoints(carPoints);

    for (auto pit: range)
    {
      if (! (* pit)->goodQuality())
        continue;

      // pit corresponds to the first wheel.
      if (! peaks.getClosest(carPoints, &Peak::goodQuality, pit, 4,
          peakPtrsUsed, peakPtrsUnused))
        continue;

      // We deal with the edges later.
peakPtrsUsed.flattenTODO(closestPeaks);
peakPtrsUnused.flattenTODO(skippedPeaks);

      rangeLocal.init(peakPtrsUsed);
      // car.makeFourWheeler(rangeLocal, closestPeaks);
      car.makeFourWheeler(rangeLocal, peakPtrsUsed);
      if (! models.gapsPlausible(car))
        continue;
      
      if (! models.matchesDistance(car, GOOD_CAR_DISTANCE, true, 
          mno, match))
        continue;
      
      if (! peakPtrsUsed.isFourWheeler(true))
      {
        cout << "WARNING: Peaks inconsistent with car #" <<
          match.index << " found (distance " << match.distance << ")\n";
        cout << range.strFull(offset) << endl;
      }

      peakPtrsUsed.markup();
      peakPtrsUnused.apply(&Peak::markdown);

      return FIND_CAR_MATCH;
    }
  }
  return FIND_CAR_NO_MATCH;
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

  if (range.looksEmpty())
  {
    range.getPeakPtrs().apply(&Peak::markdown);
    return FIND_CAR_DOWNGRADE;
  }
  else
    return FIND_CAR_NO_MATCH;
}


PeakStructure::FindCarType PeakStructure::findCarByLeveledPeaks(
  const CarModels& models,
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  // Here we use peak quality, so if we have 5 peaks with a spurious
  // one in the middle (and if it's a new car geometry, so
  // findCarByGeometry didn't find it), then the spurious peak will
  // probably fail on peak quality as opposed to shape quality.
  UNUSED(peaks);

  PeakPtrs peakPtrsUsed, peakPtrsUnused;
  range.split(&Peak::goodPeakQuality, peakPtrsUsed, peakPtrsUnused);

  if (peakPtrsUsed.size() == 4 &&
      PeakStructure::findCarByPeaks(models, range, peakPtrsUsed, car) ==
        FIND_CAR_MATCH)
  {
    peakPtrsUnused.apply(&Peak::markdown);
    return FIND_CAR_MATCH;
  }
  else
    return FIND_CAR_NO_MATCH;
}


PeakStructure::FindCarType PeakStructure::findCarByEmptyLast(
  const CarModels& models,
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  UNUSED(models);
  UNUSED(peaks);
  UNUSED(car);

  if (range.looksEmptyLast())
  {
    range.getPeakPtrs().apply(&Peak::markdown);
    return FIND_CAR_DOWNGRADE;
  }
  else
    return FIND_CAR_NO_MATCH;
}


PeakStructure::FindCarType PeakStructure::findCarByThreePeaks(
  const CarModels& models,
  PeakPool& peaks,
  PeakRange& range,
  CarDetect& car) const
{
  if (range.numGood() != 3)
    // TODO For now.
    // Could also take first or last 3 of a large range.
    // s04, 065516 has two three-wheel cars next to each other.
    return FIND_CAR_NO_MATCH;
  else if (range.isFirstCar())
    // TODO For now, as these are unusually error-prone.
    return FIND_CAR_NO_MATCH;

  PeakPtrs peakPtrsUsed, peakPtrsUnused;
  range.split(&Peak::goodQuality, peakPtrsUsed, peakPtrsUnused);

  MatchData match;
  Peak peakHint;
  if (! models.matchesPartial(peakPtrsUsed, GREAT_CAR_DISTANCE,
      match, peakHint))
    return FIND_CAR_NO_MATCH;

  cout << "\nHint for repair:\n";
  cout << peakHint.strHeaderQuality();
  cout << peakHint.strQuality(offset) << "\n";

  if (! peaks.repair(peakHint, &Peak::goodQuality, offset))
    return FIND_CAR_NO_MATCH;

  range.fill(peaks);

  return findCarByGeometry(models, peaks, range, car);
}


CarListIter PeakStructure::updateRecords(
  const PeakRange& range,
  const CarDetect& car,
  CarModels& models,
  list<CarDetect>& cars)
{
  CarListIter newcit = cars.insert(range.carAfterIter(), car);

  if (newcit != cars.begin())
  {
    auto prevcit = prev(newcit);
    const bool hasRight = prevcit->hasRightGap();
    PeakStructure::fillPartialSides(* prev(newcit), * newcit);
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
  list<FncGroup>& findCarMethods)
{
  CarDetect car;

  bool changeFlag = true;
  bool anyChangeFlag = false;
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
      for (auto& fgroup: findCarMethods)
      {
        findFlag = (this->* fgroup.fptr)(models, peaks, range, car);
        if (findFlag != FIND_CAR_NO_MATCH)
        {
          cout << "Hit " << fgroup.name << "\n";
          cout << range.strFull(offset);
          hits[fgroup.number]++;
          changeFlag = true;
          anyChangeFlag = true;
          break;
        }
      }

      if (findFlag == FIND_CAR_NO_MATCH)
      {
        rit++;
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
    }
  }
  return anyChangeFlag;
}


void PeakStructure::fixSpuriousInterPeaks(
  const list<CarDetect>& cars,
  PeakPool& peaks) const
{
  // It can happen that there are spurious peaks between the starting
  // and/or ending bogey of a car and that car's start or end.
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
      (* pit)->markNoBogey();
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

  while (true)
  {
    if (! PeakStructure::loopOverMethods(models, cars, peaks, 
          findCarFunctions) &&
        ! PeakStructure::loopOverMethods(models, cars, peaks, 
          findCarFallbacks))
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
  "All selected peaks at end of PeakStructure", offset);


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

