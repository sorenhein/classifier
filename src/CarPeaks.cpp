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
  const Peak& firstBogieLeftIn,
  const Peak& firstBogieRightIn,
  const Peak& secondBogieLeftIn,
  const Peak& secondBogieRightIn)
{
  firstBogieLeft = firstBogieLeftIn;
  firstBogieRight = firstBogieRightIn;
  secondBogieLeft = secondBogieLeftIn;
  secondBogieRight = secondBogieRightIn;
}


void CarPeaks::operator += (const CarPeaks& cp2)
{
  firstBogieLeft += cp2.firstBogieLeft;
  firstBogieRight += cp2.firstBogieRight;
  secondBogieLeft += cp2.secondBogieLeft;
  secondBogieRight += cp2.secondBogieRight;
}


void CarPeaks::getPeaksPtr(CarPeaksPtr& cp)
{
  cp.firstBogieLeftPtr = &firstBogieLeft;
  cp.firstBogieRightPtr = &firstBogieRight;
  cp.secondBogieLeftPtr = &secondBogieLeft;
  cp.secondBogieRightPtr = &secondBogieRight;
}


void CarPeaks::increment(
  const CarPeaksPtr& cp2,
  CarPeaksNumbers& cpn)
{
  if (cp2.firstBogieLeftPtr)
  {
    firstBogieLeft += * cp2.firstBogieLeftPtr;
    cpn.numFirstLeft++;
  }

  if (cp2.firstBogieRightPtr)
  {
    firstBogieRight += * cp2.firstBogieRightPtr;
    cpn.numFirstRight++;
  }

  if (cp2.secondBogieLeftPtr)
  {
    secondBogieLeft += * cp2.secondBogieLeftPtr;
    cpn.numSecondLeft++;
  }

  if (cp2.secondBogieRightPtr)
  {
    secondBogieRight += * cp2.secondBogieRightPtr;
    cpn.numSecondRight++;
  }
}


void CarPeaks::increment(const CarPeaksPtr& cp2)
{
  if (cp2.firstBogieLeftPtr)
    firstBogieLeft += * cp2.firstBogieLeftPtr;

  if (cp2.firstBogieRightPtr)
    firstBogieRight += * cp2.firstBogieRightPtr;

  if (cp2.secondBogieLeftPtr)
    secondBogieLeft += * cp2.secondBogieLeftPtr;

  if (cp2.secondBogieRightPtr)
    secondBogieRight += * cp2.secondBogieRightPtr;
}


void CarPeaks::average(const CarPeaksNumbers& cpn)
{
  if (cpn.numFirstLeft)
    firstBogieLeft /= cpn.numFirstLeft;

  if (cpn.numFirstRight)
    firstBogieRight /= cpn.numFirstRight;

  if (cpn.numSecondLeft)
    secondBogieLeft /= cpn.numSecondLeft;

  if (cpn.numSecondRight)
    secondBogieRight /= cpn.numSecondRight;
}
 

void CarPeaks::average(const unsigned count)
{
  if (count == 0)
    return;

  firstBogieLeft /= count;
  firstBogieRight /= count;
  secondBogieLeft /= count;
  secondBogieRight /= count;
}
 
