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
    { &PeakStructure::findCarByOrder, "by 1234 order"});
  findCarFunctions.push_back(
    { &PeakStructure::findPartialFirstCarByQuality, "partial by quality"});
  findCarFunctions.push_back(
    { &PeakStructure::findCarByGreatQuality, "by great quality"});
  findCarFunctions.push_back(
    { &PeakStructure::findCarByGoodQuality, "by good quality"});
  findCarFunctions.push_back(
     { &PeakStructure::findCarByGeometry, "by geometry"});
  findCarFunctions.push_back(
    { &PeakStructure::findEmptyRange, "by emptiness"});
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
  // TODO Move and consolidate checks, maybe to CarModels.
  if (numWheels == 2)
  {
    car.makeLastTwoOfFourWheeler(range, peakPtrs);

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
  else if (numWheels == 3)
  {
    car.makeLastThreeOfFourWheeler(range, peakPtrs);

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

    return car.midGapPlausible();
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


PeakStructure::FindCarType PeakStructure::findPartialCarByQualityNew(
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
    return PeakStructure::findPartialCarByQualityNew(models,
        &Peak::greatQuality, 3, range, car);
  }
  else if (range.numGood() == 3)
  {
    return PeakStructure::findPartialCarByQualityNew(models,
        &Peak::goodQuality, 3, range, car);
  }
  else if (range.numGreat() == 2)
  {
    return PeakStructure::findPartialCarByQualityNew(models,
        &Peak::greatQuality, 2, range, car);
  }
  else
    return FIND_CAR_NO_MATCH;
}


PeakStructure::FindCarType PeakStructure::findCarByQualityNew(
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
    return PeakStructure::findCarByQualityNew(models,
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
    return PeakStructure::findCarByQualityNew(models,
      &Peak::goodQuality, range, car);
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

      if (! skippedPeaks.empty())
      {
        cout << "WARNING (inspect?): Some good peaks were skipped.\n";
        cout << skippedPeaks.front()->strHeaderQuality();
        for (Peak * skip: skippedPeaks)
          cout << skip->strQuality(offset);
        cout << "\n";
      }

      return FIND_CAR_MATCH;
    }
  }
  return FIND_CAR_NO_MATCH;
}


// TODO: 3 geometric peaks, look in peaks for the missed 4th one
// Only if loop is stuck?


void PeakStructure::makeRanges(
  const list<CarDetect>& cars,
  const PeakPool& peaks)
{
  ranges2.clear();
  PeakRange range2;
  range2.init(cars, peaks);
  ranges2.push_back(range2);
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
    if (! (* pit)->goodQuality())
      continue;

    if (PeakStructure::isWholeCar(runPtr))
    {
      // We deal with the edges later.
      // rangeLocal.start = runPtr[0]->getIndex();
      // rangeLocal.end = runPtr[3]->getIndex();
      // rangeLocal.leftGapPresent = false;
      // rangeLocal.rightGapPresent = false;
      rangeLocal.init(runPtr);
      car.makeFourWheeler(rangeLocal, runPtr);

      // TODO Maybe something we should do in all recognizers?
      PeakStructure::markUpPeaks(runPtr, 4);

      return FIND_CAR_MATCH;
    }

    for (unsigned i = 0; i < 3; i++)
      runPtr[i] = runPtr[i+1];

    runPtr[3] = * pit;
  }

  return FIND_CAR_NO_MATCH;
}


list<PeakRange>::iterator PeakStructure::updateRanges2(
  CarModels& models,
  const list<CarDetect>& cars,
  const list<CarDetect>::iterator& carIt,
  list<PeakRange>::iterator& rit,
  const FindCarType& findFlag)
{
  PeakRange& range = * rit;

  if (findFlag == FIND_CAR_DOWNGRADE)
    return ranges2.erase(rit);

  CarDetect& car = * carIt;
  if (carIt != cars.begin())
    PeakStructure::fillPartialSides(models, * prev(carIt), car);

  auto nextCarIt = next(carIt);
  if (nextCarIt != cars.end())
    PeakStructure::fillPartialSides(models, car, * nextCarIt);

  if (car.hasLeftGap() || car.startValue() == range.startValue())
  {
    if (car.hasRightGap() || car.endValue() == range.endValue())
      return ranges2.erase(rit);
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
    return ranges2.insert(rit, rangeNew);
  }
}


void PeakStructure::markCars(
  CarModels& models,
  list<CarDetect>& cars,
  PeakPool& peaks,
  const unsigned offsetIn)
{
  offset = offsetIn;

  PeakStructure::makeRanges(cars, peaks);

  CarDetect car;
  // TODO Could be vectors later on
  PeakIterVector peakIters;
  PeakPtrVector peakPtrs;
  PeakProfile profile;

  bool changeFlag = true;
  while (changeFlag && ! ranges2.empty())
  {
    changeFlag = false;
    auto rit2 = ranges2.begin();
    while (rit2 != ranges2.end())
    {
      PeakRange& range2 = * rit2;

      // TODO This becomes a loop over detectors.
      // Can also include downgrade, separately, then findFlag
      // is not needed in the recognizer itself (but still needed).
      cout << "Range: " << range2.strFull(offset);

      // Set up some useful stuff for all recognizers.
      range2.fill(peaks);

      FindCarType findFlag = FIND_CAR_SIZE;
      for (auto& fgroup: findCarFunctions)
      {
        findFlag = (this->* fgroup.fptr)(models, peaks, range2, car);
        if (findFlag != FIND_CAR_NO_MATCH)
        {
          cout << "NEWHIT " << fgroup.name << "\n";
          cout << range2.strFull(offset);
          break;
        }
      }

      if (findFlag == FIND_CAR_NO_MATCH)
      {
        rit2++;
        continue;
      }

      changeFlag = true;

      list<CarDetect>::iterator newcit;
      if (findFlag == FIND_CAR_MATCH)
      {
        PeakStructure::updateModels(models, car);
        newcit = cars.insert(range2.carAfterIter(), car);
        PeakStructure::updateCarDistances(models, cars);
      }

      rit2 = PeakStructure::updateRanges2(models, cars, newcit, 
        rit2, findFlag);

      PeakStructure::printWheelCount(peaks, "Counting");
      PeakStructure::printCars(cars, "after range");
      PeakStructure::printCarStats(models, "after range");
    }
  }

  if (! ranges2.empty())
  {
    cout << "WARNFINAL: " << ranges2.size() << " ranges left\n";
    for (auto& range: ranges2)
      cout << range.strFull(offset);
    cout << endl;
  }
}


bool PeakStructure::updateImperfections(
  const unsigned num,
  const bool firstCarFlag,
  Imperfections& imperf) const
{
  if (firstCarFlag)
  {
    // Special case: There are selected peaks preceding the first
    // recognized car.  Probably we missed the entire car.
    if (num <= 4)
    {
      // It doesn't have to be skips, but we can't sort out here
      // whether we missed the first or in-between peaks.
      imperf.numSkipsOfReal = 0;
      imperf.numSkipsOfSeen = 4 - num;
    }
    else
      return false;
  }
  else
  {
    if (num <= 4)
    {
      // Probably we missed the car.
      imperf.numMissingLater += 4 - num;
    }
    else if (num == 5)
    {
      // Probably a single, spurious peak.
      imperf.numSpuriousLater++;
    }
    else
      return false;
  }
  return true;
}


bool PeakStructure::markImperfections(
  const list<CarDetect>& cars,
  const PeakPool& peaks,
  Imperfections& imperf) const
{
  // Based on the cars we recognized, make an educated guess of the
  // number of peaks of different types that we may not have got
  // exactly right.
  
  // For now we assume cars of exactly 4 peaks each.

  imperf.reset();
  auto car = cars.begin();
  PPciterator cand = peaks.candcbegin();
  PPciterator candcend = peaks.candcend();

  while (cand != candcend && car != cars.end())
  {
    if (! (*cand)->isSelected())
    {
      cand++;
      continue;
    }

    // First look for unmatched, leading detected peaks.
    unsigned numPreceding = 0;
    while (cand != candcend && car->peakPrecedesCar(** cand))
    {
      if ((*cand)->isSelected())
        numPreceding++;
      cand++;
    }

    if (numPreceding)
    {
      if (! PeakStructure::updateImperfections(numPreceding,
          car == cars.begin(), imperf))
        return false;
    }

    if (cand == candcend)
      break;

    // Then look for matched, detected peaks.
    unsigned numMatches = 0;
    while (cand != candcend && ! car->carPrecedesPeak(** cand))
    {
      if ((*cand)->isSelected())
        numMatches++;
      cand++;
    }

    if (numMatches)
    {
      if (! PeakStructure::updateImperfections(numMatches,
          car == cars.begin(), imperf))
        return false;
    }

    if (cand == candcend)
      break;

    // Finally, advance the car.
    while (car != cars.end() && car->carPrecedesPeak(** cand))
      car++;
  }

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

