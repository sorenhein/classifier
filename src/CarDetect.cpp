#include <iostream>
#include <iomanip>
#include <sstream>

#include "CarDetect.h"
#include "PeakRange.h"


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

  gaps.reset();

  peaksPtr.firstBogieLeftPtr = nullptr;
  peaksPtr.firstBogieRightPtr = nullptr;
  peaksPtr.secondBogieLeftPtr = nullptr;
  peaksPtr.secondBogieRightPtr = nullptr;

  match.distance = numeric_limits<float>::max();
  match.reverseFlag = false;
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
  if (peaksPtr.firstBogieLeftPtr == nullptr)
    return;

  const unsigned index = peaksPtr.firstBogieLeftPtr->getIndex();
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
  if (peaksPtr.secondBogieRightPtr == nullptr)
    return;
  
  const unsigned index = peaksPtr.secondBogieRightPtr->getIndex();
  if (end <= index)
  {
    cout << "CarDetect::setEndAndGap WARNING: index too large\n";
    return;
  }
  CarDetect::logRightGap(end - index);
}


void CarDetect::logCore(
  const unsigned leftBogieGap,
  const unsigned midGap,
  const unsigned rightBogieGap)
{
  gaps.logCore(leftBogieGap, midGap, rightBogieGap);
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
  Peak * firstBogieLeftPtr,
  Peak * firstBogieRightPtr,
  Peak * secondBogieLeftPtr,
  Peak * secondBogieRightPtr)
{
  peaksPtr.firstBogieLeftPtr = firstBogieLeftPtr;
  peaksPtr.firstBogieRightPtr = firstBogieRightPtr;
  peaksPtr.secondBogieLeftPtr = secondBogieLeftPtr;
  peaksPtr.secondBogieRightPtr = secondBogieRightPtr;
}


void CarDetect::logStatIndex(const unsigned index)
{
  match.index = index;
}


void CarDetect::logMatchData(const MatchData& matchIn)
{
  match = matchIn;
}


void CarDetect::makeLastTwoOfFourWheeler(
  const PeakRange& range,
  PeakPtrs& peakPtrsIn) // TODO should be const again
{
  CarDetect::setLimits(range.startValue(), range.endValue());

  vector<unsigned> indices;
  peakPtrsIn.getIndices(indices);
  peakPtrsIn.flattenCar(peaksPtr);

  const unsigned peakNo0 = indices[0];
  const unsigned peakNo1 = indices[1];

  if (range.hasRightGap())
    CarDetect::logRightGap(end - peakNo1);

  CarDetect::logCore(0, 0, peakNo1 - peakNo0);

}


void CarDetect::makeLastThreeOfFourWheeler(
  const PeakRange& range,
  PeakPtrs& peakPtrsIn)
{
  CarDetect::setLimits(range.startValue(), range.endValue());

  vector<unsigned> indices;
  peakPtrsIn.getIndices(indices);
  peakPtrsIn.flattenCar(peaksPtr);

  const unsigned peakNo0 = indices[0];
  const unsigned peakNo1 = indices[1];
  const unsigned peakNo2 = indices[2];

  CarDetect::logCore(0, peakNo1 - peakNo0, peakNo2 - peakNo1);

  if (range.hasRightGap())
    CarDetect::logRightGap(end - peakNo2);

}


void CarDetect::makeFourWheeler(
  const PeakRange& range,
  PeakPtrs& peakPtrs)
{
  CarDetect::setLimits(range.startValue(), range.endValue());

  vector<unsigned> indices;
  peakPtrs.getIndices(indices);

  const unsigned peakNo0 = indices[0];
  const unsigned peakNo1 = indices[1];
  const unsigned peakNo2 = indices[2];
  const unsigned peakNo3 = indices[3];

  if (range.hasLeftGap())
    CarDetect::logLeftGap(peakNo0 - start);

  if (range.hasRightGap())
    CarDetect::logRightGap(end - peakNo3);

  CarDetect::logCore(
    peakNo1 - peakNo0, peakNo2 - peakNo1, peakNo3 - peakNo2);

  peakPtrs.flattenCar(peaksPtr);
}


void CarDetect::makeAnyWheeler(
  const PeakRange& range,
  PeakPtrs& peakPtrs) // TODO Const later on again
{
  CarDetect::setLimits(range.startValue(), range.endValue());

  vector<unsigned> indices;
  peakPtrs.getIndices(indices);
  peakPtrs.flattenCar(peaksPtr);

  if (peaksPtr.firstBogieLeftPtr && range.hasLeftGap())
    CarDetect::logLeftGap(indices[0] - start);

  if (peaksPtr.secondBogieRightPtr && range.hasRightGap())
    CarDetect::logRightGap(end - indices[3]);

  unsigned g1 = 0, g2 = 0, g3 = 0;
  if (peaksPtr.firstBogieLeftPtr && peaksPtr.firstBogieRightPtr)
    g1 = indices[1] - indices[0];
  if (peaksPtr.secondBogieLeftPtr && peaksPtr.firstBogieRightPtr) 
    g2 = indices[2] - indices[1];
  if (peaksPtr.secondBogieLeftPtr && peaksPtr.secondBogieRightPtr)
    g3 = indices[3] - indices[2];

  CarDetect::logCore(g1, g2, g3);
}


void CarDetect::logDistance(const float d)
{
  match.distance = d;
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
  gaps.increment(cdn);
}


void CarDetect::reverse()
{
  gaps.reverse();

  Peak * tmp = peaksPtr.firstBogieLeftPtr;
  peaksPtr.firstBogieLeftPtr = peaksPtr.secondBogieRightPtr;
  peaksPtr.secondBogieRightPtr = tmp;

  tmp = peaksPtr.firstBogieRightPtr;
  peaksPtr.firstBogieRightPtr = peaksPtr.secondBogieLeftPtr;
  peaksPtr.secondBogieLeftPtr = tmp;

  match.reverseFlag = ! match.reverseFlag;
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
  if (end > 0)
    return end;
  else 
    return CarDetect::lastPeakPlus1();
}


const unsigned CarDetect::index() const
{
  return match.index;
}


const bool CarDetect::isReversed() const
{
  return match.reverseFlag;
}


// TODO TMP

const unsigned CarDetect::getLeftBogieGap() const
{
  return gaps.leftBogieGapValue();
}


const unsigned CarDetect::getMidGap() const
{
  return gaps.midGapValue();
}


const unsigned CarDetect::getRightBogieGap() const
{
  return gaps.rightBogieGapValue();
}


const unsigned CarDetect::firstPeakMinus1() const
{
  if (peaksPtr.firstBogieLeftPtr == nullptr)
    return 0;
  else
    // So we miss the first peak of the car.
    return peaksPtr.firstBogieLeftPtr->getIndex() - 1;
}


const unsigned CarDetect::lastPeakPlus1() const
{
  if (peaksPtr.secondBogieRightPtr)
    return peaksPtr.secondBogieRightPtr->getIndex() + 1;
  else if (peaksPtr.secondBogieLeftPtr)
    return peaksPtr.secondBogieLeftPtr->getIndex() + 1;
  else if (peaksPtr.firstBogieRightPtr)
    return peaksPtr.firstBogieRightPtr->getIndex() + 1;
  else if (peaksPtr.firstBogieLeftPtr)
    return peaksPtr.firstBogieLeftPtr->getIndex() + 1;
  else
    return 0;
}


const CarPeaksPtr& CarDetect::getPeaksPtr() const
{
  return peaksPtr;
}


void CarDetect::getCarPoints(list<unsigned>& carPoints) const
{
  carPoints.clear();

  // Left edge of the car.
  carPoints.push_back(0);

  // Left bogie, left wheel.
  unsigned v = gaps.leftGapValue();
  carPoints.push_back(v);

  // Left bogie, right wheel.
  v += gaps.leftBogieGapValue();
  carPoints.push_back(v);

  // Right bogie, left wheel.
  v += gaps.midGapValue();
  carPoints.push_back(v);

  // Right bogie, right wheel.
  v += gaps.rightBogieGapValue();
  carPoints.push_back(v);

  // Right edge of the car.
  v += gaps.rightGapValue();
  carPoints.push_back(v);
}


bool CarDetect::hasLeftGap() const
{
  return gaps.hasLeftGap();
}


bool CarDetect::hasRightGap() const
{
  return gaps.hasRightGap();
}


bool CarDetect::hasLeftBogieGap() const
{
  return gaps.hasLeftBogieGap();
}


bool CarDetect::hasRightBogieGap() const
{
  return gaps.hasRightBogieGap();
}


bool CarDetect::hasMidGap() const
{
  return gaps.hasMidGap();
}


bool CarDetect::isPartial() const
{
  return gaps.isPartial();
}


void CarDetect::averageGaps(const CarDetectNumbers& cdn)
{
  gaps.average(cdn);
}


void CarDetect::averageGaps(const unsigned count)
{
  gaps.average(count);
}


float CarDetect::distance(const CarDetect& cref) const
{
  return gaps.distanceForGapMatch(cref.gaps);
}


void CarDetect::distanceSymm(
  const CarDetect& cref,
  const bool partialFlag,
  MatchData& matchIn) const
{
  if (gaps.isSymmetric())
  {
    if (partialFlag)
      matchIn.distance = gaps.distanceForGapInclusion(cref.gaps);
    else
      matchIn.distance = gaps.distanceForGapMatch(cref.gaps);
    matchIn.reverseFlag = false;
    return;
  }

  float value1, value2;
  if (partialFlag)
  {
    value1 = gaps.distanceForGapInclusion(cref.gaps);
    value2 = gaps.distanceForReverseInclusion(cref.gaps);
  }
  else
  {
    value1 = gaps.distanceForGapMatch(cref.gaps);
    value2 = gaps.distanceForReverseMatch(cref.gaps);
  }

  if (value1 <= value2)
  {
    matchIn.distance = value1;
    matchIn.reverseFlag = false;
  }
  else
  {
    matchIn.distance = value2;
    matchIn.reverseFlag = true;
  }
}


bool CarDetect::distancePartialSymm(
  const PeakPtrs& peakPtrs,
  const float& limit,
  MatchData& matchIn,
  Peak& peakCand) const
{
  const float value1 = gaps.distancePartial(peakPtrs, limit, peakCand);
  const float value2 = gaps.distancePartialReverse(peakPtrs, limit, 
    peakCand);

  if (value1 <= value2 && value1 <= limit)
  {
    matchIn.distance = value1;
    matchIn.reverseFlag = false;
    return true;
  }
  else if (value2 < value1 && value2 <= limit)
  {
    matchIn.distance = value2;
    matchIn.reverseFlag = true;
    return true;
  }
  else
    return false;
}


bool CarDetect::gapsPlausible(const CarDetect& cref) const
{
  return gaps.sideGapsPlausible(cref.gaps);
}


bool CarDetect::corePlausible() const
{
  return gaps.corePlausible();
}


bool CarDetect::sideGapsPlausible(const CarDetect& cref) const
{
  return gaps.sideGapsPlausible(cref.gaps);
}


bool CarDetect::midGapPlausible() const
{
  return gaps.midGapPlausible();
}


bool CarDetect::rightBogiePlausible(const CarDetect& cref) const
{
  return gaps.rightBogiePlausible(cref.gaps);
}


unsigned CarDetect::getGap(
  const bool reverseFlag,
  const bool specialFlag,
  const bool skippedFlag,
  const unsigned peakNo) const
{
  return gaps.getGap(reverseFlag, specialFlag, skippedFlag, peakNo);
}


bool CarDetect::peakPrecedesCar(const Peak& peak) const
{
  return (peak.getIndex() < start);
}


bool CarDetect::carPrecedesPeak(const Peak& peak) const
{
  return (end < peak.getIndex());
}


unsigned CarDetect::numFrontWheels() const
{
  if (gaps.hasLeftGap())
    return 4;
  else if (gaps.hasLeftBogieGap())
    return 4;
  else if (gaps.hasMidGap())
    return 3;
  else if (gaps.hasRightBogieGap())
    return 2;
  else if (hasRightGap())
    return 1;
  else
    return 0;
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
    setw(6) << match.strIndex() << 
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
  CarDetect::updateStars(peaksPtr.firstBogieLeftPtr, best);
  CarDetect::updateStars(peaksPtr.firstBogieRightPtr, best);
  CarDetect::updateStars(peaksPtr.secondBogieLeftPtr, best);
  CarDetect::updateStars(peaksPtr.secondBogieRightPtr, best);
  return best;
}


string CarDetect::starsDistance() const
{
  if (match.distance <= 1.f)
    return "***";
  else if (match.distance <= 3.f)
    return "**";
  else if (match.distance <= 5.f)
    return "*";
  else
    return "";
}

