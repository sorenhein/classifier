#include <iostream>
#include <iomanip>
#include <sstream>

#include "PeakPool.h"
#include "Except.h"


#define SAMPLE_RATE 2000.


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


bool PeakPool::pruneTransients(
  const unsigned firstGoodIndex)
{
  if (transientTrimmedFlag)
    return false;
  else
    transientTrimmedFlag = true;

cout << "firstGoodIndex " << firstGoodIndex << endl;
  // Determine statistics.
  float levelSum = 0., levelSumSq = 0.;
  unsigned num = 0;
  for (auto& peak: * peaks)
  {
    if (peak.isSelected() && peak.getIndex() >= firstGoodIndex)
    {
      const float level = peak.getValue();
      levelSum += level;
      levelSumSq += level * level;
      num++;
    }
  }

  if (num <= 1)
    return false;

  const float mean = levelSum / num;
  const float sdev = sqrt(levelSumSq / (num-1) - mean * mean);
  const float limit = mean - 2.f * sdev;
// cout << "mean " << mean << " sdev " << sdev << " limit " << limit << endl;

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


void PeakPool::printRepairData(
  const Piterator& foundIter,
  const Piterator& pprev,
  const Piterator& pnext,
  const Piterator& pfirstPrev,
  const Piterator& pfirstNext,
  const Piterator& pcurrPrev,
  const Piterator& pcurrNext,
  const unsigned offset) const
{
  cout << foundIter->strHeaderQuality();
  cout << foundIter->strQuality(offset);

  cout << "It is between\n";
  cout << pprev->strQuality(offset);
  cout << pnext->strQuality(offset);

  cout << "At top level it is between\n";
  cout << pfirstPrev->strQuality(offset);
  cout << pfirstNext->strQuality(offset);

  cout << "At current level it is between\n";
  cout << pcurrPrev->strQuality(offset);
  cout << pcurrNext->strQuality(offset);
}


void PeakPool::printRepairedSegment(
  const Piterator& pfirstPrev,
  const Piterator& pfirstNext,
  const unsigned offset) const
{
  cout << "Modified the top-level peaks\n";
  cout << pfirstNext->strHeaderQuality();
  for (auto pit = pfirstPrev; pit != pfirstNext; pit++)
    cout << pit->strQuality(offset);
  cout << pfirstNext->strQuality(offset);
}


void PeakPool::updateRepairedPeaks(
  Piterator& pfirstPrev,
  Piterator& pfirstNext)
{
  // The left flanks must be updated.
  for (auto pit = next(pfirstPrev); pit != next(pfirstNext); pit++)
    pit->update(&* prev(pit));

  if (pfirstNext->isMinimum())
    pfirstNext->calcQualities(averages);

  // The right flanks must be copied.
  for (auto pit = pfirstPrev; pit != pfirstNext; pit++)
  {
    pit->logNextPeak(&* next(pit));
    if (pit->isMinimum())
    {
      pit->calcQualities(averages);

      if (pit != pfirstPrev && ! _candidates.add(&* pit))
        cout << "PINSERT: Couldn't add candidate\n";
    }
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
  // - pprevSelected (min),
  // - pprevBestMax (max),
  // - foundIter (min),
  // - pnextBestMax (max),
  // - pnextSelected (min).

  Bracket bracketOuterMin, bracketInnerMax;
  if (! peaks->brackets(foundIter, bracketOuterMin, bracketInnerMax))
    return nullptr;

  // We make a trial run to ensure the quality if we clean up.
  Peak ptmp = * foundIter;
  ptmp.update(&* bracketInnerMax.left.pit);
  Peak ptmpNext = * bracketInnerMax.right.pit;
  ptmpNext.update(& ptmp);
  ptmp.logNextPeak(& ptmpNext);
  ptmp.calcQualities(averages);
  if (! (ptmp.* fptr)())
  {
    cout << "PINSERT: Cleaned top-level peak would not be OK(3)\n";
    cout << ptmp.strHeaderQuality();
    cout << ptmp.strQuality(offset);
    cout << "Used peaks:\n";
    cout << bracketOuterMin.left.pit->strQuality(offset);
    cout << bracketInnerMax.left.pit->strQuality(offset);
    cout << bracketInnerMax.right.pit->strQuality(offset);
    cout << bracketOuterMin.right.pit->strQuality(offset);
    return nullptr;
  }

  cout << "PINSERT: Top peak exists\n";
  cout << foundIter->strHeaderQuality();
  cout << foundIter->strQuality(offset);

  Piterator nprevBestMax = next(bracketInnerMax.left.pit);
  if (nprevBestMax != foundIter)
  {
    // Delete peaks in (pprevBestMax, foundIter).

    cout << "PINSERT: Delete (" <<
      bracketInnerMax.left.pit->getIndex() + offset << ", " <<
      foundIter->getIndex() + offset << ")\n";

    // This also recalculates left flanks.
    peaks->collapse(nprevBestMax, foundIter);

    // The right flank of nprevBestMax must/may be updated.
    nprevBestMax->logNextPeak(&* foundIter);
  }


  Piterator nfoundIter = next(foundIter);
  if (bracketInnerMax.right.pit != nfoundIter)
  {
    // Delete peaks in (foundIter, pnextBestMax).

    cout << "PINSERT: Delete (" <<
      foundIter->getIndex() + offset << ", " <<
      bracketInnerMax.right.pit->getIndex() + offset << ")\n";
    
    peaks->collapse(nfoundIter, bracketInnerMax.right.pit);

    // The right flank of foundIter must be updated.
    foundIter->logNextPeak(&* bracketInnerMax.right.pit);

  }

  // Re-score foundIter.
  foundIter->calcQualities(averages);
  if (((* foundIter).* fptr)())
    foundIter->select();

  cout << "peakHint now\n";
  cout << foundIter->strQuality(offset);

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

  // Peak only exists in lower list and might be resurrected.
  // It would go between ptopPrev and ptopNext (one min, one max).
  Bracket bracketTop;
  peakLists.rbegin()->bracket(foundLowerIter->getIndex(), false, bracketTop);

  if (! bracketTop.left.hasFlag || ! bracketTop.right.hasFlag)
  {
    cout << "PINSERT: Not an interior interval\n";
    return nullptr;
  }

  // Find the same bracketing peaks in the current, lower list.
  // Piterator plowerPrev, plowerNext;
  // PeakPool::locateTopBrackets(ptopPrev, ptopNext, foundLowerIter, 
    // plowerPrev, plowerNext);
  Bracket bracketLower;
  listLower.bracketSpecific(bracketTop, foundLowerIter, bracketLower);


  // We only introduce two new peaks, so the one we found goes next 
  // to the bracketing one with opposite polarity.  In the gap we put 
  // the intervening peak with maximum value.  This is quick and dirty.
  Peak * pmax;
  Bracket bracketTmp;

  if (bracketLower.left.pit->isMinimum())
  {
    bracketTmp.left.pit = next(bracketLower.left.pit);
    bracketTmp.right.pit = foundLowerIter;

    if (! listLower.getHighestMax(bracketTmp, pmax))
      return nullptr;

    // We make a trial run to ensure that the new minimum at the top
    // level will be of good quality.
    Peak ptmp = * foundLowerIter;
    ptmp.update(pmax);
    ptmp.logNextPeak(&* next(foundLowerIter));
    ptmp.calcQualities(averages);
    if (! (ptmp.* fptr)())
    {
      cout << "PINSERT: Inserted peak would not be OK(1)\n";
      cout << ptmp.strHeaderQuality();
      cout << ptmp.strQuality(offset);
      return nullptr;
    }

    // Order: 
    // - ptopPrev (min), 
    // - NEW: pmax (max),
    // - NEW: the peak pointed to by foundLowerIter (min),
    // - the old maximum lifted from the lower level, 
    // - ptopNext (min).
    auto ptopNew = peaks->insert(bracketTop.right.pit, * foundLowerIter);
    peaks->insert(ptopNew, * pmax);
  }
  else
  {
    // New peak goes next to ptopPrev.
    // if (! PeakPool::getHighestMax(next(foundLowerIter), plowerNext, pmax))
    bracketTmp.left.pit = next(foundLowerIter);
    bracketTmp.right.pit = bracketLower.right.pit;
    if (! listLower.getHighestMax(bracketTmp, pmax))
      return nullptr;

    // Again we make a trial run.
    Peak ptmp = * foundLowerIter;
    ptmp.update(&* prev(foundLowerIter));
    Peak ptmpNext = * pmax;
    ptmpNext.update(& ptmp);
    ptmp.logNextPeak(& ptmpNext);
    ptmp.calcQualities(averages);
    if (! (ptmp.* fptr)())
    {
      cout << "PINSERT: Inserted peak would not be OK(2)\n";
      cout << ptmp.strHeaderQuality();
      cout << ptmp.strQuality(offset);
      return nullptr;
    }

    // Order:
    // ptopPrev (min),
    // - the old maximum lifted from the lower level, 
    // - NEW: the peak pointed to by foundLowerIter (min),
    // - NEW: pmax (max),
    // - ptopNext (min).
    auto ptopNew = peaks->insert(bracketTop.right.pit, * foundLowerIter);
    peaks->insert(bracketTop.right.pit, * pmax);
  }

  PeakPool::updateRepairedPeaks(bracketTop.left.pit, bracketTop.right.pit);

  PeakPool::printRepairedSegment(bracketTop.left.pit, bracketTop.right.pit, offset);

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


unsigned PeakPool::candsize() const 
{
  return _candidates.size();
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
  PeakPtrs& closestPeaks,
  PeakPtrs& skippedPeaks) const
{
  if (numWheels < 2)
    return false;

  // Do the three remaining wheels.
  PPLciterator cit0 = _candidates.next(cit, fptr);
  if (cit0 == _candidates.cend())
    return false;

  skippedPeaks.clear();
  closestPeaks.clear();
  closestPeaks.push_back(* cit);
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
        if (closestPeaks.size() != numWheels-1)
          return false;

        closestPeaks.push_back(* cit0);
        return true;
      }

      const unsigned pval0 = (* cit0)->getIndex();
      const unsigned pval1 = (* cit1)->getIndex();
      const unsigned mid = (pval0 + pval1) / 2;
      if (ptarget <= mid)
      {
        closestPeaks.push_back(* cit0);
        cit0 = cit1;
        pointIt++;
        break;
      }
      else if (ptarget <= pval1)
      {
        skippedPeaks.push_back(* cit0);
        closestPeaks.push_back(* cit1);
        cit1 = _candidates.next(cit1, fptr);
        if (cit1 == _candidates.cend() && closestPeaks.size() != numWheels)
          return false;

        cit0 = cit1;
        pointIt++;
        break;
      }
      else
      {
        skippedPeaks.push_back(* cit0);
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

