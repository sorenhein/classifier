#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

#include "PeakSeeds.h"
#include "PeakPtrs.h"

#define PEAKSEEDS_ITER 4
#define PEAKSEEDS_VALUE 0.7f
#define PEAKSEEDS_ADD 0.1f


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
  PeakPool& peaks,
  const float scale)
{
  // Here we make a list of all intervals where there is no other
  // in-between peak that exceeds the smallest of the edge peaks.

  for (auto peak = peaks.top().begin(); peak != peaks.top().end(); peak++)
  {
    // Get a negative minimum.
    const float value = peak->getValue();
    if (! peak->isCandidate() || peak->isSelected())
      continue;

    // Find the previous negative minimum that was deeper.
    auto peakPrev = peak;
    bool posFlag = false;
    bool maxPrevFlag = true;
    float vPrev = 0.;
    float lowerMinValue = 0.f;
    do
    {
      if (peakPrev == peaks.top().begin())
        break;

      peakPrev = prev(peakPrev);
      if (peakPrev->isSelected())
        continue;

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
        peakPrev != peaks.top().begin() && 
        vPrev < value)
    {
      PeakSeeds::logInterval(&*peakPrev, &*peak, 
        lowerMinValue, vPrev, scale);
    }

    // Same thing forwards.
    auto peakNext = peak;
    posFlag = false;
    bool maxNextFlag = true;
    float vNext = 0.;
    lowerMinValue = 0.f;
    do
    {
      peakNext = next(peakNext);
      if (peakNext == peaks.top().end())
        break;

      if (peakNext->isSelected())
        continue;

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
        peakNext != peaks.top().end() &&
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
    list<Interval *>::iterator nit1 = li1.end();
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
    if (it2 != li2.end())
      li2.erase(it2, li2.end());
  }
}


void PeakSeeds::markSeeds(
  PeakPool& peaks,
  PeakPtrs& peaksToSelect,
  Peak& peakAvg) const
{
  unsigned nindex = 0;
  unsigned ni1 = nestedIntervals[nindex].back()->start;
  unsigned ni2 = nestedIntervals[nindex].back()->end;

  peaksToSelect.clear();
  peakAvg.reset();

  // Note which peaks are tall.

  for (auto pit = peaks.top().begin(); pit != peaks.top().end(); pit++)
  {
    // Only want the negative minima here.
    if (! pit->isCandidate() || pit->isSelected())
      continue;

    // Exclude tall peaks without a right neighbor.
    if (next(pit) == peaks.top().cend())
      continue;

    const unsigned index = pit->getIndex();
    if (index == ni1 || index == ni2)
    {
      peaksToSelect.push_back(&*pit);
      peakAvg += * pit;
    }

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

  if (peaksToSelect.size() > 0)
    peakAvg /= peaksToSelect.size();
}


void PeakSeeds::rebalanceCenters(
  PeakPool& peaks, 
  vector<Peak>& peakCenters) const
{
  // Rebalance the centers a bit.  We could run a more complete K-Means
  // here.

  const unsigned pl = peakCenters.size();
  vector<Peak> psum(pl);
  vector<unsigned> pcount(pl);

  for (auto pit = peaks.top().begin(); pit != peaks.top().end(); pit++)
  {
    if (! pit->isSelected())
      continue;

    const unsigned index = (* pit).calcQualityPeak(peakCenters);
    psum[index] += * pit;
    pcount[index]++;
  }

  peakCenters.clear();
  for (unsigned i = 0; i < pl; i++)
  {
    if (pcount[i] > 0)
    {
      psum[i] /= pcount[i];
      peakCenters.push_back(psum[i]);
    }
  }
}


void PeakSeeds::mark(
  PeakPool& peaks,
  vector<Peak>& peakCenters,
  const unsigned offset,
  const float scale)
{
  // Make up to PEAKSEEDS_ITER groups of peaks, each center to be
  // stored in peakCenters.
  // Stop early if the level of the center peak falls below 
  // PEAKSEEDS_VALUE times the first leval.
  // Also stop early if the iteration adds less than the fraction
  // PEAKSEEDS_ADD new peaks.

  PeakPtrs peaksToSelect;
  Peak peakAvg;
  unsigned peakCount = 0;
  unsigned numIter = 0;

  for (unsigned iter = 0; iter < PEAKSEEDS_ITER; iter++)
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

    PeakSeeds::markSeeds(peaks, peaksToSelect, peakAvg);
    const unsigned pl = peaksToSelect.size();

    if (true)
    {
      cout << "PeakSeeds iteration " << iter << 
        ": got " << pl << " new peaks" << endl;
      cout << peakAvg.strHeaderQuality();
      cout << peakAvg.strQuality(offset);
    }

    // Note that the value is negative.
    if (iter == 0 ||
        (peakAvg.getValue() <= 
           PEAKSEEDS_VALUE * peakCenters.front().getValue() &&
           pl >= PEAKSEEDS_ADD * peakCount))
    {
cout << "Adding to peaks\n";
      peakCenters.push_back(peakAvg);
      for (auto& pp: peaksToSelect)
        pp->select();
      peakCount += pl;
    }
    else
    {
cout << "Not adding to peaks\n";
      numIter = iter;
      break;
    }
  }

  if (numIter == 0)
    return;
  else
    PeakSeeds::rebalanceCenters(peaks, peakCenters);

  if (true)
  {
    cout << "PeakSeeds: Ended up with " << peakCenters.size() <<
      " peaks\n";
    cout << peakCenters.front().strHeaderQuality();
    for (auto& p: peakCenters)
      cout << p.strQuality(offset);
    cout << "\n";
  }
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
      ss << p->start + offset << "-" << p->end + offset << " (" << p->len << ")\n";
    ss << "\n";
  }
  return ss.str() + "\n";
}

