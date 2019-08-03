#include <iostream>
#include <iomanip>
#include <sstream>

#include "PeakPool.h"
#include "Except.h"


PeakPool::PeakPool()
{
  PeakPool::clear();
}


PeakPool::~PeakPool()
{
}


void PeakPool::clear()
{
  peakLists.clear();
  peakLists.resize(1);

  peaks = &peakLists.front();

  _candidates.clear();

  averages.clear();

  transientTrimmedFlag = false;
  transientLimit = 0;
}


void PeakPool::copy()
{
  peakLists.emplace_back(Peaks());
  peakLists.back() = * peaks;
  peaks = &peakLists.back();
}


void PeakPool::logAverages(const vector<Peak>& averagesIn)
{
  averages = averagesIn;
}


bool PeakPool::pruneTransients(const unsigned firstGoodIndex)
{
  if (transientTrimmedFlag)
    return false;
  else
    transientTrimmedFlag = true;

  float mean, sdev;
  if (! peaks->stats(firstGoodIndex, mean, sdev))
    return false;

  const float limit = mean - 2.f * sdev;

  // Go for the first consecutive peaks that are too negative.
  Piterator pitLastTransient;
  bool seenFlag = false;

  for (Piterator pit = peaks->begin(); pit != peaks->end(); pit++)
  {
    Peak& peak = * pit;
    if (peak.getIndex() >= firstGoodIndex)
      break;

    if (! peak.isCandidate() || peak.getIndex() == 0)
      continue;

    if (peak.getValue() >= limit)
      break;

    pitLastTransient = pit;
    seenFlag = true;
  }

  if (! seenFlag) 
    return false;

  // Remove the transient peaks.
  peaks->erase(peaks->begin(), ++pitLastTransient);

  // Also remove any candidates.
  transientLimit = pitLastTransient->getIndex();
  _candidates.erase_below(transientLimit);
  return true;
}


void PeakPool::printRepairedSegment(
  const string& text,
  const Bracket& bracketTop,
  const unsigned offset) const
{
  cout << text << "\n";
  cout << bracketTop.left.pit->strHeaderQuality();
  for (auto pit = bracketTop.left.pit; pit != bracketTop.right.pit; pit++)
    cout << pit->strQuality(offset);
  cout << bracketTop.right.pit->strQuality(offset);
}


void PeakPool::updateRepairedPeaks(Bracket& bracket)
{
  // The left flanks must be updated.
  for (auto pit = next(bracket.left.pit); 
      pit != next(bracket.right.pit); pit++)
    pit->update(&* prev(pit));

  if (bracket.right.pit->isMinimum())
    bracket.right.pit->calcQualities(averages);

  // The right flanks must be copied.
  for (auto pit = bracket.left.pit; pit != bracket.right.pit; pit++)
  {
    pit->logNextPeak(&* next(pit));
    if (pit->isMinimum())
    {
      pit->calcQualities(averages);

      if (pit != bracket.left.pit && ! _candidates.add(&* pit))
        cout << "PINSERT: Couldn't add candidate\n";
    }
  }
}


void PeakPool::showSplit(
  Piterator& pStart,
  Piterator& pEnd,
  const unsigned offset,
  const string& text) const
{
  // Skip the first "peak" for now.
  if (pStart == peaks->begin())
    return;

  cout << "Split? " << text << "\n";
  cout << pStart->strHeaderQuality();
  for (Piterator pp = pStart; pp != peaks->end() && pp != next(pEnd); pp++)
    cout << pp->strQuality(offset);
}


void PeakPool::removeCandidates(
  const Piterator& pStart,
  const Piterator& pEnd)
{
  // Remove [pStart, pEnd).  Inefficient.  Could note for each 
  // peak its candidate if applicable.
  
  PPLiterator pit = _candidates.begin();

  while (pit != _candidates.end() &&
      (* pit)->getIndex() < pStart->getIndex())
    pit++;

  if (pit == _candidates.end())
    return;

  
  while (pit != _candidates.end() &&
      (* pit)->getIndex() < pEnd->getIndex())
    pit = _candidates.erase(pit);
}


void PeakPool::collapseAndRemove(
  const Piterator& pStart,
  const Piterator& pEnd)
{
  PeakPool::removeCandidates(pStart, pEnd);
  peaks->collapse(pStart, pEnd);
}


void PeakPool::collapseRange(
  const Piterator& pmin,
  const Piterator& pStart,
  const Piterator& pEnd)
{
  if (pmin == pStart)
  {
    if (next(pEnd) == peaks->end())
      PeakPool::collapseAndRemove(next(pStart), pEnd);
    else
      PeakPool::collapseAndRemove(next(pStart), next(pEnd));
  }
  else if (pmin == pEnd)
    PeakPool::collapseAndRemove(pStart, pEnd);
  else
  {
    PeakPool::collapseAndRemove(pStart, pmin);

    if (next(pEnd) == peaks->end())
      PeakPool::collapseAndRemove(next(pmin), pEnd);
    else
      PeakPool::collapseAndRemove(next(pmin), next(pEnd));
  }
}


void PeakPool::collapseRange(
  const Piterator& pStart,
  const Piterator& pEnd)
{
  // This collapse a range into a single peak in that range.
  // The lowest of the minimum peaks is used.
  // pStart and pEnd are minima.

  Piterator pmin = pEnd;
  for (Piterator pit = pStart; pit != pEnd; pit++)
  {
    if (pit->isCandidate() && pit->getValue() < pmin->getValue())
      pmin = pit;
  }

  PeakPool::collapseRange(pmin, pStart, pEnd);
}


void PeakPool::mergeSplits(
  const unsigned wheelDist,
  const unsigned offset)
{
  // Look for peaks that are a lot closer than wheelDist.
  // These should probably have been merged.

  const unsigned limit = wheelDist / 2;

  Piterator pStart = peaks->begin();
  Piterator pEnd = pStart;

// cout << "MSPL limit " << limit << endl;
  for (Piterator pit = pStart; pit != peaks->end(); pit++)
  {
    if (! pit->isCandidate())
      continue;

// cout << "pStart " << pStart->getIndex() + offset << ", pEnd " <<
  // pEnd->getIndex() + offset << ", " << " pit " << pit->getIndex() + offset << endl;
    if (pit->isSelected())
    {
      // The stretch might extend beyond pit, but let's take stock.
      if (pit->getIndex() - pStart->getIndex() <= limit)
      {
        if (pStart->isSelected())
          PeakPool::showSplit(pStart, pit, offset, 
            "MSPL Both select (OK1?)");
        else
        {
          PeakPool::showSplit(pStart, pit, offset, 
            "MSPL Ending on select (OK)");

          // Ending on selected peak. Doesn't help!
          // PeakPool::collapseRange(pit, pStart, pit);
        }
      }
      else if (pit->getIndex() - pEnd->getIndex() <= limit)
        PeakPool::showSplit(pStart, pit, offset, 
          "MSPL Ending on select (too long)");
      else if (pStart != pEnd)
      {
        if (pStart->isSelected())
        {
          PeakPool::showSplit(pStart, pit, offset, 
            "MSPL Both select (OK2?)");
        }
        else
        {
          // Ending before selected peak.
          PeakPool::collapseRange(pStart, pEnd);
        }
      }
      
      pStart = pit;
      pEnd = pit;
    }
    else if (pit->getIndex() - pStart->getIndex() > limit)
    {
      if (pStart != pEnd)
      {
        if (pStart->isSelected())
        {
          PeakPool::showSplit(pStart, pEnd, offset, 
            "MSPL Starting on select (OK)");

          // Keep pStart for sure.  Doesn't help!
          // PeakPool::collapseRange(pStart, pStart, pEnd);
        }
        else
        {
          // Starting and ending without selected peak.
          PeakPool::collapseRange(pStart, pEnd);
        }
      }

      pStart = pit;
      pEnd = pit;
    }
    else
    {
      // Otherwise continue the stretch.
      pEnd = pit;
    }
  }
}


bool PeakPool::peakFixable(
  Piterator& foundIter,
  const PeakFncPtr& fptr,
  const Bracket& bracket,
  const bool nextSkipFlag,
  const string& text,
  const unsigned offset,
  const bool forceFlag) const
{
  // bracket      max             max
  // foundIter           min
  // peak                *
  // peakNext                     *

  Peak peak = * foundIter;
  peak.update(&* bracket.left.pit);

  Peak peakNext;
  if (nextSkipFlag)
  {
    // There may be other peaks in between, so before we can use
    // the left flank to update peak, we must modify the next one.
    peakNext = * bracket.right.pit;
    peakNext.update(& peak);
  }
  else
    peak.logNextPeak(&* bracket.right.pit);

  peak.calcQualities(averages);

  if (forceFlag || (peak.* fptr)())
    return true;
  else
  {
    cout << peak.strQualityWhole(text + " not repairable", offset);
    return false;
  }
}


Peak * PeakPool::repairTopLevel(
  Piterator& foundIter,
  const PeakFncPtr& fptr,
  const unsigned offset,
  const bool testFlag,
  const bool forceFlag,
  unsigned& testIndex)
{
  // Peak exists but is not good enough, perhaps because of 
  // neighboring spurious peaks.  To salvage the peak we'll have
  // to remove kinks.

  // Find the bracketing, selected minima and the bracketing maxima
  // (inside those bracketing minima) that maximize slope * range,
  // i.e. range^2 / len, for the foundIter peak.  This is a measure
  // of a good, sharp peak.  Order, possibly with others in between:
  // bracketOuterMin       left                              right
  // bracketInnerMax                 left          right
  // foundIter                               x

  Bracket bracketOuterMin, bracketInnerMax;
  if (! peaks->brackets(foundIter, bracketOuterMin, bracketInnerMax))
    return nullptr;

  // We make a trial run to ensure the quality if we clean up.
  if (! PeakPool::peakFixable(foundIter, fptr, bracketInnerMax, 
      true, "Top-level peak", offset, forceFlag))
  {
    PeakPool::printRepairedSegment("Top-level bracket",
      bracketInnerMax, offset);
    return nullptr;
  }

  cout << foundIter->strQualityWhole("Top-level peak repairable", offset);

  if (testFlag)
  {
    testIndex = foundIter->getIndex();
    return nullptr;
  }

  Piterator nprevBestMax = next(bracketInnerMax.left.pit);
  if (nprevBestMax != foundIter)
  {
    // TODO Make a method out of this.
    for (auto pit = nprevBestMax; pit != foundIter; pit++)
    {
      if (pit->isCandidate())
        _candidates.remove(&* pit);
    }

    // Delete peaks in [nprevBestMax, foundIter).
    // This also recalculates left flanks.
    peaks->collapse(nprevBestMax, foundIter);

    // The right flank of nprevBestMax must/may be updated.
    nprevBestMax->logNextPeak(&* foundIter);
  }


  Piterator nfoundIter = next(foundIter);
  if (bracketInnerMax.right.pit != nfoundIter)
  {
    // TODO Make a method out of this.
    for (auto pit = nfoundIter; pit != bracketInnerMax.right.pit; pit++)
    {
      if (pit->isCandidate())
        _candidates.remove(&* pit);
    }

    // Delete peaks in [nfoundIter, pnextBestMax).

    peaks->collapse(nfoundIter, bracketInnerMax.right.pit);

    // The right flank of foundIter must be updated.
    foundIter->logNextPeak(&* bracketInnerMax.right.pit);

  }

  // Re-score foundIter.
  foundIter->calcQualities(averages);
  foundIter->select();

  cout << foundIter->strQualityWhole("Top-level peak now", offset);

  return &* foundIter;
}


Peak * PeakPool::repairFromLower(
  Peaks& listLower,
  Piterator& foundLowerIter,
  const PeakFncPtr& fptr,
  const unsigned offset,
  const bool testFlag,
  const bool forceFlag,
  unsigned& testIndex)
{
  // There is a lower list, in which foundLowerIter was found.
  // There is the top list, pointed to by peaks, which we may modify.

  // Peak only exists in listLower and might be resurrected.
  // It would go within bracketTop (one min, one max).

  Bracket bracketTop;
  peakLists.rbegin()->bracket(foundLowerIter->getIndex(), false, bracketTop);

  if (! bracketTop.left.hasFlag || ! bracketTop.right.hasFlag)
  {
    cout << "PINSERT: Not an interior interval\n";
    return nullptr;
  }

  // Find the same bracketing peaks in the current, lower list.
  Bracket bracketLower;
  listLower.bracketSpecific(bracketTop, foundLowerIter, bracketLower);

  // We only introduce two new peaks, so the one we found goes next 
  // to the bracketing one with opposite polarity.  In the gap we put 
  // the intervening peak with maximum value.  This is quick and dirty.
  Piterator pmax;
  Piterator ptopNew;
  Bracket bracketTmp;

  if (bracketLower.left.pit->isMinimum())
  {
    // Version 1
    // bracketLower      left min                           right max
    //
    // index of found                       . (not a top peak)
    //
    // bracketTop        left min                           right max
    // insert                     pmax     min

    bracketTmp.left.pit = next(bracketLower.left.pit);
    bracketTmp.right.pit = foundLowerIter;

    if (! listLower.getHighestMax(bracketTmp, pmax))
      return nullptr;

    // We make a trial run to ensure that the new minimum at the top
    // level will be of good quality.

    bracketTmp.left.pit = pmax;
    bracketTmp.right.pit = next(foundLowerIter);

    if (! PeakPool::peakFixable(foundLowerIter, fptr, bracketTmp, 
        false, "Lower-level peak", offset, forceFlag))
    {
      PeakPool::printRepairedSegment("Lower-level bracket", 
        bracketTmp, offset);
      return nullptr;
    }

    if (testFlag)
    {
      testIndex = foundLowerIter->getIndex();
      return nullptr;
    }

    ptopNew = peaks->insert(bracketTop.right.pit, * foundLowerIter);
    peaks->insert(ptopNew, * pmax);
  }
  else
  {
    // Version 2:
    // bracketLower      left max                           right min
    //
    // index of found                       . (not a top peak)
    //
    // bracketTop        left max                           right min
    // insert                              min     max

    bracketTmp.left.pit = next(foundLowerIter);
    bracketTmp.right.pit = bracketLower.right.pit;

    if (! listLower.getHighestMax(bracketTmp, pmax))
      return nullptr;

    // Again we make a trial run.

    bracketTmp.left.pit = prev(foundLowerIter);
    bracketTmp.right.pit = pmax;

    if (! PeakPool::peakFixable(foundLowerIter, fptr, bracketTmp, 
        false, "Lower-level peak", offset, forceFlag))
    {
      PeakPool::printRepairedSegment("Lower-level bracket", 
        bracketTmp, offset);
      return nullptr;
    }

    if (testFlag)
    {
      testIndex = foundLowerIter->getIndex();
      return nullptr;
    }

    ptopNew = peaks->insert(bracketTop.right.pit, * foundLowerIter);
    peaks->insert(bracketTop.right.pit, * pmax);
  }

  PeakPool::updateRepairedPeaks(bracketTop);

  PeakPool::printRepairedSegment("Fixed top-level", bracketTop, offset);

  return &* ptopNew;
}


Peak * PeakPool::repair(
  const Peak& peakHint,
  const PeakFncPtr& fptr,
  const unsigned offset,
  const bool testFlag,
  const bool forceFlag,
  unsigned& testIndex)
{
  testIndex = 0;
  const unsigned pindex = peakHint.getIndex();
  if (pindex < transientLimit)
    return nullptr;

  unsigned ldepth = 0;
  for (auto liter = peakLists.rbegin(); liter != peakLists.rend(); 
      liter++, ldepth++)
  {
    if (liter->size() < 2)
      continue;

    Bracket bracket;
    liter->bracket(pindex, true, bracket);

/*
cout << "ldepth " << ldepth << ", looking for " << 
  peakHint.getIndex() + offset << endl;
if (bracket.left.hasFlag)
  cout << "left " << bracket.left.pit->getIndex() + offset << endl;
if (bracket.right.hasFlag)
  cout << "right " << bracket.right.pit->getIndex() + offset << endl;
*/

    // We won't look deeper if we're at the edge of this level.
    if (! bracket.left.hasFlag || ! bracket.right.hasFlag)
    {
cout << "No complete bracket, giving up at level " << ldepth << endl;
      return nullptr;
    }

    // Is one of them close enough?  If both, pick the lowest value.
    Piterator foundIter;
    if (! liter->near(peakHint, bracket, foundIter))
      continue;

/*
    if (foundIter == liter->begin() ||
        next(foundIter) == liter->end())
    {
cout << "Found1, but giving up at level " << ldepth << endl;
      return false;
    }

    const unsigned fi = peakLists.rbegin()->front().getIndex();
    const unsigned bi = peakLists.rbegin()->back().getIndex();

    if (prev(foundIter)->getIndex() == fi ||
        next(foundIter)->getIndex() == bi)
    {
cout << "Found2, but giving up at level " << ldepth << endl;
      return false;
    }
*/


cout << "Found possible peak, ldepth " << ldepth << ", offset " << offset << endl;
cout << foundIter->strQuality(offset) << endl;

    if (liter == peakLists.rbegin())
    {
      // Peak exists, but may need to be sharpened by removing
      // surrounding peaks.
      return PeakPool::repairTopLevel(foundIter, fptr, offset, 
        testFlag, forceFlag, testIndex);
    }
    else
    {
      // Peak only exists in earlier list and might be resurrected.

      if (testFlag)
      {
        // Don't reconstruct the first or last minimum at top level.
        const unsigned findex = foundIter->getIndex();
        Peak& pfirst = * next(peakLists.rbegin()->begin());
        Peak& plast = * prev(prev(peakLists.rbegin()->end()));
        if (findex == pfirst.getIndex() ||
            findex == plast.getIndex())
        {
/*
cout << "findex " << findex << endl;
cout << "first " << pfirst.getIndex() << endl;
cout << "last " << plast.getIndex() << endl;
*/
          testIndex = 0;
          return nullptr;
        }
      }

      return PeakPool::repairFromLower(* liter, foundIter, fptr, 
        offset, testFlag, forceFlag, testIndex);
    }
  }

  return nullptr;
}


void PeakPool::makeCandidates()
{
  for (auto& peak: * peaks)
  {
    if (peak.isCandidate())
      _candidates.push_back(&peak);
  }
}


PeakPtrs& PeakPool::candidates()
{
  return _candidates;
}


const PeakPtrs& PeakPool::candidatesConst() const
{
  return _candidates;
}


Peaks& PeakPool::top()
{
  return * peaks;
}


const Peaks& PeakPool::topConst() const
{
  return * peaks;
}


bool PeakPool::getClosest(
  const list<unsigned>& carPoints,
  const PeakFncPtr& fptr,
  const PPLciterator& cit,
  const unsigned numWheels,
  PeakPtrs& peakPtrsUsed,
  PeakPtrs& peakPtrsUnused) const
{
  if (numWheels < 2)
    return false;

  // Do the three remaining wheels.
  PPLciterator cit0 = _candidates.next(cit, fptr);
  if (cit0 == _candidates.cend())
    return false;

  peakPtrsUnused.clear();
  peakPtrsUsed.clear();
  peakPtrsUsed.push_back(* cit);
  const unsigned pstart = (* cit)->getIndex();
  
  auto pointIt = carPoints.begin();
  pointIt++;
  // pit is assumed to line up with carPoints[1], the first wheel.
  const unsigned cstart = * pointIt;
  pointIt++;
  // Start from the second wheel.

  PPLciterator cit1;
  for (unsigned i = 0; i < numWheels-1; i++)
  {
    const unsigned ptarget = (* pointIt) + pstart - cstart;

    while (true)
    {
      cit1 = _candidates.next(cit0, fptr);
      if (cit1 == _candidates.cend())
      {
        if (peakPtrsUsed.size() != numWheels-1)
          return false;

        peakPtrsUsed.push_back(* cit0);
        return true;
      }

      const unsigned pval0 = (* cit0)->getIndex();
      const unsigned pval1 = (* cit1)->getIndex();
      const unsigned mid = (pval0 + pval1) / 2;
      if (ptarget <= mid)
      {
        peakPtrsUsed.push_back(* cit0);
        cit0 = cit1;
        pointIt++;
        break;
      }
      else if (ptarget <= pval1)
      {
        peakPtrsUnused.push_back(* cit0);
        peakPtrsUsed.push_back(* cit1);
        cit1 = _candidates.next(cit1, fptr);
        if (cit1 == _candidates.cend() && peakPtrsUsed.size() != numWheels)
          return false;

        cit0 = cit1;
        pointIt++;
        break;
      }
      else
      {
        peakPtrsUnused.push_back(* cit0);
        cit0 = cit1;
      }
    }
  }
  return true;
}


bool PeakPool::check() const
{
  bool flag = true;
  for (auto& pl: peakLists)
  {
    if (! pl.check())
      flag = false;
  }
  return flag;
}


void PeakPool::print(const unsigned offset) const
{
  unsigned level = 0;
  for (auto& pl: peakLists)
  {
    cout << "PeakPool print, level " << level << "\n";
    cout << pl.str("", offset);
    level++;
  }
}


string PeakPool::strCounts() const
{
  stringstream ss;
  ss << 
    setw(8) << left << "Level" << 
    setw(6) << right << "Count" << endl;

  unsigned i = 0;
  for (auto& pl: peakLists)
  {
    ss << setw(8) << left << i++ <<
      setw(6) << right << pl.size() << endl;
  }
  ss << endl;
  return ss.str();
}

