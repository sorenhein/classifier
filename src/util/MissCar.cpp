#include <sstream>
#include <vector>
#include <sstream>

#include "MissCar.h"

#include "../Peak.h"


MissCar::MissCar()
{
  MissCar::reset();
}


MissCar::~MissCar()
{
}


void MissCar::reset()
{
  weight = 1;
  distance = 0;
  _closestPeaks.clear();
  _limitLower = 0;
  _limitUpper = 0;
}


void MissCar::setWeight(const unsigned weightIn)
{
  weight = weightIn;
}


void MissCar::setDistance(const unsigned dist)
{
  distance = dist;
}


void MissCar::add(
  const unsigned target,
  const unsigned tolerance)
{
  misses.emplace_back(MissPeak());
  MissPeak& miss = misses.back();

  miss.set(target, tolerance);
}


Miterator MissCar::begin()
{
  return misses.begin();
}


Miterator MissCar::end()
{
  return misses.end();
}


void MissCar::addPeak(Peak& peak)
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


void MissCar::markWith(
  Peak& peak,
  const MissType type)
{
  unsigned dist;
  for (auto& miss: misses)
  {
    if (miss.markWith(peak, type, dist))
    {
      if (type == MISS_REPAIRABLE)
        MissCar::pruneRepairables(miss);
      else
      {
        MissCar::addPeak(peak);
        distance += dist;
      }
    }
  }
}


bool MissCar::complete() const
{
  for (auto& miss: misses)
  {
    if (miss.source() == MISS_UNMATCHED)
      return false;
  }
  return true;
}


void MissCar::setMatch(
  vector<Peak const *>& closestPeaksIn,
  const unsigned limitLowerIn,
  const unsigned limitUpperIn)
{
  for (auto ptr: closestPeaksIn)
  {
    if (ptr)
      _closestPeaks.push_back(ptr);
  }

  _limitLower = limitLowerIn;
  _limitUpper = limitUpperIn;
}


void MissCar::getMatch(
  vector<Peak const *>*& closestPtr,
  unsigned& limitLowerOut,
  unsigned& limitUpperOut)
{
  closestPtr = &_closestPeaks;
  limitLowerOut = _limitLower;
  limitUpperOut = _limitUpper;
}


bool MissCar::condense(MissCar& miss2)
{
  /*
  const unsigned nm = misses.size();
  if (miss2.misses.size() != nm)
    return false;

  Miterator mi1, mi2;
  for (mi1 = misses.begin(), mi2 = miss2.misses.begin();
      mi1 != misses.end() && mi2 != miss2.misses.end();
      mi1++, mi2++)
  {
    if (! mi1->consistentWith(* mi2))
      return false;
  }
  */

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


void MissCar::makeRepairables()
{
  repairables.clear();
  for (auto& miss: misses)
  {
    if (miss.source() == MISS_UNMATCHED)
      repairables.push_back(&miss);
  }
  itRep = repairables.begin();
}


bool MissCar::nextRepairable(Peak& peak)
{
  if (itRep == repairables.end())
    return false;

  (* itRep)->fill(peak);
  itRep++;
  return true;
}


void MissCar::pruneRepairables(MissPeak& miss)
{
  if (itRep == repairables.end() || ** itRep > miss)
    return; // No point
  else if (* itRep == &miss)
    itRep = repairables.erase(itRep);
  else
  {
    for (auto it = next(itRep); it != repairables.end(); it++)
    {
      if (* itRep == &miss)
      {
        repairables.erase(it);
        return;
      }
    }
  }
}


unsigned MissCar::score() const
{
  vector<unsigned> types(MISS_SIZE);
  for (auto& miss: misses)
    types[miss.source()]++;

  // TODO For example.
  return 
    weight +
    3 * types[MISS_UNUSED] +
    1 * types[MISS_REPAIRABLE];
}


string MissCar::str(const unsigned offset) const
{
  stringstream ss;
  ss << "Car with weight " << weight << ", distance " << distance;

  if (misses.empty())
    ss << " is complete\n\n";
  else
  {
    ss << "\n\n";
    ss << misses.front().strHeader();
    for (auto& miss: misses)
      ss << miss.str(offset);
    ss << "\n";
  }

  ss << "Peaks\n";
    for (auto& p: _closestPeaks)
      ss << p->strQuality(offset);
    ss << "\n";

  return ss.str();
}

