#include <iostream>
#include <iomanip>
#include <sstream>

#include "CarGeom.h"


CarGeom::CarGeom()
{
  CarGeom::reset();
}


CarGeom::~CarGeom()
{
}


void CarGeom::reset()
{
  leftGapSet = false;
  rightGapSet = false;

  leftGap = 0;
  leftBogeyGap = 0;
  midGap = 0;
  rightBogeyGap = 0;
  rightGap = 0;
}


void CarGeom::logAll(
  const unsigned leftGapIn,
  const unsigned leftBogeyGapIn, // Zero if single wheel
  const unsigned midGapIn,
  const unsigned rightBogeyGapIn, // Zero if single wheel
  const unsigned rightGapIn)
{
  CarGeom::logLeftGap(leftGapIn);
  CarGeom::logCore(leftBogeyGapIn, midGapIn, rightBogeyGapIn);
  CarGeom::logRightGap(rightGapIn);
}


void CarGeom::logLeftGap(const unsigned leftGapIn)
{
  leftGapSet = true;
  leftGap = leftGapIn;
}


void CarGeom::logCore(
  const unsigned leftBogeyGapIn, // Zero if single wheel
  const unsigned midGapIn,
  const unsigned rightBogeyGapIn) // Zero if single wheel
{
  leftBogeyGap = leftBogeyGapIn;
  midGap = midGapIn;
  rightBogeyGap = rightBogeyGapIn;
}


void CarGeom::logRightGap(const unsigned rightGapIn)
{
  rightGapSet = true;
  rightGap = rightGapIn;
}


void CarGeom::operator += (const CarGeom& cg2)
{
  if (cg2.leftGapSet)
    leftGapSet = true;

  leftGap += cg2.leftGap;

  leftBogeyGap += cg2.leftBogeyGap;
  midGap += cg2.midGap;
  rightBogeyGap += cg2.rightBogeyGap;

  if (cg2.rightGapSet)
    rightGapSet = true;

  rightGap += cg2.rightGap;
}


void CarGeom::average(
  const unsigned numLeftGap,
  const unsigned numCore,
  const unsigned numRightGap)
{
  if (numLeftGap)
    leftGap /= numLeftGap;

  if (numCore)
  {
    leftBogeyGap /= numCore;
    midGap /= numCore;
    rightBogeyGap /= numCore;
  }

  if (numRightGap)
    rightGap /= numRightGap;
}


bool CarGeom::hasLeftGap() const
{
  return leftGapSet;
}


bool CarGeom::hasRightGap() const
{
  return rightGapSet;
}


string CarGeom::strHeader() const
{
  stringstream ss;
  ss << right <<
    setw(6) << "leftg" <<
    setw(6) << "leftb" <<
    setw(6) << "mid" <<
    setw(6) << "rgtb" <<
    setw(6) << "rgtg";
  return ss.str();
}


string CarGeom::str() const
{
  stringstream ss;
  ss << right << 
    setw(6) << leftGap <<
    setw(6) << leftBogeyGap <<
    setw(6) << midGap <<
    setw(6) << rightBogeyGap <<
    setw(6) << rightGap;
  return ss.str();
}

