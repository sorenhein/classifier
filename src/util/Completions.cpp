#include "Completions.h"

#include "../Peak.h"


Completions::Completions()
{
  Completions::reset();
}


Completions::~Completions()
{
}


void Completions::reset()
{
  completions.clear();
}


MissCar& Completions::emplace_back()
{
  return completions.emplace_back(MissCar());
}


#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

void Completions::markWith(
  Peak const * peak,
  const MissType type)
{
  UNUSED(peak);
  UNUSED(type);
}


unsigned Completions::numComplete() const
{
  unsigned n = 0;
  for (auto& comp: completions)
  {
    if (comp.complete())
      n++;
  }
  return n;
}


void Completions::condense()
{
}


void Completions::repairables(list<list<MissPeak>>& repairList)
{
  UNUSED(repairList);
}


string Completions::str() const
{
  string s = "";
  for (auto& comp: completions)
    s += comp.str();
  return s;
}

