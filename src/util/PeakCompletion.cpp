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
  lower = (target < tolerance ? 0 : target - tolerance);
  upper = target + tolerance;
  _type = COMP_UNMATCHED;
  pptr = nullptr;
}


void PeakCompletion::addMatch(
  const unsigned target,
  Peak * ptr)
{
  mid = target;
  _type = COMP_USED;
  pptr = ptr;
}


bool PeakCompletion::markWith(
  Peak& peak,
  const CompletionType typeIn)
{
  if (_type == COMP_UNMATCHED && peak.match(lower, upper))
  {
    _type = typeIn;
    if (typeIn == COMP_UNUSED)
      pptr = &peak;
    return true;
  }
  else if (_type == COMP_REPAIRABLE && peak.match(lower, upper))
  {
    if (typeIn == COMP_REPAIRED)
    {
      _type = typeIn;
      pptr = &peak;
      return true;
    }
    else
      return false;
  }
  else
    return false;
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
    return pptr == pc2.pptr;
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
  if (_type == COMP_USED)
    return "used";
  else if (_type == COMP_UNUSED)
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
    setw(8) << "Adjust" <<
    setw(12) << "Source" << 
    setw(6) << "Peak" << 
    setw(6) << "Dist" << "\n";
  return ss.str();
}


string PeakCompletion::str(const unsigned offset) const
{
  const string range = (upper == 0 ? "-" :
    to_string(lower + offset) + "-" + to_string(upper + offset));

  const unsigned pi = (pptr ? pptr->getIndex() + offset : 0);
  unsigned d;
  if (pi >= mid)
    d = (pi - mid) * (pi - mid);
  else
    d = (mid - pi) * (mid - pi);

  stringstream ss;
  ss << setw(6) << mid + offset <<
    setw(12) << right << range <<
    setw(8) <<  mid << // For now
    setw(12) << PeakCompletion::strType() <<
    setw(6) << (pptr ? to_string(pi) : "-") << 
    setw(6) << (pptr ? to_string(d) : "-") << "\n";
  return ss.str();
}

