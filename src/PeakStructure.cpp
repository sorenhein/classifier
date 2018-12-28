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


PeakStructure::PeakStructure()
{
  PeakStructure::reset();
  PeakStructure::setCarRecognizers();
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
    return (distance <= 1.5f);
}


bool PeakStructure::findLastTwoOfFourWheeler(
  const CarModels& models,
  const PeakCondition& condition,
  const PeakPtrVector& peakPtrs,
  CarDetect& car) const
{
  car.setLimits(condition.start, condition.end);

  const unsigned peakNo0 = peakPtrs[0]->getIndex();
  const unsigned peakNo1 = peakPtrs[1]->getIndex();

  if (condition.rightGapPresent)
    car.logRightGap(condition.end - peakNo1);

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
  const PeakCondition& condition,
  const PeakPtrVector& peakPtrs,
  CarDetect& car) const
{
  car.setLimits(condition.start, condition.end);

  const unsigned peakNo0 = peakPtrs[0]->getIndex();
  const unsigned peakNo1 = peakPtrs[1]->getIndex();
  const unsigned peakNo2 = peakPtrs[2]->getIndex();

  car.logCore(0, peakNo1 - peakNo0, peakNo2 - peakNo1);
  if (condition.rightGapPresent)
    car.logRightGap(condition.end - peakNo2);

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
  const PeakCondition& condition,
  const PeakPtrVector& peakPtrs,
  CarDetect& car) const
{
  car.setLimits(condition.start, condition.end);

  const unsigned peakNo0 = peakPtrs[0]->getIndex();
  const unsigned peakNo1 = peakPtrs[1]->getIndex();
  const unsigned peakNo2 = peakPtrs[2]->getIndex();
  const unsigned peakNo3 = peakPtrs[3]->getIndex();

  if (condition.leftGapPresent)
    car.logLeftGap(peakNo0 - condition.start);

  if (condition.rightGapPresent)
    car.logRightGap(condition.end - peakNo3);

  car.logCore(peakNo1 - peakNo0, peakNo2 - peakNo1, peakNo3 - peakNo2);

  car.logPeakPointers(
    peakPtrs[0], peakPtrs[1], peakPtrs[2], peakPtrs[3]);

  return models.gapsPlausible(car);
}


bool PeakStructure::findNumberedWheeler(
  const CarModels& models,
  const PeakCondition& condition,
  const PeakPtrVector& peakPtrs,
  const unsigned numWheels,
  CarDetect& car) const
{
  if (numWheels == 2)
    return PeakStructure::findLastTwoOfFourWheeler(models, 
      condition, peakPtrs, car);
  else if (numWheels == 3)
    return PeakStructure::findLastThreeOfFourWheeler(models, 
      condition, peakPtrs, car);
  else if (numWheels == 4)
    return PeakStructure::findFourWheeler(models, 
      condition, peakPtrs, car);
  else
    return false;
}


void PeakStructure::markUpPeaks(
  PeakPtrVector& peakPtrsNew,
  const unsigned numPeaks) const
{
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


void PeakStructure::markDownPeaks(PeakPtrVector& peakPtrsUnused) const
{
  for (auto& pp: peakPtrsUnused)
    pp->unselect();
}


void PeakStructure::updatePeaks(
  PeakPtrVector& peakPtrsNew,
  PeakPtrVector& peakPtrsUnused,
  const unsigned numPeaks) const
{
  PeakStructure::markUpPeaks(peakPtrsNew, numPeaks);
  PeakStructure::markDownPeaks(peakPtrsUnused);
}


void PeakStructure::downgradeAllPeaks(PeakPtrVector& peakPtrs) const
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


void PeakStructure::splitPeaks(
  const PeakPtrVector& peakPtrs,
  const PeakCondition& condition,
  PeakCondition& condition1,
  PeakCondition& condition2) const
{
  vector<unsigned> indices;
  for (unsigned i = 0; i < peakPtrs.size(); i++)
  {
    Peak * pp = peakPtrs[i];
    if (pp->isLeftWheel() || 
        pp->isRightWheel() ||
        pp->goodQuality())
    {
      indices.push_back(i);
    }
  }

  if (indices.size() != 8)
    THROW(ERR_ALGO_PEAK_STRUCTURE, "Expected 8 peaks in two cars");

  const unsigned middle = 
    (peakPtrs[indices[3]]->getIndex() + 
     peakPtrs[indices[4]]->getIndex()) / 2;

  condition1.source = PEAK_SOURCE_INNER;
  condition1.start = condition.start;
  condition1.end = middle;
  condition1.leftGapPresent  = condition.leftGapPresent;
  condition1.rightGapPresent  = true;

  condition2.source = PEAK_SOURCE_LAST;
  condition2.start = middle;
  condition2.end = condition.end;
  condition2.leftGapPresent  = true;
  condition2.rightGapPresent  = condition.rightGapPresent;
}


void PeakStructure::updateCarDistances(
  const CarModels& models,
  vector<CarDetect>& cars) const
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


void PeakStructure::updateCars(
  CarModels& models,
  vector<CarDetect>& cars,
  CarDetect& car) const
{
  PeakStructure::updateModels(models, car);
  cars.push_back(car);
}


bool PeakStructure::findCarsInInterval(
  const PeakCondition& condition,
  CarModels& models,
  vector<CarDetect>& cars,
  PeakPool& peaks) const
{
  vector<Peak *> peakPtrs;
  peaks.getCandPtrs(condition.start, condition.end, peakPtrs);

  PeakProfile profile;
  profile.make(peakPtrs, condition.source);

/* */
if (! profile.looksLikeTwoCars() && profile.looksLong())
{
  cout << "findCarsInterval: LONG " << peakPtrs.size() << "\n";
  for (auto pp: peakPtrs)
    cout << pp->strQuality(offset);
  cout << "---\n\n";
}
/* */

  if (profile.looksLikeTwoCars())
  {
    // This might become more general in the future.
    PeakCondition condition1, condition2;
    PeakStructure::splitPeaks(peakPtrs, condition, 
      condition1, condition2);

    if (! PeakStructure::findCarsInInterval(condition1, models, 
        cars, peaks))
      return false;

    if (! PeakStructure::findCarsInInterval(condition2, models, 
        cars, peaks))
      return false;

    return true;
  }

  PeakPtrVector peakPtrsNew;
  PeakPtrVector peakPtrsUnused;

  for (auto& recog: recognizers)
  {
    if (profile.match(recog))
    {
      PeakStructure::getWheelsByQuality(peakPtrs, 
        recog.quality, peakPtrsNew, peakPtrsUnused);

      if (peakPtrsNew.size() != recog.numWheels)
        THROW(ERR_ALGO_PEAK_STRUCTURE, "Not " + recog.text);

      CarDetect car;

      if (PeakStructure::findNumberedWheeler(models, condition,
          peakPtrsNew, recog.numWheels, car))
      {
        PeakStructure::updateCars(models, cars, car);
        PeakStructure::updatePeaks(peakPtrsNew, peakPtrsUnused, 
          recog.numWheels);
        return true;
      }
      else
      {
        cout << "Failed to find car among " << recog.text << "\n";
        cout << profile.str();
        return false;
      }
    }
  }

  if (profile.looksEmpty())
  {
    PeakStructure::downgradeAllPeaks(peakPtrs);
    return true;
  }

  cout << profile.str();
  return false;
}


void PeakStructure::findMissingCar(
  const PeakCondition& condition,
  CarModels& models,
  vector<CarDetect>& cars,
  PeakPool& peaks) const
{
  if (condition.start >= condition.end)
    return;

  if (PeakStructure::findCarsInInterval(condition, models, cars, peaks))
    PeakStructure::printRange(condition, "Did " + condition.text);
  else
    PeakStructure::printRange(condition, "Didn't do " + condition.text);
}


void PeakStructure::findMissingCars(
  PeakCondition& condition,
  CarModels& models,
  vector<CarDetect>& cars,
  PeakPool& peaks) const
{
  if (condition.source == PEAK_SOURCE_FIRST)
  {
    condition.start = peaks.firstCandIndex();
    condition.end = cars.front().startValue();
    condition.leftGapPresent = false;
    condition.rightGapPresent = cars.front().hasLeftGap();
    // condition.rightGapPresent = true;
    condition.text = "first whole-car";

    PeakStructure::findMissingCar(condition, models, cars, peaks);
  }
  else if (condition.source == PEAK_SOURCE_INNER)
  {
    condition.text = "intra-gap";

    const unsigned csize = cars.size(); // As cars grows in the loop
    for (unsigned cno = 0; cno+1 < csize; cno++)
    {
      condition.start = cars[cno].endValue();
      condition.end = cars[cno+1].startValue();
      condition.leftGapPresent = cars[cno].hasRightGap();
      condition.rightGapPresent = cars[cno+1].hasLeftGap();

      PeakStructure::findMissingCar(condition, models, cars, peaks);
    }
  }
  else if (condition.source == PEAK_SOURCE_LAST)
  {
    // TODO leftGapPresent is not a given, either.
    condition.start = cars.back().endValue();
    condition.end = peaks.lastCandIndex();
    condition.leftGapPresent = true;
    condition.rightGapPresent = false;
    condition.text = "last whole-car";

    PeakStructure::findMissingCar(condition, models, cars, peaks);
  }
}


void PeakStructure::fillPartialSides(
  CarModels& models,
  vector<CarDetect>& cars)
{
  // Some cars from the initial findWholeCars() run may have been
  // partial (i.e., missing a side gap) on either or both sides.
  // Now that we have filled in the inner cars, we can guess those
  // gaps and also check the partial car types.
  
  for (unsigned cno = 0; cno+1 < cars.size(); cno++)
  {
    CarDetect& car1 = cars[cno];
    CarDetect& car2 = cars[cno+1];
    if (car1.hasRightGap() || car2.hasLeftGap())
      continue;

    const unsigned mid = 
      (car1.lastPeakPlus1() + car2.firstPeakMinus1()) / 2;

    car1.setEndAndGap(mid);
    car2.setStartAndGap(mid);

    PeakStructure::updateModels(models, car1);
    PeakStructure::updateModels(models, car2);
  }
}


void PeakStructure::seekGaps(
  PPciterator pitLeft,
  PPciterator pitRight,
  const PeakPool& peaks,
  PeakCondition& condition) const
{
  // Check for gap in front of pitLeft.
  condition.leftGapPresent = false;
  condition.start = (* pitLeft)->getIndex() - 1;
  if (pitLeft != peaks.candcbegin())
  {
    Peak const * prevPtr = * prev(pitLeft);
    if (prevPtr->isBogey())
    {
      // This rounds to make the cars abut.
      const unsigned d = (* pitLeft)->getIndex() - prevPtr->getIndex();
      condition.start = (* pitLeft)->getIndex() + (d/2) - d;
      condition.leftGapPresent = true;
    }
  }

  // Check for gap after pitRight.
  condition.rightGapPresent = false;
  condition.end = (* pitRight)->getIndex() + 1;
  auto postIter = next(pitRight);
  if (postIter != peaks.candcend())
  {
    Peak const * nextPtr = * postIter;
    if (nextPtr->isBogey())
    {
      condition.end = (* pitRight)->getIndex() + 
        (nextPtr->getIndex() - (* pitRight)->getIndex()) / 2;
      condition.rightGapPresent = true;
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
  vector<CarDetect>& cars,
  PeakPool& peaks) const
{
  // Set up a sliding vector of running peaks.
  vector<PPiterator> runIter;
  PeakPtrVector runPtr;
  PeakCondition condition;

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
      PeakStructure::seekGaps(runIter[0], runIter[3], peaks, condition);

      CarDetect car;
      PeakStructure::findFourWheeler(models, condition, runPtr, car);

      PeakStructure::updateCars(models, cars, car);
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


void PeakStructure::markCars(
  CarModels& models,
  vector<CarDetect>& cars,
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

  // Check open intervals.  Start with inner ones as they are complete.

  PeakCondition condition;

  condition.source = PEAK_SOURCE_INNER;
  PeakStructure::findMissingCars(condition, models, cars, peaks);
  PeakStructure::updateCarDistances(models, cars);
  sort(cars.begin(), cars.end());

  PeakStructure::fillPartialSides(models, cars);

  PeakStructure::printWheelCount(peaks, "Counting");
  PeakStructure::printCars(cars, "after whole-car gaps");
  PeakStructure::printCarStats(models, "after whole-car inner gaps");

  condition.source = PEAK_SOURCE_LAST;
  PeakStructure::findMissingCars(condition, models, cars, peaks);
  PeakStructure::updateCarDistances(models, cars);

  PeakStructure::printWheelCount(peaks, "Counting");
  PeakStructure::printCars(cars, "after trailing whole car");
  PeakStructure::printCarStats(models, "after trailing whole car");

  condition.source = PEAK_SOURCE_FIRST;
  PeakStructure::findMissingCars(condition, models, cars, peaks);
  PeakStructure::updateCarDistances(models, cars);
  sort(cars.begin(), cars.end());

  PeakStructure::fillPartialSides(models, cars);

  PeakStructure::printWheelCount(peaks, "Counting");
  PeakStructure::printCars(cars, "after leading whole car");
  PeakStructure::printCarStats(models, "after leading whole car");


  // TODO Check peak quality and deviations in carStats[0].
  // Detect inner and open intervals that are not done.
  // Could be carStats[0] with the right length etc, but missing peaks.
  // Or could be a new type of car (or multiple cars).
  // Ends come later.

  PeakStructure::printWheelCount(peaks, "Returning");
}


void PeakStructure::updateImperfections(
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
      THROW(ERR_ALGO_CAR_STRUCTURE, "Too many leading peaks: " +
        to_string(num));
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
      THROW(ERR_ALGO_CAR_STRUCTURE, "Too many peaks in middle: " + 
        to_string(num));
  }
}


void PeakStructure::markImperfections(
  const vector<CarDetect>& cars,
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
      PeakStructure::updateImperfections(numPreceding,
        car == cars.begin(), imperf);

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
      PeakStructure::updateImperfections(numMatches,
        car == cars.begin(), imperf);

    if (cand == candcend)
      break;

    // Finally, advance the car.
    while (car != cars.end() && car->carPrecedesPeak(** cand))
      car++;
  }

  cout << "IMPERF " <<
    imperf.numSkipsOfReal << "-" << imperf.numSkipsOfSeen << ", " <<
    imperf.numSpuriousLater << "-" << imperf.numMissingLater << endl;
}


void PeakStructure::printRange(
  const PeakCondition& condition,
  const string& text) const
{
  cout << text << " " << 
    condition.start + offset << "-" << 
    condition.end + offset << endl;
}


void PeakStructure::printWheelCount(
  const PeakPool& peaks,
  const string& text) const
{
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


void PeakStructure::printCarStats(
  const CarModels& models,
  const string& text) const
{
  cout << "Car stats " << text << "\n";
  cout << models.str();
}

