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
  PeakPtrListNew& pplNew)
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
  PeakPtrListNew& pplNew)
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
  PeakIterListNew& pilNew)
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
  PeakIterListNew& pilNew)
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

