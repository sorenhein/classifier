#include <iostream>
#include <iomanip>
#include <sstream>

#include "CarGaps.h"
#include "Except.h"
#include "errors.h"

#define SIDE_GAP_FACTOR 2.f
#define SIDE_GAP_TO_BOGEY_FACTOR 2.f
#define MID_GAP_TO_BOGEY_FACTOR 0.67f
#define BOGEY_TO_BOGEY_FACTOR 1.2f
#define BOGEY_TO_REF_SOFT_FACTOR 1.5f
#define BOGEY_TO_REF_HARD_FACTOR 1.1f

#define RESET_SIDE_GAP_TOLERANCE 10 // Samples


CarGaps::CarGaps()
{
  CarGaps::reset();
}


CarGaps::~CarGaps()
{
}


void CarGaps::reset()
{
  leftGapSet = false;
  rightGapSet = false;

  leftGap = 0;
  leftBogeyGap = 0;
  midGap = 0;
  rightBogeyGap = 0;
  rightGap = 0;
}


void CarGaps::logAll(
  const unsigned leftGapIn,
  const unsigned leftBogeyGapIn, // Zero if single wheel
  const unsigned midGapIn,
  const unsigned rightBogeyGapIn, // Zero if single wheel
  const unsigned rightGapIn)
{
  CarGaps::logLeftGap(leftGapIn);
  CarGaps::logCore(leftBogeyGapIn, midGapIn, rightBogeyGapIn);
  CarGaps::logRightGap(rightGapIn);
}


bool CarGaps::logLeftGap(const unsigned leftGapIn)
{
  if (leftGapSet)
  {
    const unsigned d = (leftGapIn >= leftGap ? 
      leftGapIn - leftGap : leftGap - leftGapIn);
    if (d > RESET_SIDE_GAP_TOLERANCE)
      THROW(ERR_CAR_SIDE_GAP_RESET, 
        "Left gap reset from " + to_string(leftGap) + 
        " to " + to_string(leftGapIn));
    return false;
  }

  leftGapSet = true;
  leftGap = leftGapIn;
  return true;
}


void CarGaps::logCore(
  const unsigned leftBogeyGapIn, // Zero if single wheel
  const unsigned midGapIn,
  const unsigned rightBogeyGapIn) // Zero if single wheel
{
  leftBogeyGap = leftBogeyGapIn;
  midGap = midGapIn;
  rightBogeyGap = rightBogeyGapIn;
}


bool CarGaps::logRightGap(const unsigned rightGapIn)
{
  if (rightGapSet)
  {
    const unsigned d = (rightGapIn >= rightGap ? 
      rightGapIn - rightGap : rightGap - rightGapIn);
    if (d > RESET_SIDE_GAP_TOLERANCE)
      THROW(ERR_CAR_SIDE_GAP_RESET, 
        "Right gap reset from " + to_string(rightGap) + 
        " to " + to_string(rightGapIn));
    return false;
  }

  rightGapSet = true;
  rightGap = rightGapIn;
  return true;
}


void CarGaps::operator += (const CarGaps& cg2)
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


void CarGaps::average(
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


unsigned CarGaps::leftGapValue() const
{
  return leftGap;
}


unsigned CarGaps::rightGapValue() const
{
  return rightGap;
}


bool CarGaps::hasLeftGap() const
{
  return leftGapSet;
}


bool CarGaps::hasRightGap() const
{
  return rightGapSet;
}


bool CarGaps::isPartial() const
{
  return (! leftGapSet || ! rightGapSet);
}


float CarGaps::relativeComponent(
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


float CarGaps::relativeDistance(const CarGaps& cg2) const
{
  return 100.f * (
    CarGaps::relativeComponent(leftGap, cg2.leftGap) +
    CarGaps::relativeComponent(leftBogeyGap, cg2.leftBogeyGap) +
    CarGaps::relativeComponent(midGap, cg2.midGap) +
    CarGaps::relativeComponent(rightBogeyGap, cg2.rightBogeyGap) +
    CarGaps::relativeComponent(rightGap, cg2.rightGap)
      );
}


bool CarGaps::checkTwoSided(
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


bool CarGaps::checkTooShort(
  const unsigned actual,
  const unsigned reference,
  const float factor,
  const string& text) const
{
  if (actual * factor < reference)
  {
    if (text != "")
      cout << "Suspect " << text << ": " << actual << " vs. " <<
        reference << endl;
    return false;
  }
  else
    return true;
}



bool CarGaps::sideGapsPlausible(const CarGaps& cgref) const
{
  if (leftGapSet)
  {
    if (! CarGaps::checkTwoSided(leftGap, cgref.leftGap, 
        SIDE_GAP_FACTOR, "left gap"))
      return false;

    if (! CarGaps::checkTooShort(leftGap, cgref.leftBogeyGap,
        SIDE_GAP_TO_BOGEY_FACTOR, "left gap vs. bogey"))
      return false;
  }

  if (rightGapSet)
  {
    if (! CarGaps::checkTwoSided(rightGap, cgref.rightGap, 
        SIDE_GAP_FACTOR, "right gap"))
      return false;

    if (! CarGaps::checkTooShort(rightGap, cgref.rightBogeyGap,
        SIDE_GAP_TO_BOGEY_FACTOR, "right gap vs. bogey"))
      return false;
  }
  return true;
}


bool CarGaps::midGapPlausible() const
{
  if (leftBogeyGap > 0)
    return CarGaps::checkTooShort(midGap, leftBogeyGap,
      MID_GAP_TO_BOGEY_FACTOR, "mid-gap vs. left bogey");
  else if (rightBogeyGap > 0)
    return CarGaps::checkTooShort(midGap, rightBogeyGap,
      MID_GAP_TO_BOGEY_FACTOR, "mid-gap vs. right bogey");
  else
    return false;
}


bool CarGaps::rightBogeyPlausible(const CarGaps& cgref) const
{
  return CarGaps::checkTwoSided(rightBogeyGap, cgref.rightBogeyGap,
      BOGEY_TO_REF_SOFT_FACTOR, "");
}


bool CarGaps::rightBogeyConvincing(const CarGaps& cgref) const
{
  return CarGaps::checkTwoSided(rightBogeyGap, cgref.rightBogeyGap,
      BOGEY_TO_REF_HARD_FACTOR, "");
}


bool CarGaps::gapsPlausible(const CarGaps& cgref) const
{
  if (! CarGaps::sideGapsPlausible(cgref))
    return false;

  if (! CarGaps::checkTwoSided(leftBogeyGap, rightBogeyGap,
      BOGEY_TO_BOGEY_FACTOR, "bogey size"))
    return false;
  
  if (! CarGaps::midGapPlausible())
    return false;

  return true;
}


string CarGaps::strHeader() const
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


string CarGaps::str() const
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

