#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

#include "PeakSeeds.h"


PeakSeeds::PeakSeeds()
{
  PeakSeeds::reset();
}


PeakSeeds::~PeakSeeds()
{
}


void PeakSeeds::reset()
{
  intervals.clear();
  nestedIntervals.clear();
}


void PeakSeeds::logInterval(
  Peak * peak1,
  Peak * peak2,
  const float lowerMinValue,
  const float value,
  const float scale)
{
  intervals.emplace_back(Interval());
  Interval& p = intervals.back();

  p.start = peak1->getIndex();
  p.end = peak2->getIndex();
  p.len = p.end - p.start;
  p.depth = (lowerMinValue - value) / scale;
  p.minLevel = lowerMinValue;

  p.leftPtr = peak1;
  p.rightPtr = peak2;
}


void PeakSeeds::makeIntervals(
  list<Peak>& peaks,
  const float scale)
{
  // Here we make a list of all intervals where there is no other
  // in-between peaks that exceeds the smallest of the edge peaks.

  for (auto peak = peaks.begin(); peak != peaks.end(); peak++)
  {
    // Get a negative minimum.
    const float value = peak->getValue();
    if (value >= 0. || peak->getMaxFlag())
      continue;

    // Find the previous negative minimum that was deeper.
    auto peakPrev = peak;
    bool posFlag = false;
    bool maxPrevFlag;
    float vPrev = 0.;
    float lowerMinValue = 0.f;
    do
    {
      if (peakPrev == peaks.begin())
        break;

      peakPrev = prev(peakPrev);
      maxPrevFlag = peakPrev->getMaxFlag();
      vPrev = peakPrev->getValue();

      if (! maxPrevFlag && vPrev > value)
        lowerMinValue = min(vPrev, lowerMinValue);

      if (vPrev >= 0.f)
        posFlag = true;
    }
    while (vPrev >= value || maxPrevFlag);

    if (posFlag && 
        lowerMinValue < 0.f &&
        peakPrev != peaks.begin() && 
        vPrev < value)
    {
      PeakSeeds::logInterval(&*peakPrev, &*peak, 
        lowerMinValue, vPrev, scale);
    }

    // Same thing forwards.
    auto peakNext = peak;
    posFlag = false;
    bool maxNextFlag;
    float vNext = 0.;
    lowerMinValue = 0.f;
    do
    {
      if (peakNext == peaks.end())
        break;

      peakNext = next(peakNext);
      maxNextFlag = peakNext->getMaxFlag();
      vNext = peakNext->getValue();

      if (! maxNextFlag && vNext > value)
        lowerMinValue = min(vNext, lowerMinValue);

      if (vNext >= 0.f)
        posFlag = true;
    }
    while (vNext >= value || maxNextFlag);

    if (posFlag && 
      lowerMinValue < 0.f &&
        peakNext != peaks.end() &&
        vNext < value)
    {
      PeakSeeds::logInterval(&*peak, &*peakNext, 
        lowerMinValue, vNext, scale);
    }
  }
}


void PeakSeeds::setupReferences(
  vector<IntervalAnnotated>& intByStart,
  vector<IntervalAnnotated>& intByEnd,
  vector<IntervalAnnotated>& intByLength)
{
  // intByLength contains intervals sorted by ascending length.
  for (auto& p: intervals)
  {
    intByLength.emplace_back(IntervalAnnotated());
    IntervalAnnotated& ia = intByLength.back();
    ia.intervalPtr = &p;
    ia.usedFlag = false;
  }

  sort(intByLength.begin(), intByLength.end(),
    [](const IntervalAnnotated& ia1, const IntervalAnnotated& ia2) -> bool
    { 
      return ia1.intervalPtr->len < ia2.intervalPtr->len; 
    });

  // intByStart and intByLength contain intervals sorted by their
  // parameter in ascending order, with an index into intByLength.
  unsigned i = 0;
  for (auto& ibl: intByLength)
  {
    intByStart.emplace_back(ibl);
    IntervalAnnotated& is = intByStart.back();
    is.indexLen = i;

    intByEnd.emplace_back(ibl);
    IntervalAnnotated& ie = intByEnd.back();
    ie.indexLen = i;

    i++;
  }

  sort(intByStart.begin(), intByStart.end(),
    [](const IntervalAnnotated& ia1, const IntervalAnnotated& ia2) -> bool
    { 
      if (ia1.intervalPtr->start < ia2.intervalPtr->start)
        return true;
      else if (ia1.intervalPtr->start > ia2.intervalPtr->start)
        return false;
      else
        return ia1.intervalPtr->len < ia2.intervalPtr->len; 
    });

  sort(intByEnd.begin(), intByEnd.end(),
    [](const IntervalAnnotated& ia1, const IntervalAnnotated& ia2) -> bool
    { 
      if (ia1.intervalPtr->end < ia2.intervalPtr->end)
        return true;
      else if (ia1.intervalPtr->end > ia2.intervalPtr->end)
        return false;
      else
        return ia1.intervalPtr->len < ia2.intervalPtr->len; 
    });

  // We can also get from intByLength to the others.
  i = 0;
  for (auto& ibs: intByStart)
    intByLength[ibs.indexLen].indexStart = i++;
  
  i = 0;
  for (auto& ibe: intByEnd)
    intByLength[ibe.indexLen].indexEnd = i++;
}


void PeakSeeds::nestIntervals(
  vector<IntervalAnnotated>& intByStart,
  vector<IntervalAnnotated>& intByEnd,
  vector<IntervalAnnotated>& intByLength)
{
  // Move out from small intervals to larger ones containing it.
  const unsigned ilen = intByLength.size();
  for (auto& ibl: intByLength)
  {
    if (ibl.usedFlag)
      continue;

    nestedIntervals.emplace_back(list<Interval *>());
    list<Interval *>& li = nestedIntervals.back();

    IntervalAnnotated * iaptr = &ibl;
    while (true)
    {
      li.push_back(iaptr->intervalPtr);
      iaptr->usedFlag = true;

      // Look in starts.
      const unsigned snext = iaptr->indexStart+1;
      const unsigned enext = iaptr->indexEnd+1;

      if (snext < ilen && 
          intByStart[snext].intervalPtr->start == iaptr->intervalPtr->start)
      {
        // Same start, next up in length.
        iaptr = &intByLength[intByStart[snext].indexLen];
      }
      else if (enext < ilen && 
        intByEnd[enext].intervalPtr->end == iaptr->intervalPtr->end)
      {
        // Look in ends.
        iaptr = &intByLength[intByEnd[enext].indexLen];
      }
      else
        break;
    }
  }

  // Sort the interval lists in order of appearance.
  sort(nestedIntervals.begin(), nestedIntervals.end(),
    [](const list<Interval *>& li1, const list<Interval *>& li2) -> bool
    { 
      return li1.front()->start < li2.front()->start;
    });
}


void PeakSeeds::makeNestedIntervals()
{
  // Here we nest the intervals in a somewhat efficient way.

  vector<IntervalAnnotated> intByStart, intByEnd, intByLength;

  PeakSeeds::setupReferences(intByStart, intByEnd, intByLength);

  PeakSeeds::nestIntervals(intByStart, intByEnd, intByLength);
}


void PeakSeeds::mergeSimilarLists()
{
  // Merge lists that only differ in the first interval.
  for (unsigned i = 0; i+1 < nestedIntervals.size(); )
  {
    if (nestedIntervals[i].size() != nestedIntervals[i+1].size())
    {
      i++;
      continue;
    }

    if (nestedIntervals[i].size() <= 1)
    {
      i++;
      continue;
    }

    Interval * p1 = nestedIntervals[i].front();
    Interval * p2 = nestedIntervals[i+1].front();
    if (p1->end == p2->start)
      nestedIntervals.erase(nestedIntervals.begin() + i+1);
    else
      i++;
  }
}


void PeakSeeds::pruneListEnds()
{
  // Stop lists when they reach exact overlap.
  for (unsigned i = 0; i+1 < nestedIntervals.size(); i++)
  {
    auto& li1 = nestedIntervals[i];
    auto& li2 = nestedIntervals[i+1];

    // Count up in the ends of the first list.
    unsigned index1 = 0;
    list<Interval *>::iterator nit1;
    for (auto it = li1.begin(); it != prev(li1.end()); it++)
    {
      nit1 = next(it);
      if ((*nit1)->end > li2.front()->start)
      {
        index1 = (*it)->end;
        break;
      }
      else if (next(nit1) == li1.end())
      {
        index1 = (*nit1)->end;
        nit1++;
        break;
      }
    }

    bool foundFlag = false;
    unsigned index2 = 0;
    list<Interval *>::iterator it2;
   
    // Count down in the starts of the second list.
    for (it2 = li2.begin(); it2 != li2.end(); it2++)
    {
      if ((*it2)->start == index1)
      {
        foundFlag = true;
        index2 = (*it2)->start;
        while (it2 != li2.end() && (*it2)->start == index1)
          it2++;
        break;
      }
    }

    if (! foundFlag)
      continue;

    if (index1 != index2)
      continue;

    if (nit1 != li1.end())
      li1.erase(nit1, li1.end());
    li2.erase(it2, li2.end());
  }
}


void PeakSeeds::markSeeds(list<Peak>& peaks)
{
  unsigned nindex = 0;
  unsigned ni1 = nestedIntervals[nindex].back()->start;
  unsigned ni2 = nestedIntervals[nindex].back()->end;

  // Note which peaks are tall.

  for (auto pit = peaks.begin(); pit != peaks.end(); pit++)
  {
    // Only want the negative minima here.
    if (pit->getValue() >= 0.f || pit->getMaxFlag())
      continue;

    // Exclude tall peaks without a right neighbor.
    if (next(pit) == peaks.end())
      continue;

    const unsigned index = pit->getIndex();
    if (index == ni1 || index == ni2)
      pit->select();

    if (index >= ni2)
    {
      if (nindex+1 < nestedIntervals.size())
      {
        nindex++;
        ni1 = nestedIntervals[nindex].back()->start;
        ni2 = nestedIntervals[nindex].back()->end;
      }
    }
  }
}


void PeakSeeds::mark(
  list<Peak>& peaks,
  const unsigned offset,
  const float scale)
{
  PeakSeeds::reset();

  PeakSeeds::makeIntervals(peaks, scale);

  PeakSeeds::makeNestedIntervals();

  // TODO Input flag
  if (false)
    cout << PeakSeeds::str(offset, "before pruning");

  // It happens that two collections of intervals are identical after
  // the first interval, i.e. the first two intervals collectively
  // make up the length of the entire lists.  In this case we only
  // neede one of them.
  PeakSeeds::mergeSimilarLists();

  // Interval lists can reach very high lengths, but we want to stop
  // (effectively) when we start to reach into the neighboring lists
  // of intervals.
  PeakSeeds::pruneListEnds();

  // TODO Input flag
  if (false)
    cout << PeakSeeds::str(offset, "after pruning");

  PeakSeeds::markSeeds(peaks);
}


string PeakSeeds::str(
  const unsigned offset,
  const string& text) const
{
  stringstream ss;
  ss << "Nested intervals " << text << "\n";
  for (auto& li: nestedIntervals)
  {
    for (auto& p: li)
      ss << p->start + offset << "-" << p->end << " (" << p->len << ")\n";
    ss << "\n";
  }
  return ss.str() + "\n";
}

