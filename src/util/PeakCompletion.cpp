#include <sstream>
#include <limits>

#include "PeakCompletion.h"

#include "../Peak.h"


PeakCompletion::PeakCompletion()
{
  PeakCompletion::reset();
}


PeakCompletion::~PeakCompletion()
{
}


void PeakCompletion::reset()
{
  mid = 0;
  lower = std::numeric_limits<unsigned>::max();
  upper = 0;
  _type = COMP_UNMATCHED;
  pptr = nullptr;
}


void PeakCompletion::addMiss(
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


void PeakCompletion::addMatch(
  const unsigned target,
  Peak * ptr)
{
  mid = target;
  pptr = ptr;
}


bool PeakCompletion::markWith(
  Peak& peak,
  const CompletionType typeIn,
  unsigned& dist)
{
  if (_type == COMP_UNMATCHED && peak.match(lower, upper))
  {
    if (typeIn == COMP_UNUSED)
      PeakCompletion::nominate(typeIn, dist, &peak);
    else
      PeakCompletion::nominate(typeIn, dist, nullptr);
    return true;
  }
  else if (_type == COMP_REPAIRABLE && peak.match(lower, upper))
  {
    if (typeIn == COMP_REPAIRED)
    {
      PeakCompletion::nominate(typeIn, dist, &peak);
      return true;
    }
    else
      return false;
  }
  else
    return false;
}


void PeakCompletion::nominate(
  const CompletionType typeIn,
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


bool PeakCompletion::operator > (const PeakCompletion& pc2) const
{
  return (mid > pc2.mid);
}


bool PeakCompletion::consistentWith(const PeakCompletion& pc2)
{
  if (_type == COMP_UNUSED && pc2._type == COMP_UNUSED)
    return pptr == pc2.pptr;
  else if (_type == COMP_REPAIRED && pc2._type == COMP_REPAIRED)
  {
    return pptr == pc2.pptr;
  }
  else
    return (pc2.mid >= lower && pc2.mid <= upper);
}


CompletionType PeakCompletion::source() const
{
  return _type;
}


Peak * PeakCompletion::ptr()
{
  return pptr;
}


void PeakCompletion::fill(Peak& peak)
{
  peak.logPosition(mid, lower, upper);
}


string PeakCompletion::strType() const
{
  if (_type == COMP_UNUSED)
    return "unused";
  else if (_type == COMP_REPAIRABLE)
    return "repairable";
  else if (_type == COMP_REPAIRED)
    return "repaired";
  else if (_type == COMP_UNMATCHED)
    return "unmatched";
  else
    return "";
}


string PeakCompletion::strHeader() const
{
  stringstream ss;
  ss << setw(6) << "Center" <<
    setw(12) << "Range" <<
    setw(12) << "Source" << "\n";
  return ss.str();
}


string PeakCompletion::str(const unsigned offset) const
{
  if (upper == 0)
    return "";

  const string range = to_string(lower + offset) + "-" + 
    to_string(upper + offset);

  stringstream ss;
  ss << setw(6) << mid + offset <<
    setw(12) << right << range <<
    setw(12) << PeakCompletion::strType();
  if (_type == COMP_UNUSED || _type == COMP_REPAIRED)
    ss << setw(12) << (pptr->getIndex()) + offset;
  return ss.str() + "\n";
}

