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
  const MissType typeIn,
  unsigned& dist)
{
  if (_type == MISS_UNMATCHED && peak.match(lower, upper))
  {
    if (typeIn == MISS_UNUSED)
      MissPeak::nominate(typeIn, dist, &peak);
    else
      MissPeak::nominate(typeIn, dist, nullptr);
    return true;
  }
  else if (_type == MISS_REPAIRABLE && peak.match(lower, upper))
  {
    if (typeIn == MISS_REPAIRED)
    {
      MissPeak::nominate(typeIn, dist, &peak);
      return true;
    }
    else
      return false;
  }
  else
    return false;
}


void MissPeak::nominate(
  const MissType typeIn,
  unsigned& dist,
  Peak * pptrIn)
{
  _type = typeIn;
  if (pptrIn != nullptr)
  {
    pptr = pptrIn;
    const unsigned i = pptr->getIndex();
    const unsigned d = (i >= mid ? i-mid : mid-i);
    dist = d*d;
  }
}


bool MissPeak::operator > (const MissPeak& miss2) const
{
  return (mid > miss2.mid);
}


bool MissPeak::consistentWith(const MissPeak& miss2)
{
  if (_type == MISS_UNUSED && miss2._type == MISS_UNUSED)
    return pptr == miss2.pptr;
  else if (_type == MISS_REPAIRED && miss2._type == MISS_REPAIRED)
  {
    return pptr == miss2.pptr;
  }
  else
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
  else if (_type == MISS_REPAIRED)
    return "repaired";
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
    setw(12) << MissPeak::strType();
  if (_type == MISS_UNUSED || _type == MISS_REPAIRED)
    ss << setw(12) << (pptr->getIndex()) + offset;
  return ss.str() + "\n";
}

