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
}


void MissCar::setWeight(const unsigned weightIn)
{
  weight = weightIn;
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


bool MissCar::complete() const
{
  for (auto& miss: misses)
  {
    if (miss.source() == MISS_UNMATCHED)
      return false;
  }
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
    3 * types[MISS_USED] +
    2 * types[MISS_UNUSED] +
    1 * types[MISS_REPAIRABLE];
}


string MissCar::str() const
{
  if (misses.empty())
    return "";

  stringstream ss;
  ss << "Car with Weight " << weight << "\n\n";
  ss << misses.front().strHeader();
  for (auto& miss: misses)
    ss << miss.str();
  ss << "\n";
  return ss.str();
}

