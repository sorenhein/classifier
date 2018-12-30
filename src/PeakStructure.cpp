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
  PeakStructure::setCarRecognizers();

  findCarFunctions.push_back(&PeakStructure::findCarByQuality);
  findCarFunctions.push_back(&PeakStructure::findCarByGeometry);
}


PeakStructure::~PeakStructure()
{
}


void PeakStructure::reset()
{
}

void PeakStructure::setCarRecognizers()
{
  recognizers.clear();

  Recognizer recog;

  // The simple four-wheel car with great peaks.
  recog.params.source = {false, 0};
  recog.params.sumGreat = {true, 4};
  recog.params.starsGood = {false, 0};
  recog.numWheels = 4;
  recog.quality = PEAK_QUALITY_GREAT;
  recog.text = "4 great peaks";
  recognizers.push_back(recog);

  // The simple four-wheel car with 3 great + 1 good peak.
  recog.params.source = {false, 0};
  recog.params.sumGreat = {true, 3};
  recog.params.starsGood = {true, 1};
  recog.numWheels = 4;
  recog.quality = PEAK_QUALITY_GOOD;
  recog.text = "4 (3+1) good peaks";
  recognizers.push_back(recog);

  // The simple four-wheel car with 2 great + 2 good peaks.
  recog.params.source = {false, 0};
  recog.params.sumGreat = {true, 2};
  recog.params.starsGood = {true, 2};
  recog.numWheels = 4;
  recog.quality = PEAK_QUALITY_GOOD;
  recog.text = "4 (2+2) good peaks";
  recognizers.push_back(recog);

  // The front car with 3 great peaks, missing the very first one.
  recog.params.source = {true, static_cast<unsigned>(PEAK_SOURCE_FIRST)};
  recog.params.sumGreat = {true, 3};
  recog.params.starsGood = {true, 0};
  recog.numWheels = 3;
  recog.quality = PEAK_QUALITY_GREAT;
  recog.text = "3 great peaks (front)";
  recognizers.push_back(recog);

  // The front car with 2 great +1 good peaks, missing the very first one.
  recog.params.source = {true, static_cast<unsigned>(PEAK_SOURCE_FIRST)};
  recog.params.sumGreat = {true, 2};
  recog.params.starsGood = {true, 1};
  recog.numWheels = 3;
  recog.quality = PEAK_QUALITY_GOOD;
  recog.text = "3 (2+1) peaks (front)";
  recognizers.push_back(recog);

  // The front car with 2 great peaks, missing the first whole bogey.
  // Don't try to salvage the first 1-2 peaks if it looks too hard.
  recog.params.source = {true, PEAK_SOURCE_FIRST};
  recog.params.sumGreat = {true, 2};
  recog.params.starsGood = {false, 0};
  recog.numWheels = 2;
  recog.quality = PEAK_QUALITY_GREAT;
  recog.text = "2 great peaks (front)";
  recognizers.push_back(recog);
}


bool PeakStructure::matchesModel(
  const CarModels& models,
  const CarDetect& car, 
  unsigned& index,
  float& distance) const
{
  if (! models.findClosest(car, distance, index))
    return false;
  else
    return (distance <= GREAT_CAR_DISTANCE);
}


// TODO Combine these methods more closely
// - Setting up peaks
// - Checking them
// Move some of it to CarDetect?


bool PeakStructure::findLastTwoOfFourWheeler(
  const CarModels& models,
  const PeakRange& range,
  const PeakPtrVector& peakPtrs,
  CarDetect& car) const
{
  car.setLimits(range.start, range.end);

  const unsigned peakNo0 = peakPtrs[0]->getIndex();
  const unsigned peakNo1 = peakPtrs[1]->getIndex();

  if (range.rightGapPresent)
    car.logRightGap(range.end - peakNo1);

  car.logCore(0, 0, peakNo1 - peakNo0);

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


bool PeakStructure::findLastThreeOfFourWheeler(
  const CarModels& models,
  const PeakRange& range,
  const PeakPtrVector& peakPtrs,
  CarDetect& car) const
{
  car.setLimits(range.start, range.end);

  const unsigned peakNo0 = peakPtrs[0]->getIndex();
  const unsigned peakNo1 = peakPtrs[1]->getIndex();
  const unsigned peakNo2 = peakPtrs[2]->getIndex();

  car.logCore(0, peakNo1 - peakNo0, peakNo2 - peakNo1);
  if (range.rightGapPresent)
    car.logRightGap(range.end - peakNo2);

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

  return car.midGapPlausible();
}


bool PeakStructure::findFourWheeler(
  const CarModels& models,
  const PeakRange& range,
  const PeakPtrVector& peakPtrs,
  CarDetect& car) const
{
  car.setLimits(range.start, range.end);

  const unsigned peakNo0 = peakPtrs[0]->getIndex();
  const unsigned peakNo1 = peakPtrs[1]->getIndex();
  const unsigned peakNo2 = peakPtrs[2]->getIndex();
  const unsigned peakNo3 = peakPtrs[3]->getIndex();

  if (range.leftGapPresent)
    car.logLeftGap(peakNo0 - range.start);

  if (range.rightGapPresent)
    car.logRightGap(range.end - peakNo3);

  car.logCore(peakNo1 - peakNo0, peakNo2 - peakNo1, peakNo3 - peakNo2);

  car.logPeakPointers(
    peakPtrs[0], peakPtrs[1], peakPtrs[2], peakPtrs[3]);

  return models.gapsPlausible(car);
}


bool PeakStructure::findNumberedWheeler(
  const CarModels& models,
  const PeakRange& range,
  const PeakPtrVector& peakPtrs,
  const unsigned numWheels,
  CarDetect& car) const
{
  if (numWheels == 2)
    return PeakStructure::findLastTwoOfFourWheeler(models, 
      range, peakPtrs, car);
  else if (numWheels == 3)
    return PeakStructure::findLastThreeOfFourWheeler(models, 
      range, peakPtrs, car);
  else if (numWheels == 4)
    return PeakStructure::findFourWheeler(models, 
      range, peakPtrs, car);
  else
    return false;
}


void PeakStructure::markUpPeaks(
  PeakPtrVector& peakPtrsNew,
  const unsigned numPeaks) const
{
  // TODO Use wheelSpecs
  if (numPeaks == 2)
  {
    // The assumption is that we missed the first two peaks.
    peakPtrsNew[0]->markBogeyAndWheel(BOGEY_RIGHT, WHEEL_LEFT);
    peakPtrsNew[1]->markBogeyAndWheel(BOGEY_RIGHT, WHEEL_RIGHT);
  }
  else if (numPeaks == 3)
  {
    // The assumption is that we missed the very first peak.
    peakPtrsNew[0]->markBogeyAndWheel(BOGEY_LEFT, WHEEL_RIGHT);
    peakPtrsNew[1]->markBogeyAndWheel(BOGEY_RIGHT, WHEEL_LEFT);
    peakPtrsNew[2]->markBogeyAndWheel(BOGEY_RIGHT, WHEEL_RIGHT);
  }
  else if (numPeaks == 4)
  {
    peakPtrsNew[0]->markBogeyAndWheel(BOGEY_LEFT, WHEEL_LEFT);
    peakPtrsNew[1]->markBogeyAndWheel(BOGEY_LEFT, WHEEL_RIGHT);
    peakPtrsNew[2]->markBogeyAndWheel(BOGEY_RIGHT, WHEEL_LEFT);
    peakPtrsNew[3]->markBogeyAndWheel(BOGEY_RIGHT, WHEEL_RIGHT);
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


void PeakStructure::getWheelsByQuality(
  const PeakPtrVector& peakPtrs,
  const PeakQuality quality,
  PeakPtrVector& peakPtrsNew,
  PeakPtrVector& peakPtrsUnused) const
{
  typedef bool (Peak::*QualFncPtr)() const;
  QualFncPtr qptr;

  if (quality == PEAK_QUALITY_GREAT)
    qptr = &Peak::greatQuality;
  else if (quality == PEAK_QUALITY_GOOD)
    qptr = &Peak::goodQuality;
  else
    // TODO Could have more qualities here.
    return;

  for (unsigned i = 0; i < peakPtrs.size(); i++)
  {
    Peak * pp = peakPtrs[i];
    if (pp->isLeftWheel() || 
        pp->isRightWheel() ||
        (pp->* qptr)())
    {
      peakPtrsNew.push_back(pp);
    }
    else
      peakPtrsUnused.push_back(pp);
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
    if (! PeakStructure::matchesModel(models, car, index, distance))
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

  if (PeakStructure::matchesModel(models, car, index, distance))
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


PeakStructure::FindCarType PeakStructure::findCarByQuality(
  const PeakRange& range,
  CarModels& models,
  CarDetect& car,
  PeakPool& peaks) const
{
  vector<Peak *> peakPtrs;
  peaks.getCandPtrs(range.start, range.end, peakPtrs);

  PeakProfile profile;
  profile.make(peakPtrs, range.source);
  if (profile.looksEmpty())
  {
    PeakStructure::markDownPeaks(peakPtrs);
    PeakStructure::printRange(range, "Downgraded " + range.text);
    return FIND_CAR_DOWNGRADE;
  }

  PeakPtrVector peakPtrsNew;
  PeakPtrVector peakPtrsUnused;
  for (auto& recog: recognizers)
  {
    if (! profile.match(recog))
      continue;

    PeakStructure::getWheelsByQuality(peakPtrs, 
      recog.quality, peakPtrsNew, peakPtrsUnused);

    if (peakPtrsNew.size() != recog.numWheels)
      THROW(ERR_ALGO_PEAK_STRUCTURE, "Not " + recog.text);

    if (PeakStructure::findNumberedWheeler(models, range,
        peakPtrsNew, recog.numWheels, car))
    {
      PeakStructure::markUpPeaks(peakPtrsNew, recog.numWheels);
      PeakStructure::markDownPeaks(peakPtrsUnused);
      return FIND_CAR_MATCH;
    }
    else
    {
      cout << "Failed to find car among " << recog.text << "\n";
      PeakStructure::printRange(range, "Didn't do " + range.text);
      cout << profile.str();
      return FIND_CAR_NO_MATCH;
    }
  }

  PeakStructure::printRange(range, "Didn't match " + range.text);
  cout << profile.str();
  return FIND_CAR_NO_MATCH;
}


bool PeakStructure::getClosest(
  const list<unsigned>& carPoints,
  const PeakPool& peaks,
  const PeakFncPtr& fptr,
  PPciterator& pit,
  PeakPtrVector& closestPeaks,
  PeakPtrVector& skippedPeaks) const
{
  // Do the three remaining wheels.
  PPciterator pend = peaks.candcend();
  PPciterator pit0 = peaks.nextCandExcl(pit, fptr);
  if (pit0 == pend)
    return false;

  skippedPeaks.clear();
  closestPeaks.clear();
  closestPeaks.push_back(* pit);
  const unsigned pstart = (* pit)->getIndex() + offset;
  
  auto cit = carPoints.begin();
  cit++;
  // pit is assumed to line up with carPoints[1], the first wheel.
  const unsigned cstart = * cit;
  cit++;
  // Start from the second wheel.

  PPciterator pit1;
  for (unsigned i = 0; i < 3; i++)
  {
    const unsigned ptarget = (* cit) + pstart - cstart;
    unsigned pval0 = (* pit0)->getIndex() + offset;

    while (true)
    {
      pit1 = peaks.nextCandExcl(pit0, fptr);
      if (pit1 == pend)
      {
        if (closestPeaks.size() != 3)
          return false;

        closestPeaks.push_back(* pit0);
        return true;
      }

      const unsigned pval1 = (* pit1)->getIndex() + offset;
      const unsigned mid = (pval0 + pval1) / 2;
      if (ptarget <= mid)
      {
        closestPeaks.push_back(* pit0);
        pit0 = pit1;
        break;
      }
      else if (ptarget <= pval1)
      {
        skippedPeaks.push_back(* pit0);
        closestPeaks.push_back(* pit1);
        pit1 = peaks.nextCandExcl(pit1, fptr);
        if (pit1 == pend && closestPeaks.size() != 4)
          return false;

        pit0 = pit1;
        break;
      }
      else
      {
        skippedPeaks.push_back(* pit0);
        pit0 = pit1;
      }
    }
  }
  return true;
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
  const PeakRange& range,
  CarModels& models,
  CarDetect& car,
  PeakPool& peaks) const
{
  // TODO Could be list later on
  vector<Peak *> peakPtrs;

  vector<PPciterator> peakIters;
  peaks.getCandIters(range.start, range.end, peakIters);

  CarDetect carModel;
  list<unsigned> carPoints;
  PeakPtrVector closestPeaks, skippedPeaks;
  unsigned matchIndex;
  float distance;

  PeakRange rangeLocal;
  rangeLocal.leftGapPresent = false;
  rangeLocal.rightGapPresent = false;

  for (unsigned mno = 0; mno < models.size(); mno++)
  {
    models.getCar(carModel, mno);

    // 6 elements: Left edge, 4 wheels, right edge.
    carModel.getCarPoints(carPoints);

    for (PPciterator pit: peakIters)
    {
      if (! (* pit)->goodQuality())
        continue;

      // pit corresponds to the first wheel.
      if (! PeakStructure::getClosest(carPoints, peaks, &Peak::goodQuality,
          pit, closestPeaks, skippedPeaks))
        continue;

      // We deal with the edges later.
      rangeLocal.start = closestPeaks.front()->getIndex() - 1;
      rangeLocal.end = closestPeaks.back()->getIndex() + 1;

      if (! PeakStructure::findFourWheeler(models, rangeLocal,
          closestPeaks, car))
        continue;
      
      if (! PeakStructure::matchesModel(models, car, matchIndex, distance))
        continue;
      
      if (matchIndex != mno)
        continue;

      if (! PeakStructure::isConsistent(closestPeaks))
      {
        cout << "WARNING: Peaks inconsistent with car #" <<
          matchIndex << " found (distance " << distance << ")\n";
        cout << range.str(offset) << endl;
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
  const PeakPool& peaks,
  list<PeakRange>& ranges) const
{
  PeakRange * range;
  ranges.clear();

  // There may be a car in front.
  if (cars.front().startValue() > peaks.firstCandIndex())
  {
    ranges.emplace_back(PeakRange());
    range = &ranges.back();

    range->carAfter = cars.begin();
    range->start = peaks.firstCandIndex();
    range->end = cars.front().startValue();
    range->leftGapPresent = false;
    range->rightGapPresent = cars.front().hasLeftGap();
    range->stuckFlag = false;
    range->text = "first whole-car";
  }

  // There may be inner cars missing.
  for (auto cit = cars.begin(); cit != prev(cars.end()); cit++)
  {
    auto ncit = next(cit);
    if (cit->endValue() >= ncit->startValue())
      continue;

    ranges.emplace_back(PeakRange());
    range = &ranges.back();

    range->carAfter = ncit;
    range->start = cit->endValue();
    range->end = ncit->startValue();
    range->leftGapPresent = cit->hasRightGap();
    range->rightGapPresent = ncit->hasLeftGap();
    range->stuckFlag = false;
    range->text = "intra-gap";
  }

  // There may be a car in back.
  if (cars.back().endValue() >= peaks.lastCandIndex())
    return;

  ranges.emplace_back(PeakRange());
  range = &ranges.back();
  range->carAfter = cars.end();
  range->start = cars.back().endValue();
  range->end = peaks.lastCandIndex();
  range->leftGapPresent = cars.back().hasRightGap();
  range->rightGapPresent = false;
  range->stuckFlag = false;
  range->text = "last whole-car";
}


bool PeakStructure::fillPartialSides(
  CarModels& models,
  CarDetect& car1,
  CarDetect& car2) const
{
  // Some cars from the initial findWholeCars() run may have been
  // partial (i.e., missing a side gap) on either or both sides.
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


void PeakStructure::seekGaps(
  PPciterator pitLeft,
  PPciterator pitRight,
  const PeakPool& peaks,
  PeakRange& range) const
{
  // Check for gap in front of pitLeft.
  range.leftGapPresent = false;
  range.start = (* pitLeft)->getIndex() - 1;
  if (pitLeft != peaks.candcbegin())
  {
    Peak const * prevPtr = * prev(pitLeft);
    if (prevPtr->isBogey())
    {
      // This rounds to make the cars abut.
      const unsigned d = (* pitLeft)->getIndex() - prevPtr->getIndex();
      range.start = (* pitLeft)->getIndex() + (d/2) - d;
      range.leftGapPresent = true;
    }
  }

  // Check for gap after pitRight.
  range.rightGapPresent = false;
  range.end = (* pitRight)->getIndex() + 1;
  auto postIter = next(pitRight);
  if (postIter != peaks.candcend())
  {
    Peak const * nextPtr = * postIter;
    if (nextPtr->isBogey())
    {
      range.end = (* pitRight)->getIndex() + 
        (nextPtr->getIndex() - (* pitRight)->getIndex()) / 2;
      range.rightGapPresent = true;
    }
  }
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


void PeakStructure::findWholeCars(
  CarModels& models,
  list<CarDetect>& cars,
  PeakPool& peaks) const
{
  // Set up a sliding vector of running peaks.
  vector<PPiterator> runIter;
  PeakPtrVector runPtr;
  PeakRange range;

  PPiterator candbegin = peaks.candbegin();
  PPiterator candend = peaks.candend();

  PPiterator cit = peaks.nextCandIncl(candbegin, &Peak::isSelected);
  for (unsigned i = 0; i < 4; i++)
  {
    runIter.push_back(cit);
    runPtr.push_back(* cit);
  }

  while (cit != candend)
  {
    if (PeakStructure::isWholeCar(runPtr))
    {
      PeakStructure::seekGaps(runIter[0], runIter[3], peaks, range);

      CarDetect car;
      PeakStructure::findFourWheeler(models, range, runPtr, car);

      PeakStructure::updateModels(models, car);
      cars.push_back(car);
      PeakStructure::markUpPeaks(runPtr, 4);
    }

    for (unsigned i = 0; i < 3; i++)
    {
      runIter[i] = runIter[i+1];
      runPtr[i] = runPtr[i+1];
    }

    cit = peaks.nextCandExcl(cit, &Peak::isSelected);
    runIter[3] = cit;
    runPtr[3] = * cit;
  }
}


list<PeakStructure::PeakRange>::iterator PeakStructure::updateRanges(
  CarModels& models,
  const list<CarDetect>& cars,
  const list<CarDetect>::iterator& carIt,
  list<PeakRange>::iterator& rit,
  list<PeakRange>& ranges,
  const FindCarType& findFlag) const
{
  if (findFlag == FIND_CAR_DOWNGRADE)
    return ranges.erase(rit);

  CarDetect& car = * carIt;
  if (carIt != cars.begin())
    PeakStructure::fillPartialSides(models, * prev(carIt), car);

  auto nextCarIt = next(carIt);
  if (nextCarIt != cars.end())
    PeakStructure::fillPartialSides(models, car, * nextCarIt);

  PeakRange& range = * rit;

  if (car.hasLeftGap())
  {
    if (car.hasRightGap())
      return ranges.erase(rit);
    else if (car.endValue() == range.end)
      // Last car
      return ranges.erase(rit);
    else
    {
      // Shorten the range on the left to make room for the new
      // car preceding it.
      // This does not change any carAfter values.
      range.start = car.endValue();
      range.leftGapPresent = car.hasRightGap();
      return ++rit;
    }
  }
  else if (car.hasRightGap())
  {
    if (car.startValue() == range.start)
      return ranges.erase(rit);
    else
    {
      // Shorten the range on the right to make room for the new
      // car following it.
      range.carAfter = carIt;
      range.end = car.startValue();
      range.rightGapPresent = car.hasLeftGap();
      return ++rit;
    }
  }
  else
  {
    // Recognized a car in the middle of the range.
    // The new order is rangeNew - car - range.
    PeakRange rangeNew = range;
    range.start = car.endValue();
    range.leftGapPresent = car.hasRightGap();
    rangeNew.carAfter = carIt;
    rangeNew.end = car.startValue();
    rangeNew.rightGapPresent = car.hasLeftGap();
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

  PeakStructure::findWholeCars(models, cars, peaks);
  PeakStructure::printCars(cars, "after whole cars");

  if (cars.size() == 0)
    THROW(ERR_NO_PEAKS, "No cars?");

  PeakStructure::updateCarDistances(models, cars);
  PeakStructure::printCars(cars, "before intra gaps");

  // Check open ranges.
  list<PeakRange> ranges;
  PeakStructure::makeRanges(cars, peaks, ranges);

  CarDetect car;
  bool changeFlag = true;
  while (changeFlag && ! ranges.empty())
  {
    changeFlag = false;
    auto rit = ranges.begin();
    while (rit != ranges.end())
    {
      PeakRange& range = * rit;
      if (range.stuckFlag)
      {
        rit++;
        continue;
      }

      // TODO This becomes a loop over detectors.
      // Can also include downgrade, separately, then findFlag
      // is not needed in the recognizer itself (but still needed).
      cout << "Range: " << range.str(offset);

      FindCarType findFlag = FIND_CAR_SIZE;
      for (auto fptr: findCarFunctions)
      {
        findFlag = (this->* fptr)(range, models, car, peaks);
        if (findFlag != FIND_CAR_NO_MATCH)
          break;
      }

      if (findFlag == FIND_CAR_NO_MATCH)
      {
        range.stuckFlag = true;
        rit++;
        continue;
      }

      changeFlag = true;

      list<CarDetect>::iterator newcit;
      if (findFlag == FIND_CAR_MATCH)
      {
        PeakStructure::updateModels(models, car);
        newcit = cars.insert(range.carAfter, car);
        PeakStructure::updateCarDistances(models, cars);
      }

      rit = PeakStructure::updateRanges(models, cars, newcit, 
        rit, ranges, findFlag);

      PeakStructure::printWheelCount(peaks, "Counting");
      PeakStructure::printCars(cars, "after range");
      PeakStructure::printCarStats(models, "after range");
    }
  }

  if (! ranges.empty())
  {
    cout << "WARNFINAL: " << ranges.size() << " ranges left\n";
    for (auto& range: ranges)
      cout << range.str(offset);
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


void PeakStructure::printRange(
  const PeakRange& range,
  const string& text) const
{
  cout << text << " " << 
    range.start + offset << "-" << 
    range.end + offset << endl;
}


void PeakStructure::printWheelCount(
  const PeakPool& peaks,
  const string& text) const
{
  // TODO Is this the same as PeakPool::countCandidates(&Peak::isSelected)?
  unsigned count = 0;
  PPciterator candcbegin = peaks.candcbegin();
  PPciterator candcend = peaks.candcend();

  for (PPciterator pit = candcbegin; pit != candcend; pit++)
  {
    if ((* pit)->isSelected())
      count++;
  }
  cout << text << " " << count << " peaks" << endl;
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

