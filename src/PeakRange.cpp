#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakRange.h"
#include "CarDetect.h"


PeakRange::PeakRange()
{
  PeakRange::reset();
}


PeakRange::~PeakRange()
{
}


void PeakRange::reset()
{
  start = 0;
  endVal = 0;
  leftGapPresent = false;
  rightGapPresent = false;

  peakPtrs.clear();
  peakIters.clear();
  profile.reset();
}


void PeakRange::init(
  const list<CarDetect>& cars,
  const PeakPtrs& candidates)
{
  carAfter = cars.begin();
  source = PEAK_SOURCE_FIRST;
  start = candidates.firstIndex();
  endVal = candidates.lastIndex();
  leftGapPresent = false;
  rightGapPresent = false;
  leftOriginal = true;
  rightOriginal = true;
}


void PeakRange::init(const PeakPtrs& candidates)
{
  start = candidates.firstIndex() - 1;
  endVal = candidates.lastIndex() + 1;
}


void PeakRange::fill(PeakPool& peaks)
{
  // Set up some useful stuff for all recognizers.
  PeakPtrs ppl;
  peaks.candidates().fill(start, endVal, peakPtrs, peakIters);
  vector<Peak *> peaksFlattened;
  peakPtrs.flattenTODO(peaksFlattened);
  profile.make(peaksFlattened, source);
}


void PeakRange::shortenLeft(const CarDetect& car)
{
  // Shorten the range on the left to make room for the new
  // car preceding it.  This does not change any carAfter values.
  source = PEAK_SOURCE_INNER;
  start = car.endValue() + 1;
  leftGapPresent = car.hasRightGap();
  leftOriginal = false;
}


void PeakRange::shortenRight(
  const CarDetect& car,
  const list<CarDetect>::iterator& carIt)
{
  // Shorten the range on the right to make room for the new
  // car following it.
  carAfter = carIt;
  endVal = car.startValue() - 1;
  rightGapPresent = car.hasLeftGap();
  rightOriginal = false;
}


const CarListConstIter& PeakRange::carAfterIter() const
{
  return carAfter;
}


unsigned PeakRange::startValue() const
{
  return start;
}


unsigned PeakRange::endValue() const
{
  return endVal;
}


bool PeakRange::hasLeftGap() const
{
  return leftGapPresent;
}


bool PeakRange::hasRightGap() const
{
  return rightGapPresent;
}


unsigned PeakRange::numGreat() const
{
  return profile.numGreat();
}


unsigned PeakRange::numGood() const
{
  return profile.numGood();
}


PeakPtrs& PeakRange::getPeakPtrs()
{
  return peakPtrs;
}


void PeakRange::split(
  const PeakFncPtr& fptr,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  peakPtrs.split(&Peak::isWheel, fptr, peakPtrsUsed, peakPtrsUnused);
}


bool PeakRange::isFirstCar() const
{
  return (source == PEAK_SOURCE_FIRST);
}


bool PeakRange::isLastCar() const
{
  return (! leftOriginal && rightOriginal);
}


bool PeakRange::match(const Recognizer& recog) const
{
  return profile.match(recog);
}


bool PeakRange::looksEmpty() const
{
  return profile.looksEmpty();
}


bool PeakRange::looksEmptyLast() const
{
  if (! PeakRange::isLastCar())
    return false;

  return profile.looksEmptyLast();
}


bool PeakRange::updateImperfections(
  const list<CarDetect>& cars,
  Imperfections& imperf) const
{
  const unsigned n = profile.numSelected();
  if (cars.empty() || start < cars.front().startValue())
  {
    // We probably missed a car in front of this one.
    if (n <= 4)
      imperf.numSkipsOfSeen += 4 - n;
    else
      return false;
  }
  else if (n <= 4)
    imperf.numMissingLater += 4 - n;
  else if (n == 5)
    imperf.numSpuriousLater++;
  else
    return false;

  return true;
}


string PeakRange::strInterval(
  const unsigned offset,
  const string& text) const
{
  return text + " " +
    to_string(start + offset) + "-" +
    to_string(endVal + offset) + "\n";
}


string PeakRange::strPos() const
{
  if (leftOriginal)
    return "first car";
  else if (rightOriginal)
    return "last car";
  else
    return "intra car";
}


string PeakRange::strFull(const unsigned offset) const
{
  return PeakRange::strPos() + ": " +
    (leftGapPresent ? "(gap)" : "(no gap)") + " " +
    to_string(start + offset) + "-" +
    to_string(endVal + offset) + " " +
    (rightGapPresent ? "(gap)" : "(no gap)") + " " +
    "\n";
}


string PeakRange::strProfile() const
{
  return profile.str();
}

