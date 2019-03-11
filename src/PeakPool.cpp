#include <iostream>
#include <iomanip>
#include <sstream>

#include "PeakPool.h"


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


void PeakPool::mergeSplits(
  const unsigned wheelDist,
  const unsigned offset)
{
  // Look for peaks that are a lot closer than wheelDist.
  // These should probably have been merged.
  //
  // TODO
  // * Stand-alone stretches with enough distance on both sides
  //   -> A single, new peak.
  //      May have extent if the levels are similar.
  // * Stretch attached on one side to a selected peak
  //   -> Eliminate.
  //      May add to extent if the levels are similar.
  // * Stretch attached on both sides.
  //   -> For now this is flagged as an error.
  //      Don't connect two selected peaks in this way.

  const unsigned limit = wheelDist / 3;

  Piterator pStart = peaks->begin();
  Piterator pEnd = pStart;

  for (Piterator pit = peaks->begin(); pit != peaks->end(); pit++)
  {
    if (! pit->isCandidate())
      continue;
    else if (! pit->isSelected() &&
        pit->getIndex() - pStart->getIndex() <= limit)
    {
      // Extend the run.
      pEnd = pit;
    }
    else
    {
      // A selected peak or a jump ends all runs.
      if (pStart != pEnd)
      {
        // It was a real run.
        // TODO
        cout << "Likely split peaks\n";
        cout << pStart->strHeaderQuality();
        for (Piterator pp = pStart; pp != next(pEnd); pp++)
          cout << pp->strQuality(offset);
      }
      pStart = pit;
      pEnd = pit;
    }
  }

  if (pStart != pEnd)
  {
    // End run.
    // TODO
    cout << "Likely split peaks\n";
    cout << pStart->strHeaderQuality();
    for (Piterator pp = pStart; pp != next(pEnd); pp++)
      cout << pp->strQuality(offset);
  }
}


bool PeakPool::peakFixable(
  Piterator& foundIter,
  const PeakFncPtr& fptr,
  const Bracket& bracket,
  const bool nextSkipFlag,
  const string& text,
  const unsigned offset) const
{
  // bracket      max             max
  // foundIter           min
  // peak                *
  // peakNext                     *

  Peak peak = * foundIter;
  peak.update(&* bracket.left.pit);

  if (nextSkipFlag)
  {
    // There may be other peaks in between, so before we can use
    // the left flank to update peak, we must modify the next one.
    Peak peakNext = * bracket.right.pit;
    peakNext.update(& peak);

    peak.logNextPeak(& peakNext);
  }
  else
    peak.logNextPeak(&* bracket.right.pit);

  peak.calcQualities(averages);

  if ((peak.* fptr)())
    return true;
  else
  {
    cout << 
      foundIter->strQualityWhole(text + " not repairable", offset);
    return false;
  }
}


Peak * PeakPool::repairTopLevel(
  Piterator& foundIter,
  const PeakFncPtr& fptr,
  const unsigned offset)
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
      true, "Top-level peak", offset))
  {
    PeakPool::printRepairedSegment("Top-level bracket",
      bracketInnerMax, offset);
    return nullptr;
  }

  cout << foundIter->strQualityWhole("Top-level peak repairable", offset);

  Piterator nprevBestMax = next(bracketInnerMax.left.pit);
  if (nprevBestMax != foundIter)
  {
    // Delete peaks in [nprevBestMax, foundIter).
    // This also recalculates left flanks.
    peaks->collapse(nprevBestMax, foundIter);

    // The right flank of nprevBestMax must/may be updated.
    nprevBestMax->logNextPeak(&* foundIter);
  }


  Piterator nfoundIter = next(foundIter);
  if (bracketInnerMax.right.pit != nfoundIter)
  {
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
  const unsigned offset)
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
        false, "Lower-level peak", offset))
    {
      PeakPool::printRepairedSegment("Lower-level bracket", 
        bracketTmp, offset);
      return nullptr;
    }

    auto ptopNew = peaks->insert(bracketTop.right.pit, * foundLowerIter);
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
        false, "Lower-level peak", offset))
    {
      PeakPool::printRepairedSegment("Lower-level bracket", 
        bracketTmp, offset);
      return nullptr;
    }

    auto ptopNew = peaks->insert(bracketTop.right.pit, * foundLowerIter);
    peaks->insert(bracketTop.right.pit, * pmax);
  }

  PeakPool::updateRepairedPeaks(bracketTop);
  PeakPool::printRepairedSegment("Fixed top-level", bracketTop, offset);

  return &* foundLowerIter;
}


Peak * PeakPool::repair(
  const Peak& peakHint,
  const PeakFncPtr& fptr,
  const unsigned offset)
{
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

    // Is one of them close enough?  If both, pick the lowest value.
    Piterator foundIter;
    if (! liter->near(peakHint, bracket, foundIter))
      continue;

    if (liter == peakLists.rbegin())
    {
      // Peak exists, but may need to be sharpened by removing
      // surrounding peaks.
      return PeakPool::repairTopLevel(foundIter, fptr, offset);
    }
    else
    {
      // Peak only exists in earlier list and might be resurrected.
      return PeakPool::repairFromLower(* liter, foundIter, fptr, offset);
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

