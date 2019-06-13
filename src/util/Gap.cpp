#include <sstream>
#include <limits>

#include "Gap.h"

#include "../const.h"


Gap::Gap()
{
  Gap::reset();
}


Gap::~Gap()
{
}


void Gap::reset()
{
  mid = 0;
  lower = std::numeric_limits<unsigned>::max();
  upper = 0;
  _count = 0;
}


void Gap::setLocation(const unsigned midIn)
{
  mid = midIn;
  lower = static_cast<unsigned>(mid / GAP_FACTOR) - 1;
  upper = static_cast<unsigned>(mid * GAP_FACTOR) + 1;
}


void Gap::set(
  const unsigned midIn,
  const unsigned countIn)
{
  Gap::setLocation(midIn);
  _count = countIn;
}


void Gap::expand(const unsigned value)
{
  if (value < lower)
    lower = value;
  if (value > upper)
    upper = value;

  mid = (lower + upper) / 2;
}


bool Gap::extend(const unsigned midIn)
{
  // Possibly pull the gap somewhat beyond its usual tolerance
  // if it looks as if it's the same gap.
  
  const unsigned lowerIn = static_cast<unsigned>(midIn / GAP_FACTOR);
  const unsigned upperIn = static_cast<unsigned>(midIn * GAP_FACTOR);

  const unsigned ourLowerExt = static_cast<unsigned>(lower / GAP_FACTOR);
  const unsigned ourUpperExt = static_cast<unsigned>(lower * GAP_FACTOR);

  if (Gap::overlap(lowerIn, upperIn))
  {
    // Extend on either end.
    if (lowerIn < lower)
      lower = lowerIn;
    if (upperIn > upper)
      upper = upperIn;
  }
  else if (upperIn >= ourLowerExt && upperIn <= lower)
  {
    // Extend on the low end.
    lower = upperIn - 1;
  }
  else if (lowerIn >= upper && lowerIn <= ourUpperExt)
  {
    // Extend on the high end.
    upper = lowerIn + 1;
  }
  else if (lowerIn >= ourLowerExt && lowerIn <= lower)
  {
    // Extend on the low end.
    lower = lowerIn - 1;
  }
  else if (upperIn >= upper && upperIn <= ourUpperExt)
  {
    // Extend on the high end.
    upper = upperIn + 1;
  }
  else
    return false;

  return true;
}


void Gap::recenter(const Gap& g2)
{
  if (g2.upper <= static_cast<unsigned>(GAP_FACTOR_SQUARED * g2.lower))
  {
    // g2 is narrower, so re-center around its midpoint.
    mid = g2.mid;
    lower = static_cast<unsigned>(mid / GAP_FACTOR) - 1;
    upper = static_cast<unsigned>(mid * GAP_FACTOR) + 1;
  }
  else
  {
    // Take over values from g2.
    mid = g2.mid;
    lower = g2.lower;
    upper = g2.upper;
  }
}


unsigned Gap::center() const
{
  return mid;
}


unsigned Gap::count() const
{
  return _count;
}


float Gap::ratio(const unsigned value) const
{
  if (Gap::openRight())
    return 0.f;
  else
    return value / static_cast<float>(upper);
}


bool Gap::openLeft() const
{
  return (lower == std::numeric_limits<unsigned>::max());
}


bool Gap::openRight() const
{
  return (upper == 0);
}


bool Gap::empty() const
{
  return (Gap::openLeft() && Gap::openRight());
}


bool Gap::startsBelow(const unsigned value) const
{
  return (lower < value);
}


bool Gap::wellBelow(const unsigned value) const
{
  return (upper * GAP_FACTOR <= value);
}


bool Gap::wellAbove(const unsigned value) const
{
  return (lower >= value * GAP_FACTOR);
}


bool Gap::overlap(
  const unsigned shortest,
  const unsigned longest) const
{
  return (shortest <= upper && longest >= lower);
}


bool Gap::overlap(const Gap& g2) const
{
  return (lower <= g2.upper && upper >= g2.lower);
}


string Gap::str(const string& text) const
{
  stringstream ss;
  if (text != "")
    ss << text << " ";

  if (Gap::openLeft())
  {
    if (Gap::openRight())
      ss << "empty";
    else
      ss << "up to " << mid << " (" << _count << ")";
  }
  else if (Gap::openRight())
    ss << "from " << mid << " (" << _count << ")";
  else
    ss << "mid " << mid << ", " <<
      lower << " - " << upper <<
      " (" << _count << ")";

  return ss.str() + "\n";
}

