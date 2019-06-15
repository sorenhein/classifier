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


MissCar& Completions::emplace_back(const unsigned dist)
{
  MissCar& miss = completions.emplace_back(MissCar());
  miss.setDistance(dist);
  return miss;
}


MissCar& Completions::back()
{
  return completions.back();
}


void Completions::markWith(
  Peak * peak,
  const MissType type)
{
  for (auto& comp: completions)
    comp.markWith(peak, type);
}


unsigned Completions::numComplete(MissCar *& complete)
{
  unsigned n = 0;
  for (auto& comp: completions)
  {
    if (comp.complete())
    {
      n++;
      complete = &comp;
    }
  }
  return n;
}


void Completions::condense()
{
  for (auto ci1 = completions.begin(); ci1 != completions.end(); ci1++)
  {
    for (auto ci2 = next(ci1); ci2 != completions.end(); )
    {
      if (ci1->condense(* ci2))
        ci2 = completions.erase(ci2);
      else
        ci2++;
    }
  }
}


void Completions::sort()
{
  completions.sort([](const MissCar& miss1, const MissCar& miss2)
    {
      return (miss1.score() > miss2.score());
    });
}


void Completions::repairables(list<list<MissPeak *>>& repairList)
{
  Completions::sort();

  repairList.clear();
  for (auto& comp: completions)
  {
    list<MissPeak *>& missList = 
      repairList.emplace_back(list<MissPeak *>());
    comp.repairables(missList);
  }
}


string Completions::str(const unsigned offset) const
{
  string s = "";
  for (auto& comp: completions)
    s += comp.str(offset);
  return s;
}

