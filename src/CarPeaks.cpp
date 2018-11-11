#include <iostream>
#include <iomanip>
#include <sstream>

#include "CarPeaks.h"


CarPeaks::CarPeaks()
{
  CarPeaks::reset();
}


CarPeaks::~CarPeaks()
{
}


void CarPeaks::reset()
{
}


void CarPeaks::logAll(
  const Peak& firstBogeyLeftIn,
  const Peak& firstBogeyRightIn,
  const Peak& secondBogeyLeftIn,
  const Peak& secondBogeyRightIn)
{
  firstBogeyLeft = firstBogeyLeftIn;
  firstBogeyRight = firstBogeyRightIn;
  secondBogeyLeft = secondBogeyLeftIn;
  secondBogeyRight = secondBogeyRightIn;
}


void CarPeaks::operator += (const CarPeaks& cp2)
{
  firstBogeyLeft += cp2.firstBogeyLeft;
  firstBogeyRight += cp2.firstBogeyRight;
  secondBogeyLeft += cp2.secondBogeyLeft;
  secondBogeyRight += cp2.secondBogeyRight;
}


void CarPeaks::getPeaksPtr(CarPeaksPtr& cp)
{
  cp.firstBogeyLeftPtr = &firstBogeyLeft;
  cp.firstBogeyRightPtr = &firstBogeyRight;
  cp.secondBogeyLeftPtr = &secondBogeyLeft;
  cp.secondBogeyRightPtr = &secondBogeyRight;
}


void CarPeaks::increment(
  const CarPeaksPtr& cp2,
  CarPeaksNumbers& cpn)
{
  if (cp2.firstBogeyLeftPtr)
  {
    firstBogeyLeft += * cp2.firstBogeyLeftPtr;
    cpn.numFirstLeft++;
  }

  if (cp2.firstBogeyRightPtr)
  {
    firstBogeyRight += * cp2.firstBogeyRightPtr;
    cpn.numFirstRight++;
  }

  if (cp2.secondBogeyLeftPtr)
  {
    secondBogeyLeft += * cp2.secondBogeyLeftPtr;
    cpn.numSecondLeft++;
  }

  if (cp2.secondBogeyRightPtr)
  {
    secondBogeyRight += * cp2.secondBogeyRightPtr;
    cpn.numSecondRight++;
  }
}


void CarPeaks::average(const CarPeaksNumbers& cpn)
{
  if (cpn.numFirstLeft)
    firstBogeyLeft /= cpn.numFirstLeft;

  if (cpn.numFirstRight)
    firstBogeyRight /= cpn.numFirstRight;

  if (cpn.numSecondLeft)
    secondBogeyLeft /= cpn.numSecondLeft;

  if (cpn.numSecondRight)
    secondBogeyRight /= cpn.numSecondRight;
}
 
