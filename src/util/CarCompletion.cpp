#include <sstream>
#include <vector>
#include <sstream>

#include "CarCompletion.h"

#include "../Peak.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


CarCompletion::CarCompletion()
{
  CarCompletion::reset();
}


CarCompletion::~CarCompletion()
{
}


void CarCompletion::reset()
{
  weight = 0;
  _forceFlag = false;
  data.clear();

  _limitLower = 0;
  _limitUpper = 0;
}


void CarCompletion::addMiss(
  const unsigned target,
  const unsigned tolerance)
{
  peakCompletions.emplace_back(PeakCompletion());
  PeakCompletion& pc = peakCompletions.back();

  pc.addMiss(target, tolerance);
}


void CarCompletion::addMatch(
  const unsigned target,
  Peak * pptr)
{
  peakCompletions.emplace_back(PeakCompletion());
  PeakCompletion& pc = peakCompletions.back();

  pc.addMatch(target, pptr);
}


Miterator CarCompletion::begin()
{
  return peakCompletions.begin();
}


Miterator CarCompletion::end()
{
  return peakCompletions.end();
}


void CarCompletion::markWith(
  Peak& peak,
  const CompletionType type,
  const bool forceFlag)
{
  // _forceFlag says whether the whole completion is prepared to
  // force peaks back into existence (in at least one instance).
  // forceFlag says whether the particular peak arose with force or
  // not.  But as it might also have arisen without force, we
  // actually can't make a lot of this.  It probably doesn't matter
  // much in practice anyway.  Otherwise, if we were sure that the
  // peak only would have arisen with force, we would not revive it
  // if data.forceFlag was false.

  UNUSED(forceFlag);
  for (auto& pc: peakCompletions)
  {
    if (pc.markWith(peak, type))
    {
      if (type == COMP_REPAIRABLE)
        CarCompletion::pruneRepairables(pc);
    }
  }
}


unsigned CarCompletion::filled() const
{
  unsigned f = 0;
  for (auto& pc: peakCompletions)
  {
    const auto s = pc.source();
    if (s == COMP_USED || s == COMP_UNUSED || s == COMP_REPAIRED)
      f++;
  }
  return f;
}


bool CarCompletion::complete() const
{
  for (auto& pc: peakCompletions)
  {
    if (pc.source() == COMP_UNMATCHED)
      return false;
  }
  return true;
}


void CarCompletion::setData(const Target& target)
{
  target.limits(_limitLower, _limitUpper);
  weight = target.getData().weight;
  data.push_back(target.getData());
}


bool CarCompletion::operator < (const CarCompletion& comp2) const
{
  // Sort first by count, then by model number, then by reverseFlag.
  return (CarCompletion::filled() >= comp2.filled());
}


void CarCompletion::sort()
{
  data.sort();
}


void CarCompletion::getMatch(
  vector<Peak const *>& closestPtrs,
  unsigned& limitLowerOut,
  unsigned& limitUpperOut)
{
  closestPtrs.clear();
  for (auto& pc: peakCompletions)
    closestPtrs.push_back(pc.ptr());

  limitLowerOut = _limitLower;
  limitUpperOut = _limitUpper;
}


bool CarCompletion::forceFlag() const
{
  return _forceFlag;
}


bool CarCompletion::condense(CarCompletion& miss2)
{
  const unsigned nc = peakCompletions.size();
  if (miss2.peakCompletions.size() != nc)
    return false;

  for (auto pc1 = peakCompletions.begin(), 
      pc2 = miss2.peakCompletions.begin();
      pc1 != peakCompletions.end() && 
      pc2 != miss2.peakCompletions.end();
      pc1++, pc2++)
  {
    if (pc1->ptr() != pc2->ptr())
      return false;
  }

// cout << "Weight before merge: " << weight << endl;

// TODO When all 4 peaks are accounted for, reverseFlag can be
// ignored.

  // Merge the two lists of origins.
  for (auto& d2: miss2.data)
  {
    bool seenFlag = false;
    for (auto& d1: data)
    {
      if (d1.modelNo == d2.modelNo && d1.reverseFlag == d2.reverseFlag)
      {
        seenFlag = true;
        break;
      }
    }

    if (! seenFlag)
    {
      // Could then forget about this d2 here (efficiency).
      weight += d2.weight;
// cout << "Merging " << d2.modelNo << ", " << d2.reverseFlag << " weight " <<
  // d2.weight << ", now " << weight << endl;
      if (d2.forceFlag)
        _forceFlag = true;
      data.push_back(d2);
    }
  }

  return true;
}


void CarCompletion::makeRepairables()
{
  repairables.clear();
  for (auto& pc: peakCompletions)
  {
    if (pc.source() == COMP_UNMATCHED)
      repairables.push_back(&pc);
  }
  itRep = repairables.begin();
}


bool CarCompletion::nextRepairable(
  Peak& peak,
  bool& forceFlag)
{
  if (itRep == repairables.end())
    return false;

  (* itRep)->fill(peak);
  forceFlag = _forceFlag;
  itRep++;
  return true;
}


void CarCompletion::pruneRepairables(PeakCompletion& pc)
{
  if (itRep == repairables.end() || ** itRep > pc)
    return; // No point
  else if (* itRep == &pc)
    itRep = repairables.erase(itRep);
  else
  {
    for (auto it = next(itRep); it != repairables.end(); it++)
    {
      if (* itRep == &pc)
      {
        repairables.erase(it);
        return;
      }
    }
  }
}


void CarCompletion::makeShift()
{
  int sum = 0;
  int npos = 0;
  int nneg = 0;
  int nall = 0;

  // Make a list of the distances.
  for (auto& pc: peakCompletions)
  {
    if (! pc.ptr())
      continue;

    const int d = pc.distance();
    sum += d;
    if (d > 0)
      npos++;
    if (d < 0)
      nneg++;
    nall++;
  }

  // If they are systematically shifted, compensate for this.
  int shift = 0;
  if (nall > 0 && (npos == 0 || nneg == 0))
    shift = sum / nall;

  for (auto& pc: peakCompletions)
    pc.adjust(-shift);
}


unsigned CarCompletion::distanceShiftSquared() const
{
  unsigned dist = 0;
  for (auto& pc: peakCompletions)
    dist += pc.distanceShiftSquared();
  return dist;
}


string CarCompletion::str(const unsigned offset) const
{
  stringstream ss;
  ss << "Car with weight " << weight << 
    ", squared distance " << CarCompletion::distanceShiftSquared();

  if (peakCompletions.empty())
    return ss.str() + " is empty\n\n";

  string smprev = "", sm;
  for (auto& d: data)
  {
    sm = d.strModel();
    if (sm != smprev)
    {
      ss << "\n" << sm << ":";
      smprev = sm;
    }
    ss << d.strSource();
  }

  ss << "\n\n";
  ss << peakCompletions.front().strHeader();
  for (auto& pc: peakCompletions)
    ss << pc.str(offset);
  ss << "\n";

  return ss.str();
}

