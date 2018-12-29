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
  PPiterator& pit,
  const PeakFncPtr& fptr) const
{
  if (pit == candidates.end())
    THROW(ERR_ALGO_PEAK_CONSISTENCY, "Miss later matching peak");

  PPiterator npit = next(pit);
  return PeakPool::nextCandIncl(npit, fptr);
}


PPciterator PeakPool::nextCandExcl(
  PPciterator& pit,
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
  PPciterator& pit,
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

