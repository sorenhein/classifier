#include <list>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "PeakPtrList.h"
#include "Except.h"


PeakPtrListNew::PeakPtrListNew()
{
  PeakPtrListNew::clear();
}


PeakPtrListNew::~PeakPtrListNew()
{
}


void PeakPtrListNew::clear()
{
  peaks.clear();
}


void PeakPtrListNew::push_back(Peak * peak)
{
  peaks.push_back(peak);
}


bool PeakPtrListNew::add(Peak * peak)
{
  // TODO Pretty inefficient if we are adding multiple candidates.
  // If we need to do that, better add them all at once.
  const unsigned pindex = peak->getIndex();
  for (auto it = peaks.begin(); it != peaks.end(); it++)
  {
    if ((* it)->getIndex() > pindex)
    {
      peaks.insert(it, peak);
      return true;
    }
  }
  return false;
}


void PeakPtrListNew::insert(
  PPLiterator& it,
  Peak * peak)
{
  peaks.insert(it, peak);
}


PPLiterator PeakPtrListNew::erase(PPLiterator& it)
{
  return peaks.erase(it);
}


void PeakPtrListNew::erase_below(const unsigned limit)
{
  for (PPLiterator it = peaks.begin(); it != peaks.end(); )
  {
    if ((* it)->getIndex() < limit)
      it = peaks.erase(it);
    else
      break;
  }
}


PPLiterator PeakPtrListNew::begin()
{
  return peaks.begin();
}


PPLiterator PeakPtrListNew::end()
{
  return peaks.end();
}


PPLciterator PeakPtrListNew::cbegin() const
{
  return peaks.cbegin();
}


PPLciterator PeakPtrListNew::cend() const
{
  return peaks.cend();
}


unsigned PeakPtrListNew::size() const
{
  return peaks.size();
}


unsigned PeakPtrListNew::count(const PeakFncPtr& fptr) const
{
  unsigned c = 0;
  for (auto peak: peaks)
  {
    if ((peak->* fptr)())
      c++;
  }
  return c;
}


bool PeakPtrListNew::empty() const
{
  return peaks.empty();
}


unsigned PeakPtrListNew::firstIndex() const
{
  if (peaks.empty())
    return numeric_limits<unsigned>::max();
  else
    return peaks.front()->getIndex();
}


unsigned PeakPtrListNew::lastIndex() const
{
  if (peaks.empty())
    return numeric_limits<unsigned>::max();
  else
    return peaks.back()->getIndex();
}


PPLiterator PeakPtrListNew::next(
  const PPLiterator& it,
  const PeakFncPtr& fptr,
  const bool exclFlag)
{
  if (it == peaks.end())
    return peaks.end();

  PPLiterator itNext = (exclFlag ? std::next(it) : it);
  while (itNext != peaks.end() && ! ((* itNext)->* fptr)())
    itNext++;

  return itNext;
}


PPLciterator PeakPtrListNew::next(
  const PPLciterator& it,
  const PeakFncPtr& fptr,
  const bool exclFlag) const
{
  if (it == peaks.end())
    return peaks.end();

  PPLciterator itNext = (exclFlag ? std::next(it) : it);
  while (itNext != peaks.end() && ! ((* itNext)->* fptr)())
    itNext++;

  return itNext;
}


PPLiterator PeakPtrListNew::prev(
  const PPLiterator& it,
  const PeakFncPtr& fptr,
  const bool exclFlag)
{
  // Return something that cannot be a useful iterator.
  if (it == peaks.begin())
    return peaks.end();

  PPLiterator itPrev = (exclFlag ? std::prev(it) : it);
  while (true)
  {
    if (((* itPrev)->* fptr)())
      return itPrev;
    else if (itPrev == peaks.begin())
      return peaks.end();
    else
      itPrev = prev(itPrev);
  }
}

PPLciterator PeakPtrListNew::prev(
  const PPLciterator& it,
  const PeakFncPtr& fptr,
  const bool exclFlag) const
{
  // Return something that cannot be a useful iterator.
  if (it == peaks.begin())
    return peaks.end();

  PPLciterator itPrev = (exclFlag ? std::prev(it) : it);
  while (true)
  {
    if (((* itPrev)->* fptr)())
      return itPrev;
    else if (itPrev == peaks.begin())
      return peaks.end();
    else
      itPrev = std::prev(itPrev);
  }
}


void PeakPtrListNew::fill(
  const unsigned start,
  const unsigned end,
  PeakPtrListNew& ppl,
  list<PPLciterator>& pil)
{
  // Pick out peaks in [start, end].
  ppl.clear();
  pil.clear();
  for (PPLiterator it = peaks.begin(); it != peaks.end(); it++)
  {
    Peak * peak = * it;
    const unsigned index = peak->getIndex();
    if (index > end)
      break;
    else if (index < start)
      continue;
    else
    {
      ppl.push_back(peak);
      pil.push_back(it);
    }
  }
}


void PeakPtrListNew::split(
  const PeakFncPtr& fptr1,
  const PeakFncPtr& fptr2,
  PeakPtrListNew& peaksMatched,
  PeakPtrListNew& peaksRejected) const
{
  // Pick out peaks satisfying fptr1 or fptr2.
  for (auto peak: peaks)
  {
    if ((peak->* fptr1)() || (peak->* fptr2)())
      peaksMatched.push_back(peak);
    else
      peaksRejected.push_back(peak);
  }
}


void PeakPtrListNew::flattenTODO(vector<Peak *>& flattened)
{
  flattened.clear();
  for (auto& peak: peaks)
    flattened.push_back(peak);
}


void PeakPtrListNew::apply(const PeakRunFncPtr& fptr)
{
  for (auto& peak: peaks)
    (peak->* fptr)();
}


string PeakPtrListNew::strQuality(
  const string& text,
  const unsigned offset,
  const PeakFncPtr& fptr) const
{
  if (peaks.empty())
    return "";

  stringstream ss;
  if (text != "")
    ss << text << "\n";
  ss << peaks.front()->strHeaderQuality();

  for (const auto& peak: peaks)
  {
    if ((peak->* fptr)())
      ss << peak->strQuality(offset);
  }
  ss << endl;
  return ss.str();
}

