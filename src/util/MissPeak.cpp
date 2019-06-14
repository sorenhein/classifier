#include <sstream>
#include <limits>

#include "MissPeak.h"

#include "../Peak.h"


MissPeak::MissPeak()
{
  MissPeak::reset();
}


MissPeak::~MissPeak()
{
}


void MissPeak::reset()
{
  mid = 0;
  lower = std::numeric_limits<unsigned>::max();
  upper = 0;
  _type = MISS_UNMATCHED;
  pptr = nullptr;
}


void MissPeak::set(
  const unsigned target,
  const unsigned tolerance)
{
  if (target < tolerance)
    lower = 0;
  else
    lower = target - tolerance;

  upper = target + tolerance;
}


void MissPeak::nominate(
  const MissType typeIn,
  Peak * pptrIn)
{
  _type = typeIn;
  if (pptrIn != nullptr)
    pptr = pptrIn;
}


MissType MissPeak::source() const
{
  return _type;
}


Peak * MissPeak::ptr()
{
  return pptr;
}


string MissPeak::strType() const
{
  if (_type == MISS_UNUSED)
    return "unused";
  else if (_type == MISS_USED)
    return "used";
  else if (_type == MISS_REPAIRABLE)
    return "repairable";
  else if (_type == MISS_UNMATCHED)
    return "unmatched";
  else
    return "";
}


string MissPeak::strHeader() const
{
  stringstream ss;
  ss << setw(6) << "Center" <<
    setw(12) << "Range" <<
    setw(12) << "Source" << "\n";
  return ss.str();
}


string MissPeak::str() const
{
  if (upper == 0)
    return "";

  const string range = to_string(lower) + "-" + to_string(upper);

  stringstream ss;
  ss << setw(6) << mid <<
    setw(12) << right << range <<
    setw(12) << left << MissPeak::strType() << "\n";
  return ss.str();
}

