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


bool PeakPool::empty() const
{
  return peaks->empty();
}


unsigned PeakPool::size() const 
{
  return peaks->size();
}


Piterator PeakPool::begin() const
{
  return peaks->begin();
}


Pciterator PeakPool::cbegin() const
{
  return peaks->cbegin();
}


Piterator PeakPool::end() const
{
  return peaks->end();
}


Pciterator PeakPool::cend() const
{
  return peaks->cend();
}


Peak& PeakPool::front()
{
  return peaks->front();
}


Peak& PeakPool::back()
{
  return peaks->back();
}


void PeakPool::extend()
{
  peaks->emplace_back(Peak());
}


void PeakPool::copy()
{
  peakLists.emplace_back(PeakList());
  peakLists.back() = * peaks;
  peaks = &peakLists.back();
}


void PeakPool::logAverages(const vector<Peak>& averagesIn)
{
  averages = averagesIn;
}


Piterator PeakPool::erase(
  Piterator pit1,
  Piterator pit2)
{
  return peaks->erase(pit1, pit2);
}


Piterator PeakPool::collapse(
  Piterator pit1,
  Piterator pit2)
{
  // Analogous to erase(): pit1 does not survive, while pit2 does.
  // But pit2 gets updated first.
  if (pit1 == pit2)
    return pit1;

  Peak * peak0 =
    (pit1 == peaks->begin() ? nullptr : &*prev(pit1));

  if (peak0 != nullptr)
    pit2->update(peak0);

  return peaks->erase(pit1, pit2);
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


void PeakPool::getBracketingPeaks(
  const list<PeakList>::reverse_iterator& liter,
  const unsigned pindex,
  const bool minFlag,
  PiterPair& pprev,
  PiterPair& pnext) const
{
  // Find bracketing minima.
  pprev.hasFlag = false;
  pnext.hasFlag = false;
  for (auto piter = liter->begin(); piter != liter->end(); piter++)
  {
    if (minFlag && ! piter->isMinimum())
      continue;

    if (piter->getIndex() > pindex)
    {
      pnext.pit = piter;
      pnext.hasFlag = true;
      break;
    }

    pprev.pit = piter;
    pprev.hasFlag = true;
  }
}


bool PeakPool::findCloseIter(
  const Peak& peakHint,
  const PiterPair& pprev,
  const PiterPair& pnext,
  Piterator& foundIter) const
{
  bool fitsNext = false;
  bool fitsPrev = false;
  if (pprev.hasFlag && peakHint.fits(* pprev.pit))
    fitsPrev = true;
  if (pnext.hasFlag && peakHint.fits(* pnext.pit))
    fitsNext = true;

  if (fitsPrev && fitsNext)
  {
    if (pprev.pit->getValue() <= pnext.pit->getValue())
      foundIter = pprev.pit;
    else
      foundIter = pnext.pit;
  }
  else if (fitsPrev)
    foundIter = pprev.pit;
  else if (fitsNext)
    foundIter = pnext.pit;
  else
    return false;

  return true;
}


void PeakPool::locateTopBrackets(
  const PiterPair& pfirstPrev,
  const PiterPair& pfirstNext,
  const Piterator& foundIter,
  Piterator& pprev,
  Piterator& pnext) const
{
  const unsigned ppindex = pfirstPrev.pit->getIndex();
  const unsigned pnindex = pfirstNext.pit->getIndex();

  pprev = foundIter;
  do
  {
    pprev = prev(pprev);
  }
  while (pprev->getIndex() != ppindex);

  pnext = foundIter;
  do
  {
    pnext = next(pnext);
  }
  while (pnext->getIndex() != pnindex);
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

      // if (pit != pfirstPrev && ! PeakPool::addCandidate(&* pit))
      if (pit != pfirstPrev && ! _candidates.add(&* pit))
        cout << "PINSERT: Couldn't add candidate\n";
    }
  }
}


bool PeakPool::getHighestMax(
  const Piterator& pb,
  const Piterator& pe,
  Peak *& pmax) const
{
  pmax = nullptr;
  float val = numeric_limits<float>::lowest();
  for (auto pit = pb; pit != pe; pit++)
  {
    if (pit->getValue() > val)
    {
      val = pit->getValue();
      pmax = &* pit;
    }
  }

  if (pmax == nullptr)
  {
    cout << "PINSERT: Null maximum\n";
    return false;
  }
  else
    return true;
}


bool PeakPool::getBestMax(
  Piterator& pbmin,
  Piterator& pemin,
  Piterator& pref,
  Piterator& pbest) const
{
  pbest = peaks->begin();
  float valMax = 0.f;

  for (auto pit = pbmin; pit != pemin; pit++)
  {
    if (pit->isMinimum())
      continue;

    const float val = pref->matchMeasure(* pit);
    if (val > valMax)
    {
      pbest = pit;
      valMax = val;
    }
  }
  return (pbest != peaks->begin());
}


bool PeakPool::findTopSurrounding(
  Piterator& foundIter,
  Piterator& pprevSelected,
  Piterator& pnextSelected,
  Piterator& pprevBestMax,
  Piterator& pnextBestMax) const
{
  // Find the previous selected minimum.  
  pprevSelected = PeakPool::prevExclSoft(foundIter, &Peak::isSelected);

  if (pprevSelected == peaks->begin())
  {
    cout << "PINSERT: Predecessor is begin()\n";
    return false;
  }

  // Find the in-between maximum that maximizes slope * range,
  // i.e. range^2 / len.
  if (! PeakPool::getBestMax(pprevSelected, foundIter, foundIter,
    pprevBestMax))
  {
    cout << "PINSERT: Maximum is begin()\n";
    return false;
  }

  pnextSelected = PeakPool::nextExcl(foundIter, &Peak::isSelected);

  if (pnextSelected == peaks->begin())
  {
    cout << "PINSERT: Successor is end()\n";
    return false;
  }

  if (! PeakPool::getBestMax(foundIter, pnextSelected, foundIter,
    pnextBestMax))
  {
    cout << "PINSERT: Maximum is end()\n";
    return false;
  }

  return true;
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
  Piterator pprevSelected, pnextSelected;
  Piterator pprevBestMax, pnextBestMax;
  if (! PeakPool::findTopSurrounding(foundIter,
      pprevSelected, pnextSelected, pprevBestMax, pnextBestMax))
    return nullptr;

  // We make a trial run to ensure the quality if we clean up.
  Peak ptmp = * foundIter;
  ptmp.update(&* pprevBestMax);
  Peak ptmpNext = * pnextBestMax;
  ptmpNext.update(& ptmp);
  ptmp.logNextPeak(& ptmpNext);
  ptmp.calcQualities(averages);
  if (! (ptmp.* fptr)())
  {
    cout << "PINSERT: Cleaned top-level peak would not be OK(3)\n";
    cout << ptmp.strHeaderQuality();
    cout << ptmp.strQuality(offset);
    cout << "Used peaks:\n";
    cout << pprevSelected->strQuality(offset);
    cout << pprevBestMax->strQuality(offset);
    cout << pnextBestMax->strQuality(offset);
    cout << pnextSelected->strQuality(offset);
    return nullptr;
  }

  cout << "PINSERT: Top peak exists\n";
  cout << foundIter->strHeaderQuality();
  cout << foundIter->strQuality(offset);

  Piterator nprevBestMax = next(pprevBestMax);
  if (nprevBestMax != foundIter)
  {
    // Delete peaks in (pprevBestMax, foundIter).

    cout << "PINSERT: Delete (" <<
      pprevBestMax->getIndex() + offset << ", " <<
      foundIter->getIndex() + offset << ")\n";

    // This also recalculates left flanks.
    PeakPool::collapse(nprevBestMax, foundIter);

    // The right flank of nprevBestMax must/may be updated.
    nprevBestMax->logNextPeak(&* foundIter);
  }


  Piterator nfoundIter = next(foundIter);
  if (pnextBestMax != nfoundIter)
  {
    // Delete peaks in (foundIter, pnextBestMax).

    cout << "PINSERT: Delete (" <<
      foundIter->getIndex() + offset << ", " <<
      pnextBestMax->getIndex() + offset << ")\n";
    
    PeakPool::collapse(nfoundIter, pnextBestMax);

    // The right flank of foundIter must be updated.
    foundIter->logNextPeak(&* pnextBestMax);

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
  Piterator& foundLowerIter,
  const PeakFncPtr& fptr,
  const unsigned offset)
{
  // There is a lower list, in which foundLowerIter was found.
  // There is the top list, pointed to by peaks, which we may modify.

  // Peak only exists in lower list and might be resurrected.
  // It would go between ptopPrev and ptopNext (one min, one max).
  PiterPair ptopPrev, ptopNext;
  PeakPool::getBracketingPeaks(peakLists.rbegin(), 
    foundLowerIter->getIndex(), false, ptopPrev, ptopNext);

  if (! ptopPrev.hasFlag || ! ptopNext.hasFlag)
  {
    cout << "PINSERT: Not an interior interval\n";
    return nullptr;
  }

  // Find the same bracketing peaks in the current, lower list.
  Piterator plowerPrev, plowerNext;
  PeakPool::locateTopBrackets(ptopPrev, ptopNext, foundLowerIter, 
    plowerPrev, plowerNext);

  // We only introduce two new peaks, so the one we found goes next 
  // to the bracketing one with opposite polarity.  In the gap we put 
  // the intervening peak with maximum value.  This is quick and dirty.
  Peak * pmax;
  if (plowerPrev->isMinimum())
  {
    if (! PeakPool::getHighestMax(next(plowerPrev), foundLowerIter, pmax))
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
    auto ptopNew = peaks->insert(ptopNext.pit, * foundLowerIter);
    peaks->insert(ptopNew, * pmax);
  }
  else
  {
    // New peak goes next to ptopPrev.
    if (! PeakPool::getHighestMax(next(foundLowerIter), plowerNext, pmax))
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
    auto ptopNew = peaks->insert(ptopNext.pit, * foundLowerIter);
    peaks->insert(ptopNext.pit, * pmax);
  }

  PeakPool::updateRepairedPeaks(ptopPrev.pit, ptopNext.pit);

  PeakPool::printRepairedSegment(ptopPrev.pit, ptopNext.pit, offset);

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

    // Find bracketing minima.
    PiterPair pprev, pnext;
    PeakPool::getBracketingPeaks(liter, pindex, true, pprev, pnext);

    // Is one of them close enough?  If both, pick the lowest value.
    Piterator foundIter;
    if (! PeakPool::findCloseIter(peakHint, pprev, pnext, foundIter))
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
      return PeakPool::repairFromLower(foundIter, fptr, offset);
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


PeakPtrListNew& PeakPool::candidates()
{
  return _candidates;
}


const PeakPtrListNew& PeakPool::candidatesConst() const
{
  return _candidates;
}


Piterator PeakPool::prevExcl(
  Piterator& pit,
  const PeakFncPtr& fptr) const
{
  if (pit == peaks->begin())
    THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss earlier matching peak");

  Piterator ppit = prev(pit);
  return PeakPool::prevIncl(ppit, fptr);
}


Piterator PeakPool::prevExclSoft(
  Piterator& pit,
  const PeakFncPtr& fptr) const
{
  if (pit == peaks->begin())
    return peaks->begin();

  Piterator ppit = prev(pit);
  return PeakPool::prevInclSoft(ppit, fptr);
}


Piterator PeakPool::prevIncl(
  Piterator& pit,
  const PeakFncPtr& fptr) const
{
  for (Piterator pitPrev = pit; ; pitPrev = prev(pitPrev))
  {
    if (((* pitPrev).* fptr)())
      return pitPrev;
    else if (pitPrev == peaks->begin())
      THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss earlier matching peak");
  }
}


Piterator PeakPool::prevInclSoft(
  Piterator& pit,
  const PeakFncPtr& fptr) const
{
  for (Piterator pitPrev = pit; ; pitPrev = prev(pitPrev))
  {
    if (((* pitPrev).* fptr)())
      return pitPrev;
    else if (pitPrev == peaks->begin())
      return peaks->begin();
  }
}


Piterator PeakPool::nextExcl(
  Piterator& pit,
  const PeakFncPtr& fptr) const
{
  if (pit == peaks->end())
    THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss later matching peak");

  Piterator npit = next(pit);
  return PeakPool::nextIncl(npit, fptr);
}


Piterator PeakPool::nextIncl(
  Piterator& pit,
  const PeakFncPtr& fptr) const
{
  Piterator pitNext = pit;
  while (pitNext != peaks->end() && ! ((* pitNext).* fptr)())
    pitNext++;

  return pitNext;
}


bool PeakPool::getClosest(
  const list<unsigned>& carPoints,
  const PeakFncPtr& fptr,
  const PPLciterator& cit,
  const unsigned numWheels,
  PeakPtrVector& closestPeaks,
  PeakPtrVector& skippedPeaks) const
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


void PeakPool::getSelectedSamples(vector<float>& selected) const
{
  for (unsigned i = 0; i < selected.size(); i++)
    selected[i] = 0;

  for (auto& peak: * peaks)
  {
    if (peak.isSelected())
      selected[peak.getIndex()] = peak.getValue();
  }
}


float PeakPool::getFirstPeakTime() const
{
  for (auto& peak: * peaks)
  {
    if (peak.isSelected())
      return peak.getIndex() / static_cast<float>(SAMPLE_RATE);
  }
  return 0.f;
}


void PeakPool::getSelectedTimes(vector<PeakTime>& times) const
{
  times.clear();
  const float t0 = PeakPool::getFirstPeakTime();

  for (auto& peak: * peaks)
  {
    if (peak.isSelected())
    {
      times.emplace_back(PeakTime());
      PeakTime& p = times.back();
      p.time = peak.getIndex() / SAMPLE_RATE - t0;
      p.value = peak.getValue();
    }
  }
}

string PeakPool::strAll(
  const string& text,
  const unsigned& offset) const
{
  if (peaks->empty())
    return "";

  stringstream ss;
  if (text != "")
    ss << text << ": " << peaks->size() << "\n";
  ss << peaks->front().strHeader();

  for (auto& peak: * peaks)
    ss << peak.str(offset);
  ss << endl;
  return ss.str();
}


string PeakPool::strSelectedTimesCSV(const string& text) const
{
  if (peaks->empty())
    return "";

  stringstream ss;
  ss << text << "n";
  unsigned i = 0;
  for (auto& peak: * peaks)
  {
    if (peak.isSelected())
      ss << i++ << ";" <<
        fixed << setprecision(6) << peak.getTime() << "\n";
  }
  ss << endl;
  return ss.str();
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

