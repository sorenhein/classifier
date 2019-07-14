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
  adjusted = 0;

  _type = COMP_UNMATCHED;
  pptr = nullptr;
}


void PeakCompletion::addMiss(
  const unsigned target,
  const unsigned tolerance)
{
  // A target of 0 indicates a sentinel, not a real target.
  mid = target;
  lower = (target < tolerance ? 0 : target - tolerance);
  upper = (target == 0 ? 0 : target + tolerance);
  adjusted = target;

  _type = COMP_UNMATCHED;
  pptr = nullptr;
}


void PeakCompletion::addMatch(
  const unsigned target,
  Peak * ptr)
{
  mid = target;
  adjusted = target;
  _type = COMP_USED;
  pptr = ptr;
}


void PeakCompletion::unrepair()
{
  _type = COMP_UNMATCHED;
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


int PeakCompletion::distance() const
{
  const unsigned i = pptr->getIndex();
  return (i >= mid ? static_cast<int>(i-mid) : -static_cast<int>(mid-i));
}


unsigned PeakCompletion::distanceShiftSquared() const
{
  if (! pptr)
    return 0;

  const unsigned pi = pptr->getIndex();
  const unsigned d = (pi >= adjusted ? pi-adjusted : adjusted-pi);
  return (d * d);
}


float PeakCompletion::qualityShape() const
{
  return (pptr ? pptr->qualityShapeValue() : 0.f);
}


float PeakCompletion::qualityPeak() const
{
  return (pptr ? pptr->qualityPeakValue() : 0.f);
}


void PeakCompletion::adjust(const int shift)
{
  if (mid == 0)
    return;

  // Avoid 0 as this is a sentinel.
  adjusted = (shift >= static_cast<int>(mid) ? 1 : mid - shift);
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
    setw(8) << "Qshape" << 
    setw(8) << "Qpeak" << 
    setw(6) << "Dist" << "\n";
  return ss.str();
}


string PeakCompletion::str(const unsigned offset) const
{
  const string range = (upper == 0 ? "-" :
    to_string(lower + offset) + "-" + to_string(upper + offset));

  const unsigned pi = (pptr ? pptr->getIndex() : 0);

  stringstream ss;
  ss << setw(6) << mid + offset <<
    setw(12) << right << range <<
    setw(8) <<  adjusted + offset <<
    setw(12) << PeakCompletion::strType();

  if (pptr)
  {
    const float qshape = pptr->qualityShapeValue();
    const float qpeak = pptr->qualityPeakValue();

    ss << setw(6) << pi + offset << 
      setw(8) << fixed << setprecision(2) << qshape <<
      setw(8) << fixed << setprecision(2) << qpeak <<
      setw(6) << PeakCompletion::distanceShiftSquared() << 
      "\n";
  }
  else
  {
    ss << setw(6) << "-" << 
      setw(8) << "-" << 
      setw(8) << "-" << 
      setw(6) << "-" << 
      "\n";
  }
  return ss.str();
}

