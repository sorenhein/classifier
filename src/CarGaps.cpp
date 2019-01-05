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
  leftBogeyGapSet = false;
  midGapSet = false;
  rightBogeyGapSet = false;
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


unsigned CarGaps::logLeftGap(const unsigned leftGapIn)
{
  if (leftGapSet)
  {
    const unsigned d = (leftGapIn >= leftGap ? 
      leftGapIn - leftGap : leftGap - leftGapIn);
    if (d > RESET_SIDE_GAP_TOLERANCE)
    {
      cout << "Left gap attempted reset from " + to_string(leftGap) + 
        " to " + to_string(leftGapIn) << endl;
    }
    return leftGap;
  }

  leftGapSet = true;
  leftGap = leftGapIn;
  return leftGap;
}


void CarGaps::logCore(
  const unsigned leftBogeyGapIn, // Zero if single wheel
  const unsigned midGapIn,
  const unsigned rightBogeyGapIn) // Zero if single wheel
{
  leftBogeyGap = leftBogeyGapIn;
  leftBogeyGapSet = (leftBogeyGapIn > 0);

  midGap = midGapIn;
  midGapSet = (midGapIn > 0);

  rightBogeyGap = rightBogeyGapIn;
  rightBogeyGapSet = (rightBogeyGapIn > 0);
}


unsigned CarGaps::logRightGap(const unsigned rightGapIn)
{
  if (rightGapSet)
  {
    const unsigned d = (rightGapIn >= rightGap ? 
      rightGapIn - rightGap : rightGap - rightGapIn);
    if (d > RESET_SIDE_GAP_TOLERANCE)
    {
      cout << "Right gap attempted reset from " + to_string(rightGap) + 
        " to " + to_string(rightGapIn) << endl;
    }
    return rightGap;
  }

  rightGapSet = true;
  rightGap = rightGapIn;
  return rightGap;
}


void CarGaps::operator += (const CarGaps& cg2)
{
  leftGap += cg2.leftGap;
  leftBogeyGap += cg2.leftBogeyGap;
  midGap += cg2.midGap;
  rightBogeyGap += cg2.rightBogeyGap;
  rightGap += cg2.rightGap;

  if (cg2.leftGapSet)
    leftGapSet = true;
  if (cg2.leftBogeyGapSet)
    leftBogeyGapSet = true;
  if (cg2.midGapSet)
    midGapSet = true;
  if (cg2.rightBogeyGapSet)
    rightBogeyGapSet = true;
  if (cg2.rightGapSet)
    rightGapSet = true;
}


void CarGaps::increment(CarDetectNumbers& cdn) const
{
  if (leftGapSet)
    cdn.numLeftGaps++;
  if (leftBogeyGapSet)
    cdn.numLeftBogeyGaps++;
  if (midGapSet)
    cdn.numMidGaps++;
  if (rightBogeyGapSet)
    cdn.numRightBogeyGaps++;
  if (rightGapSet)
    cdn.numRightGaps++;
}


void CarGaps::reverse()
{
  bool btmp = leftGapSet;
  leftGapSet = rightGapSet;
  rightGapSet = btmp;

  btmp = leftBogeyGapSet;
  leftBogeyGapSet = rightBogeyGapSet;
  rightBogeyGapSet = btmp;

  unsigned tmp = leftGap;
  leftGap = rightGap;
  rightGap = tmp;

  tmp = leftBogeyGap;
  leftBogeyGap = rightBogeyGap;
  rightBogeyGap = tmp;
}


void CarGaps::average(
  const CarDetectNumbers& cdn)
{
  if (cdn.numLeftGaps)
    leftGap /= cdn.numLeftGaps;
  if (cdn.numLeftBogeyGaps)
    leftBogeyGap /= cdn.numLeftBogeyGaps;
  if (cdn.numMidGaps)
    midGap /= cdn.numMidGaps;
  if (cdn.numRightBogeyGaps)
    rightBogeyGap /= cdn.numRightBogeyGaps;
  if (cdn.numRightGaps)
    rightGap /= cdn.numRightGaps;
}


void CarGaps::average(const unsigned count)
{
  if (count == 0)
    return;

  leftGap /= count;
  leftBogeyGap /= count;
  midGap /= count;
  rightBogeyGap /= count;
  rightGap /= count;
}


unsigned CarGaps::leftGapValue() const
{
  return leftGap;
}


unsigned CarGaps::leftBogeyGapValue() const
{
  return leftBogeyGap;
}


unsigned CarGaps::midGapValue() const
{
  return midGap;
}


unsigned CarGaps::rightBogeyGapValue() const
{
  return rightBogeyGap;
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

  // TODO Decide
  /*
  if (ratio <= 1.1f)
    return 0.f;
  else
  */
    return (ratio - 1.f) * (ratio - 1.f);
}


float CarGaps::distance(const CarGaps& cg2) const
{
  return 100.f * (
    CarGaps::relativeComponent(leftGap, cg2.leftGap) +
    CarGaps::relativeComponent(leftBogeyGap, cg2.leftBogeyGap) +
    CarGaps::relativeComponent(midGap, cg2.midGap) +
    CarGaps::relativeComponent(rightBogeyGap, cg2.rightBogeyGap) +
    CarGaps::relativeComponent(rightGap, cg2.rightGap)
      );
}


float CarGaps::reverseDistance(const CarGaps& cg2) const
{
  return 100.f * (
    CarGaps::relativeComponent(leftGap, cg2.rightGap) +
    CarGaps::relativeComponent(leftBogeyGap, cg2.rightBogeyGap) +
    CarGaps::relativeComponent(midGap, cg2.midGap) +
    CarGaps::relativeComponent(rightBogeyGap, cg2.leftBogeyGap) +
    CarGaps::relativeComponent(rightGap, cg2.leftGap)
      );
}


float CarGaps::distanceForGapMatch(const CarGaps& cg2) const
{
  if (leftGapSet != cg2.leftGapSet ||
      leftBogeyGapSet != cg2.leftBogeyGapSet ||
      midGapSet != cg2.midGapSet ||
      rightBogeyGapSet != cg2.rightBogeyGapSet ||
      rightGapSet != cg2.rightGapSet)
    return numeric_limits<float>::max();

  return CarGaps::distance(cg2);
}


float CarGaps::distanceForReverseMatch(const CarGaps& cg2) const
{
  if (leftGapSet != cg2.rightGapSet ||
      leftBogeyGapSet != cg2.rightBogeyGapSet ||
      midGapSet != cg2.midGapSet ||
      rightBogeyGapSet != cg2.leftBogeyGapSet ||
      rightGapSet != cg2.leftGapSet)
    return numeric_limits<float>::max();

  return CarGaps::reverseDistance(cg2);
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
  if (leftGapSet && cgref.leftGapSet)
  {
    if (! CarGaps::checkTwoSided(leftGap, cgref.leftGap, 
        SIDE_GAP_FACTOR, "left gap"))
      return false;

    if (! CarGaps::checkTooShort(leftGap, cgref.leftBogeyGap,
        SIDE_GAP_TO_BOGEY_FACTOR, "left gap vs. bogey"))
      return false;
  }

  if (rightGapSet && cgref.rightGapSet)
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


string CarGaps::strHeader(const bool numberFlag) const
{
  stringstream ss;
  if (numberFlag)
    ss << right << setw(2) << "no";

  ss <<
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


string CarGaps::str(const unsigned no) const
{
  stringstream ss;
  ss << right << setw(2) << right << no << CarGaps::str();
  return ss.str();
}

