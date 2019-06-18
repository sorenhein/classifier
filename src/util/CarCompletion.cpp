#include <sstream>
#include <vector>
#include <sstream>

#include "CarCompletion.h"

#include "../Peak.h"


CarCompletion::CarCompletion()
{
  CarCompletion::reset();
}


CarCompletion::~CarCompletion()
{
}


void CarCompletion::reset()
{
  weight = 1;
  // _closestPeaks.clear();
  _limitLower = 0;
  _limitUpper = 0;
}


void CarCompletion::setWeight(const unsigned weightIn)
{
  weight = weightIn;
}


void CarCompletion::addMiss(
  const unsigned target,
  const unsigned tolerance)
{
  peakCompletions.emplace_back(PeakCompletion());
  PeakCompletion& pc = peakCompletions.back();

  pc.addMiss(target, tolerance);
}


#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

void CarCompletion::addMatch(
  const unsigned target,
  Peak * pptr)
{
  // _closestPeaks.push_back(pptr);

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


/*
void CarCompletion::addPeak(Peak& peak)
{
  const unsigned pindex = peak.getIndex();

  if (_closestPeaks.empty() || pindex > _closestPeaks.back()->getIndex())
  {
    _closestPeaks.push_back(&peak);
    return;
  }

  for (auto it = _closestPeaks.begin(); it != _closestPeaks.end(); it++)
  {
    const unsigned index = (* it)->getIndex();
    if (index == pindex)
      return;
    else if (index > pindex)
    {
      _closestPeaks.insert(it, &peak);
      return;
    }
  }
}
*/


void CarCompletion::markWith(
  Peak& peak,
  const CompletionType type)
{
  for (auto& pc: peakCompletions)
  {
    if (pc.markWith(peak, type))
    {
      if (type == COMP_REPAIRABLE)
        CarCompletion::pruneRepairables(pc);
      // else
        // CarCompletion::addPeak(peak);
    }
  }
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


void CarCompletion::setLimits(
  const unsigned limitLowerIn,
  const unsigned limitUpperIn)
{
  _limitLower = limitLowerIn;
  _limitUpper = limitUpperIn;
}


void CarCompletion::getMatch(
  vector<Peak const *>& closestPtrs,
  unsigned& limitLowerOut,
  unsigned& limitUpperOut)
{
  // closestPtr = &_closestPeaks;
  limitLowerOut = _limitLower;
  limitUpperOut = _limitUpper;

  closestPtrs.clear();
  for (auto& pc: peakCompletions)
    closestPtrs.push_back(pc.ptr());
}


bool CarCompletion::condense(CarCompletion& miss2)
{
  // const unsigned nc = _closestPeaks.size();
  const unsigned nc = peakCompletions.size();
  // if (miss2._closestPeaks.size() != nc)
  if (miss2.peakCompletions.size() != nc)
    return false;

  for (auto pc1 = peakCompletions.begin(), pc2 = miss2.peakCompletions.begin();
    pc1 != peakCompletions.end() && pc2 != miss2.peakCompletions.end();
    pc1++, pc2++)
  {
    if (pc1->ptr() != pc2->ptr())
      return false;
  }

  /*
  for (unsigned i = 0; i < nc; i++)
  {
    if (_closestPeaks[i] != miss2._closestPeaks[i])
      return false;
  }
  */

  weight += miss2.weight;

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


bool CarCompletion::nextRepairable(Peak& peak)
{
  if (itRep == repairables.end())
    return false;

  (* itRep)->fill(peak);
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


unsigned CarCompletion::score() const
{
  vector<unsigned> types(COMP_SIZE);
  for (auto& pc: peakCompletions)
    types[pc.source()]++;

  // TODO For example.
  return 
    weight +
    3 * types[COMP_UNUSED] +
    1 * types[COMP_REPAIRABLE];
}


string CarCompletion::str(const unsigned offset) const
{
  stringstream ss;
  ss << "Car with weight " << weight << ", distance TBD ";

  if (peakCompletions.empty())
    ss << " is complete\n\n";
  else
  {
    ss << "\n\n";
    ss << peakCompletions.front().strHeader();
    for (auto& pc: peakCompletions)
      ss << pc.str(offset);
    ss << "\n";
  }

  return ss.str();
}

