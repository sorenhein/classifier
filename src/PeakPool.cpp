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
  candidates.clear();
  averages.clear();
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


unsigned PeakPool::countInsertions(
  const Piterator& pprev,
  const Piterator& pnext) const
{
  unsigned n = 0;
  for (auto pit = next(pprev); pit != pnext; pit++)
    n++;
  return n;
}


bool PeakPool::addCandidate(Peak * peak)
{
  // TODO Pretty inefficient if we are adding multiple candidates.
  // If we need to do that, add them all at once.
  const unsigned pindex = peak->getIndex();
  for (auto cit = candidates.begin(); cit != candidates.end(); cit++)
  {
    if ((* cit)->getIndex() > pindex)
    {
      candidates.insert(cit, peak);
      return true;
    }
  }
  return false;
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

      if (pit != pfirstPrev && ! PeakPool::addCandidate(&* pit))
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


bool PeakPool::repairTopLevel(
  Piterator& foundIter,
  const unsigned offset)
{
  // Peak exists but is not good enough, perhaps because of 
  // neighboring spurious peaks.  To salvage the peak we'll have
  // to remove kinks.

  // Find the previous selected minimum.  
  Piterator pprevSelected = PeakPool::prevExcl(foundIter,
    &Peak::isSelected);

  if (pprevSelected == peaks->begin())
  {
    cout << "PINSERT: Predecessor is begin()\n";
    return false;
  }

  // Find the in-between maximum that maximizes slope * range,
  // i.e. range^2 / len.
  Piterator pprevBestMax;
  if (! PeakPool::getBestMax(pprevSelected, foundIter, foundIter,
    pprevBestMax))
  {
    cout << "PINSERT: Maximum is begin()\n";
    return false;
  }

  Piterator nprevBestMax = next(pprevBestMax);
  if (nprevBestMax != foundIter)
  {
    // Delete peaks in (pprevBestMax, foundIter).
    cout << "PINSERT: Peak exists\n";
    cout << foundIter->strHeaderQuality();
    cout << foundIter->strQuality(offset);

    cout << "PINSERT: Delete (" <<
      pprevBestMax->getIndex() + offset << ", " <<
      foundIter->getIndex() + offset << ")\n";

    // This also recalculates left flanks.
    PeakPool::collapse(nprevBestMax, foundIter);
    //
    // The right flank of nprevBestMax must/may be updated.
    nprevBestMax->logNextPeak(&* foundIter);

    // Re-score foundIter.
    foundIter->calcQualities(averages);

    if (foundIter->goodQuality())
      foundIter->select();

    cout << "peakHint now\n";
    cout << foundIter->strQuality(offset);
  }

  Piterator pnextSelected = PeakPool::nextExcl(foundIter,
    &Peak::isSelected);

  if (pnextSelected == peaks->begin())
  {
    cout << "PINSERT: Successor is end()\n";
    return false;
  }

  Piterator pnextBestMax;
  if (! PeakPool::getBestMax(foundIter, pnextSelected, foundIter,
    pnextBestMax))
  {
    cout << "PINSERT: Maximum is begin()\n";
    return false;
  }

  Piterator nfoundIter = next(foundIter);
  if (pnextBestMax != nfoundIter)
  {
    // Delete peaks in (foundIter, pnextBestMax).
    cout << "PINSERT: Peak exists\n";
    cout << foundIter->strHeaderQuality();
    cout << foundIter->strQuality(offset);

    cout << "PINSERT: Delete (" <<
      foundIter->getIndex() + offset << ", " <<
      pnextBestMax->getIndex() + offset << ")\n";
    
    PeakPool::collapse(nfoundIter, pnextBestMax);

    // The right flank of foundIter must be updated.
    foundIter->logNextPeak(&* pnextBestMax);

    // Re-score foundIter.
    foundIter->calcQualities(averages);

    if (foundIter->goodQuality())
      foundIter->select();

    cout << "peakHint now\n";
    cout << foundIter->strQuality(offset);
  }

  return true;
}


bool PeakPool::repairFromLower(
  Piterator& foundIter,
  const unsigned offset)
{
  // Peak only exists in earlier list and might be resurrected.
  // It would go between pfirstPrev and pfirstNext (one min, one max).
  PiterPair pfirstPrev, pfirstNext;
  PeakPool::getBracketingPeaks(peakLists.rbegin(), 
    foundIter->getIndex(), false, pfirstPrev, pfirstNext);

  if (! pfirstPrev.hasFlag || ! pfirstNext.hasFlag)
  {
    cout << "PINSERT: Not an interior interval\n";
    return false;
  }

  // Find the same bracketing peaks in the current list.
  Piterator pcurrPrev, pcurrNext;
  PeakPool::locateTopBrackets(pfirstPrev, pfirstNext, foundIter, 
    pcurrPrev, pcurrNext);

  // We only introduce two new peaks, so the one we found goes next 
  // to the bracketing one with opposite polarity.  In the gap we put 
  // the intervening peak with maximum value.  This is quick and dirty.
  auto pnew = peaks->insert(pfirstNext.pit, * foundIter);

  Peak * pmax;
  if (pcurrPrev->isMinimum())
  {
    // New peak goes next to pfirstNext.
    if (! PeakPool::getHighestMax(next(pcurrPrev), foundIter, pmax))
      return false;

    peaks->insert(pnew, * pmax);
  }
  else
  {
    // New peak goes next to pfirstPrev.
    if (! PeakPool::getHighestMax(next(foundIter), pcurrNext, pmax))
      return false;

    peaks->insert(pfirstNext.pit, * pmax);
  }

  PeakPool::updateRepairedPeaks(pfirstPrev.pit, pfirstNext.pit);

  PeakPool::printRepairedSegment(pfirstPrev.pit, pfirstNext.pit,
    offset);

  return true;
}


bool PeakPool::repair(
  const Peak& peakHint,
  const unsigned offset)
{
  const unsigned pindex = peakHint.getIndex();

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
      return PeakPool::repairTopLevel(foundIter, offset);
    }
    else
    {
      // Peak only exists in earlier list and might be resurrected.
      return PeakPool::repairFromLower(foundIter, offset);
    }
  }

  return false;
}


void PeakPool::makeCandidates()
{
  for (auto& peak: * peaks)
  {
    if (peak.isCandidate())
      candidates.push_back(&peak);
  }
}


unsigned PeakPool::countCandidates(const PeakFncPtr& fptr) const
{
  unsigned c = 0;
  for (auto cand: candidates)
  {
    if ((cand->* fptr)())
      c++;
  }
  return c;
}


unsigned PeakPool::candsize() const 
{
  return candidates.size();
}


void PeakPool::getCandPtrs(
  const unsigned start,
  const unsigned end,
  PeakPtrVector& peakPtrs) const
{
  peakPtrs.clear();
  for (auto& cand: candidates)
  {
    const unsigned index = cand->getIndex();
    if (index > end)
      break;
    if (index < start)
      continue;

    peakPtrs.push_back(cand);
  }
}


void PeakPool::getCandIters(
  const unsigned start,
  const unsigned end,
  PeakIterVector& peakIters) const
{
  peakIters.clear();
  for (PPciterator pit = candidates.begin(); pit != candidates.end(); pit++)
  {
    Peak * cand = * pit;
    const unsigned index = cand->getIndex();
    if (index > end)
      break;
    if (index < start)
      continue;

    peakIters.push_back(pit);
  }
}


void PeakPool::getCandPtrs(
  const unsigned start,
  const unsigned end,
  const PeakFncPtr& fptr,
  PeakPtrVector& peakPtrs) const
{
  for (auto& cand: candidates)
  {
    const unsigned index = cand->getIndex();
    if (index > end)
      break;
    if (index < start)
      continue;

    if ((cand->* fptr)())
      peakPtrs.push_back(cand);
  }
}


void PeakPool::getCands(
  const unsigned start,
  const unsigned end,
  PeakPtrVector& peakPtrs,
  PeakIterVector& peakIters) const
{
  peakPtrs.clear();
  peakIters.clear();
  for (PPciterator pit = candidates.begin(); pit != candidates.end(); pit++)
  {
    Peak * cand = * pit;
    const unsigned index = cand->getIndex();
    if (index > end)
      break;
    if (index < start)
      continue;

    peakIters.push_back(pit);
    peakPtrs.push_back(* pit);
  }
}


void PeakPool::getCands(
  const unsigned start,
  const unsigned end,
  PeakPtrVector& peakPtrs,
  PeakIterList& peakIters) const
{
  peakPtrs.clear();
  peakIters.clear();
  for (PPciterator pit = candidates.begin(); pit != candidates.end(); pit++)
  {
    Peak * cand = * pit;
    const unsigned index = cand->getIndex();
    if (index > end)
      break;
    if (index < start)
      continue;

    peakIters.push_back(pit);
    peakPtrs.push_back(* pit);
  }
}


unsigned PeakPool::firstCandIndex() const
{
  if (candidates.size() == 0)
    return numeric_limits<unsigned>::max();
  else
    return candidates.front()->getIndex();
}


unsigned PeakPool::lastCandIndex() const
{
  if (candidates.size() == 0)
    return numeric_limits<unsigned>::max();
  else
    return candidates.back()->getIndex();
}


PPiterator PeakPool::candbegin()
{
  return candidates.begin();
}


PPciterator PeakPool::candcbegin() const
{
  return candidates.cbegin();
}


PPiterator PeakPool::candend()
{
  return candidates.end();
}


PPciterator PeakPool::candcend() const
{
  return candidates.cend();
}


PPiterator PeakPool::nextCandExcl(
  const PPiterator& pit,
  const PeakFncPtr& fptr) const
{
  if (pit == candidates.end())
    THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss later matching peak");

  PPiterator npit = next(pit);
  return PeakPool::nextCandIncl(npit, fptr);
}


PPciterator PeakPool::nextCandExcl(
  const PPciterator& pit,
  const PeakFncPtr& fptr) const
{
  if (pit == candidates.end())
    THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss later matching peak");

  PPciterator npit = next(pit);
  return PeakPool::nextCandIncl(npit, fptr);
}


PPiterator PeakPool::nextCandIncl(
  PPiterator& pit,
  const PeakFncPtr& fptr) const
{
  PPiterator pitNext = pit;
  while (pitNext != candidates.end() && ! ((* pitNext)->* fptr)())
    pitNext++;

  return pitNext;
}


PPciterator PeakPool::nextCandIncl(
  const PPciterator& pit,
  const PeakFncPtr& fptr) const
{
  PPciterator pitNext = pit;
  while (pitNext != candidates.end() && ! ((* pitNext)->* fptr)())
    pitNext++;

  return pitNext;
}


PPiterator PeakPool::prevCandExcl(
  PPiterator& pit,
  const PeakFncPtr& fptr) const
{
  if (pit == candidates.begin())
    THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss earlier matching peak");

  PPiterator ppit = prev(pit);
  return PeakPool::prevCandIncl(ppit, fptr);
}


PPciterator PeakPool::prevCandExclSoft(
  PPciterator& pit,
  const PeakFncPtr& fptr) const
{
  if (pit == candidates.begin())
    return candidates.cend();

  PPciterator ppit = prev(pit);
  return PeakPool::prevCandInclSoft(ppit, fptr);
}


PPciterator PeakPool::prevCandExcl(
  PPciterator& pit,
  const PeakFncPtr& fptr) const
{
  if (pit == candidates.begin())
    THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss earlier matching peak");

  PPciterator ppit = prev(pit);
  return PeakPool::prevCandIncl(ppit, fptr);
}


PPiterator PeakPool::prevCandIncl(
  PPiterator& pit,
  const PeakFncPtr& fptr) const
{
  for (PPiterator pitPrev = pit; ; pitPrev = prev(pitPrev))
  {
    if (((* pitPrev)->* fptr)())
      return pitPrev;
    else if (pitPrev == candidates.begin())
      THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss earlier matching peak");
  }
}


PPciterator PeakPool::prevCandIncl(
  PPciterator& pit,
  const PeakFncPtr& fptr) const
{
  for (PPciterator pitPrev = pit; ; pitPrev = prev(pitPrev))
  {
    if (((* pitPrev)->* fptr)())
      return pitPrev;
    else if (pitPrev == candidates.begin())
      THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss earlier matching peak");
  }
}


PPciterator PeakPool::prevCandInclSoft(
  PPciterator& pit,
  const PeakFncPtr& fptr) const
{
  for (PPciterator pitPrev = pit; ; pitPrev = prev(pitPrev))
  {
    if (((* pitPrev)->* fptr)())
      return pitPrev;
    else if (pitPrev == candidates.begin())
      return candidates.end();
  }
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
  const PPciterator& cit,
  const unsigned numWheels,
  PeakPtrVector& closestPeaks,
  PeakPtrVector& skippedPeaks) const
{
  if (numWheels < 2)
    return false;

  // Do the three remaining wheels.
  PPciterator cit0 = PeakPool::nextCandExcl(cit, fptr);
  if (cit0 == candidates.end())
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

  PPciterator cit1;
  for (unsigned i = 0; i < numWheels-1; i++)
  {
    const unsigned ptarget = (* pointIt) + pstart - cstart;

    while (true)
    {
      cit1 = PeakPool::nextCandExcl(cit0, fptr);
      if (cit1 == candidates.end())
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
        cit1 = PeakPool::nextCandExcl(cit1, fptr);
        if (cit1 == candidates.end() && closestPeaks.size() != numWheels)
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


string PeakPool::strAllCandsQuality(
  const string& text,
  const unsigned& offset) const
{
  if (candidates.empty())
    return "";

  stringstream ss;
  if (text != "")
    ss << text << ": " << candidates.size() << "\n";
  ss << candidates.front()->strHeaderQuality();

  for (auto cand: candidates)
    ss << cand->strQuality(offset);
  ss << endl;
  return ss.str();
}


string PeakPool::strSelectedCandsQuality(
  const string& text,
  const unsigned& offset) const
{
  if (candidates.empty())
    return "";

  stringstream ss;
  if (text != "")
    ss << text << "\n";
  ss << candidates.front()->strHeaderQuality();

  for (auto cand: candidates)
  {
    if (cand->isSelected())
      ss << cand->strQuality(offset);
  }
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

