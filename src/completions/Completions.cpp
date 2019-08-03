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


Completion& Completions::emplace_back()
{
  completions.emplace_back(Completion());
  return completions.back();
}


Completion& Completions::back()
{
  return completions.back();
}


void Completions::pop_back()
{
  if (! completions.empty())
    completions.pop_back();
}


void Completions::markWith(
  Peak& peak,
  const CompletionType type,
  const bool forceFlag)
{
  for (auto& comp: completions)
    comp.markWith(peak, type, forceFlag);
}


Citerator Completions::begin()
{
  return completions.begin();
}


Citerator Completions::end()
{
  return completions.end();
}


bool Completions::empty() const
{
  return completions.empty();
}


unsigned Completions::numComplete(Completion *& complete)
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


unsigned Completions::numPartial(
  Completion *& partial,
  BordersType& borders)
{
  unsigned n = 0;
  for (auto& comp: completions)
  {
    if (comp.partial())
    {
      n++;
      partial = &comp;
      borders = comp.bestBorders();
    }
  }
  return n;
}


bool Completions::effectivelyEmpty() const
{
  for (auto& comp: completions)
  {
    if (comp.filled() != 0)
     return false;
  }
  return true;
}


void Completions::makeShift()
{
  for (auto& comp: completions)
    comp.makeShift();
}


void Completions::calcMetrics()
{
  for (auto& comp: completions)
    comp.calcMetrics();
}


void Completions::condense()
{
  for (auto ci1 = completions.begin(); ci1 != completions.end(); )
  {
    bool c1flag = false;
    for (auto ci2 = next(ci1); ci2 != completions.end(); )
    {
      const CondenseType ct = ci1->condense(* ci2);

      if (ct == CONDENSE_SAME)
        ci2 = completions.erase(ci2);
      else if (ct == CONDENSE_BETTER)
        ci2 = completions.erase(ci2);
      else if (ct == CONDENSE_WORSE)
      {
        c1flag = true;
        break;
      }
      else
        ci2++;
    }

    if (c1flag)
      ci1 = completions.erase(ci1);
    else
      ci1++;
  }
}


void Completions::sort()
{
  // Sort in descending order of peaks.
  completions.sort();

  // Sort each completion by the model of its origin.
  for (auto& comp: completions)
    comp.sort();
}


void Completions::makeRepairables()
{
  for (auto& comp: completions)
    comp.makeRepairables();
}


bool Completions::nextRepairable(
  Peak& peak,
  bool& forceFlag)
{
  for (auto& comp: completions)
  {
    if (comp.nextRepairable(peak, forceFlag))
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

