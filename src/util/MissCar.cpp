#include <sstream>
#include <vector>
#include <sstream>

#include "MissCar.h"


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


void MissCar::markWith(
  Peak * peak,
  const MissType type)
{
  for (auto& miss: misses)
    miss.markWith(peak, type);
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


bool MissCar::condense(MissCar& miss2)
{
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

  weight += miss2.weight;
  return true;
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


void MissCar::repairables(list<MissPeak *>& repairList)
{
  for (auto& miss: misses)
  {
    if (miss.source() == MISS_UNMATCHED)
      repairList.push_back(&miss);
  }
}


string MissCar::str(const unsigned offset) const
{
  stringstream ss;
  ss << "Car with weight " << weight << ", distance " << distance;

  if (misses.empty())
  {
    ss << " is complete\n\n";
  }
  else
  {
    ss << "\n\n";
    ss << misses.front().strHeader();
    for (auto& miss: misses)
      ss << miss.str(offset);
    ss << "\n";
  }
  return ss.str();
}

