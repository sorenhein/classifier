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
  // If we need to do that, add them all at once.
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


/*
const Peak *& PeakPtrListNew::front() const
{
  return peaks.front();
}


const Peak *& PeakPtrListNew::back() const
{
  return peaks.back();
}
*/


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


unsigned PeakPtrListNew::size() const
{
  return peaks.size();
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


PPLiterator PeakPtrListNew::nextExcl(
  const PPLiterator& it,
  const PeakFncPtr& fptr,
  const bool softFlag)
{
  if (it == peaks.end())
  {
    if (softFlag)
      return peaks.end();
    else
      THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss later matching peak");
  }

  return PeakPtrListNew::nextIncl(next(it), fptr);
}

PPLciterator PeakPtrListNew::nextExcl(
  const PPLciterator& it,
  const PeakFncPtr& fptr,
  const bool softFlag) const
{
  if (it == peaks.end())
  {
    if (softFlag)
      return peaks.end();
    else
      THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss later matching peak");
  }

  return PeakPtrListNew::nextIncl(next(it), fptr);
}


PPLiterator PeakPtrListNew::nextIncl(
  const PPLiterator& it,
  const PeakFncPtr& fptr) const
{
  PPLiterator itNext = it;
  while (itNext != peaks.end() && ! ((* itNext)->* fptr)())
    itNext++;

  return itNext;
}

PPLciterator PeakPtrListNew::nextIncl(
  const PPLciterator& it,
  const PeakFncPtr& fptr) const
{
  PPLciterator itNext = it;
  while (itNext != peaks.end() && ! ((* itNext)->* fptr)())
    itNext++;

  return itNext;
}


PPLiterator PeakPtrListNew::prevExcl(
  const PPLiterator& it,
  const PeakFncPtr& fptr,
  const bool softFlag)
{
  if (it == peaks.begin())
  {
    if (softFlag)
      return peaks.begin();
    else
      THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss earlier matching peak");
  }

  return PeakPtrListNew::prevIncl(prev(it), fptr, softFlag);
}

PPLciterator PeakPtrListNew::prevExcl(
  const PPLciterator& it,
  const PeakFncPtr& fptr,
  const bool softFlag) const
{
  if (it == peaks.begin())
  {
    if (softFlag)
      return peaks.begin();
    else
      THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss earlier matching peak");
  }

  return PeakPtrListNew::prevIncl(prev(it), fptr, softFlag);
}


PPLiterator PeakPtrListNew::prevIncl(
  const PPLiterator& it,
  const PeakFncPtr& fptr,
  const bool softFlag) const
{
  for (PPLiterator itPrev = it; ; itPrev = prev(itPrev))
  {
    if (((* itPrev)->* fptr)())
      return itPrev;
    else if (itPrev == peaks.begin())
    {
      if (softFlag)
        return itPrev;
      else
        THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss earlier matching peak");
    }
  }
}

PPLciterator PeakPtrListNew::prevIncl(
  const PPLciterator& it,
  const PeakFncPtr& fptr,
  const bool softFlag) const
{
  for (PPLciterator itPrev = it; ; itPrev = prev(itPrev))
  {
    if (((* itPrev)->* fptr)())
      return itPrev;
    else if (itPrev == peaks.begin())
    {
      if (softFlag)
        return itPrev;
      else
        THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss earlier matching peak");
    }
  }
}


void PeakPtrListNew::extractPtrs(
  const unsigned start,
  const unsigned end,
  list<Peak *>& pplNew)
{
  pplNew.clear();
  for (auto& peak: peaks)
  {
    const unsigned index = peak->getIndex();
    if (index > end)
      break;
    else if (index < start)
      continue;
    else
      pplNew.push_back(peak);
  }
}


void PeakPtrListNew::extractPtrs(
  const unsigned start,
  const unsigned end,
  const PeakFncPtr& fptr,
  list<Peak *>& pplNew)
{
  pplNew.clear();
  for (auto& peak: peaks)
  {
    const unsigned index = peak->getIndex();
    if (index > end)
      break;
    else if (index < start)
      continue;
    else if ((peak->* fptr)())
      pplNew.push_back(peak);
  }
}


void PeakPtrListNew::extractIters(
  const unsigned start,
  const unsigned end,
  list<PPLiterator>& pilNew)
{
  pilNew.clear();
  for (PPLiterator it = peaks.begin(); it != peaks.end(); it++)
  {
    Peak * peak = * it;
    const unsigned index = peak->getIndex();
    if (index > end)
      break;
    else if (index < start)
      continue;
    else
      pilNew.push_back(it);
  }
}


void PeakPtrListNew::extractIters(
  const unsigned start,
  const unsigned end,
  const PeakFncPtr& fptr,
  list<PPLiterator>& pilNew)
{
  pilNew.clear();
  for (PPLiterator it = peaks.begin(); it != peaks.end(); it++)
  {
    Peak * peak = * it;
    const unsigned index = peak->getIndex();
    if (index > end)
      break;
    else if (index < start)
      continue;
    else if ((peak->* fptr)())
      pilNew.push_back(it);
  }
}


void PeakPtrListNew::extractPair(
  const unsigned start,
  const unsigned end,
  vector<Peak *>& pplNew,
  list<PPLciterator>& pilNew)
{
  pplNew.clear();
  pilNew.clear();
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
      pplNew.push_back(peak);
      pilNew.push_back(it);
    }
  }
}


void PeakPtrListNew::extractPair(
  const unsigned start,
  const unsigned end,
  const PeakFncPtr& fptr,
  vector<Peak *>& pplNew,
  list<PPLciterator>& pilNew)
{
  pplNew.clear();
  pilNew.clear();
  for (PPLiterator it = peaks.begin(); it != peaks.end(); it++)
  {
    Peak * peak = * it;
    const unsigned index = peak->getIndex();
    if (index > end)
      break;
    else if (index < start)
      continue;
    else if ((peak->* fptr)())
    {
      pplNew.push_back(peak);
      pilNew.push_back(it);
    }
  }
}


string PeakPtrListNew::strQuality(
  const string& text,
  const unsigned offset) const
{
  if (peaks.empty())
    return "";

  stringstream ss;
  if (text != "")
    ss << text << "\n";
  ss << peaks.front()->strHeaderQuality();

  for (const auto& peak: peaks)
    ss << peak->strQuality(offset);
  ss << endl;
  return ss.str();
}


string PeakPtrListNew::strSelectedQuality(
  const string& text,
  const unsigned offset) const
{
  if (peaks.empty())
    return "";

  stringstream ss;
  if (text != "")
    ss << text << "\n";
  ss << peaks.front()->strHeaderQuality();

  for (const auto& peak: peaks)
  {
    if (peak->isSelected())
      ss << peak->strQuality(offset);
  }
  ss << endl;
  return ss.str();
}

