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
  // Analogous to erase(): peak1 does not survive, while peak2 does.
  // But pit2 gets updated first.
  if (pit1 == pit2)
    return pit1;

  Peak * peak0 =
    (pit1 == peaks->begin() ? nullptr : &*prev(pit1));

  if (peak0 != nullptr)
    pit2->update(peak0);

  return peaks->erase(pit1, pit2);
}


void PeakPool::getBracketingMinima(
  const list<PeakList>::reverse_iterator& liter,
  const unsigned pindex,
  PiterPair& pprev,
  PiterPair& pnext) const
{
  // Find bracketing minima.
  pprev.hasFlag = false;
  pnext.hasFlag = false;
  for (auto piter = liter->begin(); piter != liter->end(); piter++)
  {
    if (! piter->isMinimum())
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


bool PeakPool::repair(const Peak& peakHint)
{
  const unsigned pindex = peakHint.getIndex();
  PiterPair pfirstPrev, pfirstNext;

  unsigned ldepth = 0;
  for (auto liter = peakLists.rbegin(); liter != peakLists.rend(); 
      liter++, ldepth++)
  {
    if (liter->size() < 2)
      continue;

    // Find bracketing minima.
    PiterPair pprev, pnext;
    PeakPool::getBracketingMinima(liter, pindex, pprev, pnext);

    // Is one of them close enough?  If both, pick the lowest value.
    Piterator foundIter;
    if (! PeakPool::findCloseIter(peakHint, pprev, pnext, foundIter))
    {
      pfirstPrev = pprev;
      pfirstNext = pnext;
      continue;
    }

    if (liter == peakLists.rbegin())
    {
      cout << "Peak exists (without offset)\n";
      cout << foundIter->strHeaderQuality();
      cout << foundIter->strQuality();
      
      // Peak exists but is not good enough, perhaps because of 
      // neighboring spurious peaks.  To salvage the peak we'll have
      // to remove kinks.
    }
    else
    {
      // Peak only exists in earlier list and might be resurrected.
      // It would go between pfirstPrev and pfirstNext.
      // There will be a maximum in between which will go on one
      // side of the peak.  We have to find another maximum (maybe two)
      // on the other side.  Our peak has to have a good quality.
      // Actually we don't have access to the average peaks here?
      // Maybe PeakDetect should store them here.
      // If our two modified minima are both of good quality,
      // modify the (first) list of peaks including flanks and qualities.
      // Also modify candidates.
      
      cout << "Peak found at depth " << ldepth << "\n";
      cout << foundIter->strHeaderQuality();
      cout << foundIter->strQuality();
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

