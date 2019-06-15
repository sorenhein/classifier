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
  mid = target;

  if (target < tolerance)
    lower = 0;
  else
    lower = target - tolerance;

  upper = target + tolerance;
}

bool MissPeak::markWith(
  Peak& peak,
  const MissType typeIn)
{
  if (_type == MISS_UNMATCHED && peak.match(lower, upper))
  {
    MissPeak::nominate(typeIn, &peak);
    return true;
  }
  else
    return false;
}


void MissPeak::nominate(
  const MissType typeIn,
  Peak * pptrIn)
{
  _type = typeIn;
  if (pptrIn != nullptr)
    pptr = pptrIn;
}


bool MissPeak::operator > (const MissPeak& miss2) const
{
  return (mid > miss2.mid);
}


bool MissPeak::consistentWith(const MissPeak& miss2)
{
  return (miss2.mid >= lower && miss2.mid <= upper);
}


MissType MissPeak::source() const
{
  return _type;
}


Peak * MissPeak::ptr()
{
  return pptr;
}


void MissPeak::fill(Peak& peak)
{
  peak.logPosition(mid, lower, upper);
}


string MissPeak::strType() const
{
  if (_type == MISS_UNUSED)
    return "unused";
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


string MissPeak::str(const unsigned offset) const
{
  if (upper == 0)
    return "";

  const string range = to_string(lower + offset) + "-" + 
    to_string(upper + offset);

  stringstream ss;
  ss << setw(6) << mid + offset <<
    setw(12) << right << range <<
    setw(12) << MissPeak::strType() << "\n";
  return ss.str();
}

