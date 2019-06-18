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
  distance = 0;
  _closestPeaks.clear();
  _limitLower = 0;
  _limitUpper = 0;
}


void CarCompletion::setWeight(const unsigned weightIn)
{
  weight = weightIn;
}


void CarCompletion::setDistance(const unsigned dist)
{
  distance = dist;
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
  UNUSED(target);

  _closestPeaks.push_back(pptr);

  // pc.addMatch(target, pptr);
}


Miterator CarCompletion::begin()
{
  return peakCompletions.begin();
}


Miterator CarCompletion::end()
{
  return peakCompletions.end();
}


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


void CarCompletion::markWith(
  Peak& peak,
  const CompletionType type)
{
  unsigned dist;
  for (auto& pc: peakCompletions)
  {
    if (pc.markWith(peak, type, dist))
    {
      if (type == COMP_REPAIRABLE)
        CarCompletion::pruneRepairables(pc);
      else
      {
        CarCompletion::addPeak(peak);
        distance += dist;
      }
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
  vector<Peak const *>*& closestPtr,
  unsigned& limitLowerOut,
  unsigned& limitUpperOut)
{
  closestPtr = &_closestPeaks;
  limitLowerOut = _limitLower;
  limitUpperOut = _limitUpper;
}


bool CarCompletion::condense(CarCompletion& miss2)
{
  const unsigned nc = _closestPeaks.size();
  if (miss2._closestPeaks.size() != nc)
    return false;

  for (unsigned i = 0; i < nc; i++)
  {
    if (_closestPeaks[i] != miss2._closestPeaks[i])
      return false;
  }

  weight += miss2.weight;
  if (miss2.distance < distance)
    distance = miss2.distance;

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
  ss << "Car with weight " << weight << ", distance " << distance;

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

  ss << "Peaks\n";
    for (auto& p: _closestPeaks)
      ss << p->strQuality(offset);
    ss << "\n";

  return ss.str();
}

