#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakRange.h"
#include "CarDetect.h"


PeakRange2::PeakRange2()
{
  PeakRange2::reset();
}


PeakRange2::~PeakRange2()
{
}


void PeakRange2::reset()
{
  profile.reset();
  peakPtrs.clear();
  peakIters.clear();
}


void PeakRange2::init(
  const list<CarDetect>& cars,
  const PeakPool& peaks)
{
  carAfter = cars.begin();
  source = PEAK_SOURCE_SIZE;
  start = peaks.firstCandIndex();
  end = peaks.lastCandIndex();
  leftGapPresent = false;
  rightGapPresent = false;
  leftOriginal = true;
  rightOriginal = true;

  PeakRange2::reset();
}


void PeakRange2::fill(const PeakPool& peaks)
{
  // Set up some useful stuff for all recognizers.
  peaks.getCands(start, end, peakPtrs, peakIters);
  profile.make(peakPtrs, source);
}


void PeakRange2::shortenLeft(const CarDetect& car)
{
  // Shorten the range on the left to make room for the new
  // car preceding it.  This does not change any carAfter values.
  start = car.endValue() + 1;
  leftGapPresent = car.hasRightGap();
  leftOriginal = false;
}


void PeakRange2::shortenRight(
  const CarDetect& car,
  const list<CarDetect>::iterator& carIt)
{
  // Shorten the range on the right to make room for the new
  // car following it.
  carAfter = carIt;
  end = car.startValue() - 1;
  rightGapPresent = car.hasLeftGap();
  rightOriginal = false;
}


bool PeakRange2::updateImperfections(
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


string PeakRange2::strInterval(
  const unsigned offset,
  const string& text) const
{
  return text + " " +
    to_string(start + offset) + "-" +
    to_string(end + offset) + "\n";
}


string PeakRange2::strPos() const
{
  if (leftOriginal)
    return "first car";
  else if (rightOriginal)
    return "last car";
  else
    return "intra car";
}


string PeakRange2::strFull(const unsigned offset) const
{
  return PeakRange2::strPos() + ": " +
    (leftGapPresent ? "(gap)" : "(no gap)") + " " +
    to_string(start + offset) + "-" +
    to_string(end + offset) + " " +
    (rightGapPresent ? "(gap)" : "(no gap)") + " " +
    "\n";
}


string PeakRange2::strProfile() const
{
  return profile.str();
}

