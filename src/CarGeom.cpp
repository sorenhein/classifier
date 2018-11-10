#include <iostream>
#include <iomanip>
#include <sstream>

#include "CarGeom.h"

#define SIDE_GAP_FACTOR 2.f
#define SIDE_GAP_TO_BOGEY_FACTOR 2.f
#define MID_GAP_TO_BOGEY_FACTOR 0.67f
#define BOGEY_TO_BOGEY_FACTOR 1.2f
#define BOGEY_TO_REF_SOFT_FACTOR 1.5f
#define BOGEY_TO_REF_HARD_FACTOR 1.1f


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


unsigned CarGeom::leftGapValue() const
{
  return leftGap;
}


unsigned CarGeom::rightGapValue() const
{
  return rightGap;
}


bool CarGeom::hasLeftGap() const
{
  return leftGapSet;
}


bool CarGeom::hasRightGap() const
{
  return rightGapSet;
}


float CarGeom::relativeComponent(
  const unsigned a,
  const unsigned b) const
{
  if (a == 0 || b == 0)
    return 0.f;

  const float ratio = (a > b ?
    static_cast<float>(a) / static_cast<float>(b) :
    static_cast<float>(b) / static_cast<float>(a));

  if (ratio <= 1.1f)
    return 0.f;
  else
    return (ratio - 1.f) * (ratio - 1.f);
}


float CarGeom::relativeDistance(const CarGeom& cg2) const
{
  return 100.f * (
    CarGeom::relativeComponent(leftGap, cg2.leftGap) +
    CarGeom::relativeComponent(leftBogeyGap, cg2.leftBogeyGap) +
    CarGeom::relativeComponent(midGap, cg2.midGap) +
    CarGeom::relativeComponent(rightBogeyGap, cg2.rightBogeyGap) +
    CarGeom::relativeComponent(rightGap, cg2.rightGap)
      );
}


bool CarGeom::checkTwoSided(
  const unsigned actual,
  const unsigned reference,
  const float factor,
  const string& text) const
{
  if (actual > factor * reference || actual * factor < reference)
  {
    if (text != "")
      cout << "Suspect " << text << ": " << actual << " vs. " <<
        reference << endl;
    return false;
  }
  else
    return true;
}


bool CarGeom::checkTooShort(
  const unsigned actual,
  const unsigned reference,
  const float factor,
  const string& text) const
{
  if (actual * factor < reference)
  {
    cout << "Suspect " << text << ": " << actual << " vs. " <<
      reference << endl;
    return false;
  }
  else
    return true;
}



bool CarGeom::sideGapsPlausible(const CarGeom& cgref) const
{
  if (leftGapSet)
  {
    if (! CarGeom::checkTwoSided(leftGap, cgref.leftGap, 
        SIDE_GAP_FACTOR, "left gap"))
      return false;

    if (! CarGeom::checkTooShort(leftGap, cgref.leftBogeyGap,
        SIDE_GAP_TO_BOGEY_FACTOR, "left gap vs. bogey"))
      return false;
  }

  if (rightGapSet)
  {
    if (! CarGeom::checkTwoSided(rightGap, cgref.rightGap, 
        SIDE_GAP_FACTOR, "right gap"))
      return false;

    if (! CarGeom::checkTooShort(rightGap, cgref.rightBogeyGap,
        SIDE_GAP_TO_BOGEY_FACTOR, "right gap vs. bogey"))
      return false;
  }
  return true;
}


bool CarGeom::midGapPlausible() const
{
  if (leftBogeyGap > 0)
    return CarGeom::checkTooShort(midGap, leftBogeyGap,
      MID_GAP_TO_BOGEY_FACTOR, "mid-gap vs. left bogey");
  else if (rightBogeyGap > 0)
    return CarGeom::checkTooShort(midGap, rightBogeyGap,
      MID_GAP_TO_BOGEY_FACTOR, "mid-gap vs. right bogey");
  else
    return false;
}


bool CarGeom::rightBogeyPlausible(const CarGeom& cgref) const
{
  return CarGeom::checkTwoSided(rightBogeyGap, cgref.rightBogeyGap,
      BOGEY_TO_REF_SOFT_FACTOR, "");
}


bool CarGeom::rightBogeyConvincing(const CarGeom& cgref) const
{
  return CarGeom::checkTwoSided(rightBogeyGap, cgref.rightBogeyGap,
      BOGEY_TO_REF_HARD_FACTOR, "");
}


bool CarGeom::gapsPlausible(const CarGeom& cgref) const
{
  if (! CarGeom::sideGapsPlausible(cgref))
    return false;

  if (! CarGeom::checkTwoSided(leftBogeyGap, rightBogeyGap,
      BOGEY_TO_BOGEY_FACTOR, "bogey size"))
    return false;
  
  if (! CarGeom::midGapPlausible())
    return false;

  return true;
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

