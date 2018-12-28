#include <iostream>
#include <iomanip>
#include <sstream>

#include "CarDetect.h"


CarDetect::CarDetect()
{
  CarDetect::reset();
}


CarDetect::~CarDetect()
{
}


void CarDetect::reset()
{
  start = 0;
  end = 0;
  peaksPtr.firstBogeyLeftPtr = nullptr;
  peaksPtr.firstBogeyRightPtr = nullptr;
  peaksPtr.secondBogeyLeftPtr = nullptr;
  peaksPtr.secondBogeyRightPtr = nullptr;
  distanceValue = 0.f;
}


void CarDetect::setLimits(
  const unsigned startIn,
  const unsigned endIn)
{
  start = startIn;
  end = endIn;
}


void CarDetect::setStartAndGap(const unsigned startIn)
{
  start = startIn;
  if (peaksPtr.firstBogeyLeftPtr == nullptr)
    return;

  const unsigned index = peaksPtr.firstBogeyLeftPtr->getIndex();
  if (start >= index)
  {
    cout << "CarDetect::setStartAndGap WARNING: index too small\n";
    return;
  }
  CarDetect::logLeftGap(index - start);
}


void CarDetect::setEndAndGap(const unsigned endIn)
{
  end = endIn;
  if (peaksPtr.secondBogeyRightPtr == nullptr)
    return;
  
  const unsigned index = peaksPtr.secondBogeyRightPtr->getIndex();
  if (end <= index)
  {
    cout << "CarDetect::setEndAndGap WARNING: index too large\n";
    return;
  }
  CarDetect::logRightGap(end - index);
}


void CarDetect::logCore(
  const unsigned leftBogeyGap,
  const unsigned midGap,
  const unsigned rightBogeyGap)
{
  gaps.logCore(leftBogeyGap, midGap, rightBogeyGap);
}


void CarDetect::logLeftGap(const unsigned leftGap)
{
  gaps.logLeftGap(leftGap);
}


void CarDetect::logRightGap(const unsigned rightGap)
{
  gaps.logRightGap(rightGap);
}


void CarDetect::logPeakPointers(
  Peak * firstBogeyLeftPtr,
  Peak * firstBogeyRightPtr,
  Peak * secondBogeyLeftPtr,
  Peak * secondBogeyRightPtr)
{
  peaksPtr.firstBogeyLeftPtr = firstBogeyLeftPtr;
  peaksPtr.firstBogeyRightPtr = firstBogeyRightPtr;
  peaksPtr.secondBogeyLeftPtr = secondBogeyLeftPtr;
  peaksPtr.secondBogeyRightPtr = secondBogeyRightPtr;
}


void CarDetect::logStatIndex(const unsigned index)
{
  statIndex = index;
}


void CarDetect::logDistance(const float d)
{
  distanceValue = d;
}


void CarDetect::operator += (const CarDetect& c2)
{
  gaps += c2.gaps;
}


bool CarDetect::operator < (const CarDetect& c2) const
{
  return (start < c2.start);
}


void CarDetect::increment(CarDetectNumbers& cdn) const
{
  if (gaps.hasLeftGap())
    cdn.numLeftGaps++;

  cdn.numCoreGaps++;

  if (gaps.hasRightGap())
    cdn.numRightGaps++;
}


const unsigned CarDetect::startValue() const
{
  if (start > 0)
    return start;
  else 
    return CarDetect::firstPeakMinus1();
}


const unsigned CarDetect::endValue() const
{
  if( end > 0)
    return end;
  else 
    return CarDetect::lastPeakPlus1();
}

// TODO TMP

const unsigned CarDetect::getMidGap() const
{
  return gaps.midGapValue();
}


const unsigned CarDetect::firstPeakMinus1() const
{
  if (peaksPtr.firstBogeyLeftPtr == nullptr)
    return 0;
  else
    // So we miss the first peak of the car.
    return peaksPtr.firstBogeyLeftPtr->getIndex() - 1;
}


const unsigned CarDetect::lastPeakPlus1() const
{
  if (peaksPtr.secondBogeyRightPtr == nullptr)
    return 0;
  else
    // So we miss the last peak of the car.
    return peaksPtr.secondBogeyRightPtr->getIndex() + 1;
}


const CarPeaksPtr& CarDetect::getPeaksPtr() const
{
  return peaksPtr;
}


bool CarDetect::hasLeftGap() const
{
  return gaps.hasLeftGap();
}


bool CarDetect::hasRightGap() const
{
  return gaps.hasRightGap();
}


bool CarDetect::fillSides(
  const unsigned leftGap,
  const unsigned rightGap)
{
  bool filledFlag = false;
  unsigned gap;
  if (leftGap > 0 && 
      leftGap <= peaksPtr.firstBogeyLeftPtr->getIndex())
  {
    gap = gaps.logLeftGap(leftGap);
    if (gap)
    {
      start = peaksPtr.firstBogeyLeftPtr->getIndex() - gap;
      filledFlag = true;
    }
  }

  if (rightGap > 0 &&
      rightGap <= peaksPtr.secondBogeyRightPtr->getIndex())
  {
    gap = gaps.logRightGap(rightGap);
    if (gap)
    {
      end = peaksPtr.secondBogeyRightPtr->getIndex() + gap;
      filledFlag = true;
    }
  }
  return filledFlag;
}


bool CarDetect::fillSides(const CarDetect& cref)
{
  return CarDetect::fillSides(
    cref.gaps.leftGapValue(), cref.gaps.rightGapValue());
}


bool CarDetect::isPartial() const
{
  return gaps.isPartial();
}


void CarDetect::averageGaps(const CarDetectNumbers& cdn)
{
  gaps.average(cdn.numLeftGaps, cdn.numCoreGaps, cdn.numRightGaps);
}


float CarDetect::distance(const CarDetect& cref) const
{
  return gaps.relativeDistance(cref.gaps);
}


bool CarDetect::gapsPlausible(const CarDetect& cref) const
{
  return gaps.gapsPlausible(cref.gaps);
}


bool CarDetect::sideGapsPlausible(const CarDetect& cref) const
{
  return gaps.sideGapsPlausible(cref.gaps);
}


bool CarDetect::midGapPlausible() const
{
  return gaps.midGapPlausible();
}


bool CarDetect::rightBogeyPlausible(const CarDetect& cref) const
{
  return gaps.rightBogeyPlausible(cref.gaps);
}


bool CarDetect::peakPrecedesCar(const Peak& peak) const
{
  return (peak.getIndex() < start);
}


bool CarDetect::carPrecedesPeak(const Peak& peak) const
{
  return (end < peak.getIndex());
}


string CarDetect::strHeaderGaps() const
{
  return gaps.strHeader(true) + "\n";
};


string CarDetect::strHeaderFull() const
{
  stringstream ss;
  ss << 
    setw(2) << right << "no" <<
    setw(6) << "start" <<
    setw(6) << "end" <<
    setw(6) << "len" <<
    gaps.strHeader(false) << 
    setw(6) << "#cs" << 
    setw(6) << "peaks" << 
    setw(6) << "dist" << 
    endl;
  return ss.str();
};


string CarDetect::strGaps(const unsigned no) const
{
  return gaps.str(no) + "\n";
}


string CarDetect::strFull(
  const unsigned carNo,
  const unsigned offset) const
{
  stringstream ss;
  ss << 
    setw(2) << right << carNo <<
    setw(6) << start + offset <<
    setw(6) << end + offset <<
    setw(6) << (end > start ? end-start : 0) <<
    gaps.str() << 
    setw(6) << statIndex << 
    setw(6) << CarDetect::starsQuality() << 
    setw(6) << CarDetect::starsDistance() << 
    endl;
  return ss.str();
}


string CarDetect::strLimits(
  const unsigned offset,
  const string& text) const
{
  return text + ": " +
    to_string(start + offset) + "-" + to_string(end + offset) + "\n";
}


void CarDetect::updateStars(
  const Peak * peakPtr,
  string& best) const
{
  if (peakPtr == nullptr)
    return;

  const string nstar = peakPtr->stars();
  if (nstar.size() < best.size())
    best = nstar;
}


string CarDetect::starsQuality() const
{
  string best(3, '*');
  CarDetect::updateStars(peaksPtr.firstBogeyLeftPtr, best);
  CarDetect::updateStars(peaksPtr.firstBogeyRightPtr, best);
  CarDetect::updateStars(peaksPtr.secondBogeyLeftPtr, best);
  CarDetect::updateStars(peaksPtr.secondBogeyRightPtr, best);
  return best;
}


string CarDetect::starsDistance() const
{
  if (distanceValue <= 1.f)
    return "***";
  else if (distanceValue <= 3.f)
    return "**";
  else if (distanceValue <= 5.f)
    return "*";
  else
    return "";
}

