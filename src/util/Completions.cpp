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


CarCompletion& Completions::emplace_back()
{
  CarCompletion& miss = completions.emplace_back(CarCompletion());
  return miss;
}


CarCompletion& Completions::back()
{
  return completions.back();
}


void Completions::markWith(
  Peak& peak,
  const CompletionType type)
{
  for (auto& comp: completions)
    comp.markWith(peak, type);
}


Citerator Completions::begin()
{
  return completions.begin();
}


Citerator Completions::end()
{
  return completions.end();
}


unsigned Completions::numComplete(CarCompletion *& complete)
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


void Completions::makeRepairables()
{
  for (auto& comp: completions)
    comp.makeRepairables();
}


bool Completions::nextRepairable(Peak& peak)
{
  for (auto& comp: completions)
  {
    if (comp.nextRepairable(peak))
      return true;
  }
  return false;
}


string Completions::str(const unsigned offset) const
{
  string s = "";
  for (auto& comp: completions)
    s += comp.str(offset);
  return s;
}

