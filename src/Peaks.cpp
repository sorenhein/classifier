#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>
#include <math.h>

#include "Peaks.h"
#include "Except.h"


#define SAMPLE_RATE 2000.


Peaks::Peaks()
{
  Peaks::clear();
}


Peaks::~Peaks()
{
}


void Peaks::clear()
{
  peaks.clear();
}


bool Peaks::empty() const
{
  return peaks.empty();
}


unsigned Peaks::size() const 
{
  return peaks.size();
}


Piterator Peaks::begin()
{
  return peaks.begin();
}


Pciterator Peaks::cbegin() const
{
  return peaks.cbegin();
}


Piterator Peaks::end()
{
  return peaks.end();
}


Pciterator Peaks::cend() const
{
  return peaks.cend();
}


Peak& Peaks::front()
{
  return peaks.front();
}


Peak& Peaks::back()
{
  return peaks.back();
}


Piterator Peaks::insert(
  Piterator& pit,
  Peak& peak)
{
  return peaks.insert(pit, peak);
}


void Peaks::extend()
{
  peaks.emplace_back(Peak());
}


Piterator Peaks::erase(
  Piterator pit1,
  Piterator pit2)
{
  return peaks.erase(pit1, pit2);
}


Piterator Peaks::collapse(
  Piterator pit1,
  Piterator pit2)
{
  // Analogous to erase(): pit1 does not survive, while pit2 does.
  // But pit2 gets updated first.
  if (pit1 == pit2)
    return pit1;

  Peak * peak0 =
    (pit1 == peaks.begin() ? nullptr : &*std::prev(pit1));

  if (peak0 != nullptr)
  {
    pit2->update(peak0);
    peak0->logNextPeak(&* pit2);
  }

  return peaks.erase(pit1, pit2);
}


Piterator Peaks::next(
  const Piterator& pit,
  const PeakFncPtr& fptr,
  const bool exclFlag)
{
  if (pit == peaks.end())
    return peaks.end();

  Piterator pitNext = (exclFlag ? std::next(pit) : pit);
  while (pitNext != peaks.end() && ! ((* pitNext).* fptr)())
    pitNext++;

  return pitNext;
}


Pciterator Peaks::next(
  const Piterator& pit,
  const PeakFncPtr& fptr,
  const bool exclFlag) const
{
  if (pit == peaks.end())
    return peaks.end();

  Pciterator pitNext = (exclFlag ? std::next(pit) : pit);
  while (pitNext != peaks.end() && ! ((* pitNext).* fptr)())
    pitNext++;

  return pitNext;
}


Piterator Peaks::prev(
  const Piterator& pit,
  const PeakFncPtr& fptr,
  const bool exclFlag)
{
  // Return something that cannot be useful iterator.
  if (pit == peaks.begin())
    return peaks.end();

  Piterator pitPrev = (exclFlag ? std::prev(pit) : pit);
  while (true)
  {
    if (((* pitPrev).* fptr)())
      return pitPrev;
    else if (pitPrev == peaks.begin())
      return peaks.end();
    else
      pitPrev = std::prev(pitPrev);
  }
}


Pciterator Peaks::prev(
  const Piterator& pit,
  const PeakFncPtr& fptr,
  const bool exclFlag) const
{
  // Return something that cannot be useful iterator.
  if (pit == peaks.begin())
    return peaks.end();

  Piterator pitPrev = (exclFlag ? std::prev(pit) : pit);
  while (true)
  {
    if (((* pitPrev).* fptr)())
      return pitPrev;
    else if (pitPrev == peaks.begin())
      return peaks.end();
    else
      pitPrev = std::prev(pitPrev);
  }
}


bool Peaks::near(
  const Peak& peakHint,
  const Bracket& bracket,
  Piterator& foundIter) const
{
  // If one of the bracketing minima is near enough, return it.
  // If they're both near, pick the nearest one.

  bool fitsNext = false;
  bool fitsPrev = false;
  if (bracket.left.hasFlag && peakHint.fits(* bracket.left.pit))
    fitsPrev = true;
  if (bracket.right.hasFlag && peakHint.fits(* bracket.right.pit))
    fitsNext = true;

  if (fitsPrev && fitsNext)
  {
    if (bracket.left.pit->getValue() <= bracket.right.pit->getValue())
      foundIter = bracket.left.pit;
    else
      foundIter = bracket.right.pit;
  }
  else if (fitsPrev)
    foundIter = bracket.left.pit;
  else if (fitsNext)
    foundIter = bracket.right.pit;
  else
    return false;

  return (foundIter->isCandidate());
}


bool Peaks::stats(
  const unsigned lower,
  float& mean,
  float& sdev) const
{
  float sum = 0., sumsq = 0.;
  unsigned num = 0;
  for (auto& peak: peaks)
  {
    if (peak.isSelected() && peak.getIndex() >= lower)
    {
      const float level = peak.getValue();
      sum += level;
      sumsq += level * level;
      num++;
    }
  }

  if (num <= 1)
    return false;

  mean = sum / num;
  sdev = sqrt(sumsq / (num-1) - mean * mean);
  return true;
}


void Peaks::bracket(
  const unsigned pindex,
  const bool minFlag,
  Bracket& bracket)
{
  // Find bracketing minima if minFlag is set, 
  // otherwise find bracketing extrema.
  bracket.left.hasFlag = false;
  bracket.right.hasFlag = false;
  for (auto pit = peaks.begin(); pit != peaks.end(); pit++)
  {
    if (minFlag && ! pit->isMinimum())
      continue;

    if (pit->getIndex() > pindex)
    {
      bracket.right.pit = pit;
      bracket.right.hasFlag = true;
      break;
    }

    bracket.left.pit = pit;
    bracket.left.hasFlag = true;
  }
}


void Peaks::bracketSpecific(
  const Bracket& bracketOther,
  const Piterator& foundIter,
  Bracket& bracket) const
{
  // We have bracketing iterators in another Peaks object.
  // We have foundIter which is in our current object.
  // We are looking for the iterators to the same peaks in this object.
  // They must exist!

  const unsigned ppindex = bracketOther.left.pit->getIndex();
  const unsigned pnindex = bracketOther.right.pit->getIndex();

  bracket.left.pit = foundIter;
  do
  {
    if (bracket.left.pit == peaks.begin())
      break;

    bracket.left.pit = std::prev(bracket.left.pit);
  }
  while (bracket.left.pit->getIndex() > ppindex);

  if (bracket.left.pit->getIndex() != ppindex)
  {
    cout << "bracketSpecific: Looked down for " <<
      ppindex << " to " << pnindex << "\n";
    cout << "Reached " << bracket.left.pit->getIndex() << endl;
    THROW(ERR_ALGO_PEAK_CONSISTENCY, "Lower bracket error");
  }

  bracket.left.hasFlag = true;

  bracket.right.pit = foundIter;
  do
  {
    bracket.right.pit = std::next(bracket.right.pit);
    
    if (bracket.right.pit == peaks.end())
      break;
  }
  while (bracket.right.pit->getIndex() < pnindex);

  if (bracket.right.pit->getIndex() != pnindex)
  {
    cout << "bracketSpecific: Looked up for " <<
      ppindex << " to " << pnindex << "\n";
    cout << "Reached " << bracket.right.pit->getIndex() << endl;
    THROW(ERR_ALGO_PEAK_CONSISTENCY, "Upper bracket error");
  }

  bracket.right.hasFlag = true;
}


bool Peaks::brackets(
  const Piterator& foundIter,
  Bracket& bracketOuterMin,
  Bracket& bracketInnerMax)
{
  // Find the two surrounding brackets:
  // bracketOuterMin  |                                       |
  // brackerInnerMax            |                   |
  // foundIter                           *

  // Find the previous selected minimum.  
  bracketOuterMin.left.pit = Peaks::prev(foundIter, &Peak::isSelected);
  if (bracketOuterMin.left.pit == peaks.end())
  {
    cout << "PINSERT: Predecessor is end()\n";
    return false;
  }

  // Find the following selected minimum.
  bracketOuterMin.right.pit = Peaks::next(foundIter, &Peak::isSelected);
  if (bracketOuterMin.right.pit == peaks.end())
  {
    cout << "PINSERT: Successor is end()\n";
    return false;
  }

  // Find the in-between maximum that maximizes slope * range,
  // i.e. range^2 / len.
  if (! Peaks::getBestMax(bracketOuterMin.left.pit, foundIter, foundIter,
    bracketInnerMax.left.pit))
  {
    cout << "PINSERT: Maximum is begin()\n";
    return false;
  }

  if (! Peaks::getBestMax(foundIter, bracketOuterMin.right.pit, foundIter,
    bracketInnerMax.right.pit))
  {
    cout << "PINSERT: Maximum is end()\n";
    return false;
  }

  return true;
}


bool Peaks::getHighestMax(
  const Bracket& bracket,
  Piterator& pmax)
{
  pmax = peaks.end();
  float val = numeric_limits<float>::lowest();
  for (auto pit = bracket.left.pit; pit != bracket.right.pit; pit++)
  {
    if (pit->getValue() > val)
    {
      val = pit->getValue();
      pmax = pit;
    }
  }

  if (pmax == peaks.end())
  {
    cout << "PINSERT: Null maximum\n";
    return false;
  }
  else
    return true;
}


bool Peaks::getBestMax(
  const Piterator& leftIter,
  const Piterator& rightIter,
  const Piterator& pref,
  Piterator& pbest)
{
  pbest = peaks.begin();
  float valMax = 0.f;

  for (auto pit = leftIter; pit != rightIter; pit++)
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
  return (pbest != peaks.begin());
}


void Peaks::getSamples(
  const PeakFncPtr& fptr,
  vector<float>& selected) const
{
  for (unsigned i = 0; i < selected.size(); i++)
    selected[i] = 0;

  for (auto& peak: peaks)
  {
    if ((peak.* fptr)())
      selected[peak.getIndex()] = peak.getValue();
  }
}


float Peaks::getFirstPeakTime(const PeakFncPtr& fptr) const
{
  for (auto& peak: peaks)
  {
    if ((peak.* fptr)())
      return peak.getIndex() / static_cast<float>(SAMPLE_RATE);
  }
  return 0.f;
}


void Peaks::getTimes(
  const PeakFncPtr& fptr,
  vector<PeakTime>& times) const
{
  times.clear();
  const float t0 = Peaks::getFirstPeakTime(fptr);

  for (auto& peak: peaks)
  {
    if ((peak.* fptr)())
    {
      times.emplace_back(PeakTime());
      PeakTime& p = times.back();
      p.time = peak.getIndex() / SAMPLE_RATE - t0;
      p.value = peak.getValue();
    }
  }
}


bool Peaks::check() const
{
  bool flag = true;
  for (auto& peak: peaks)
  {
    if (! peak.check())
    {
      cout << "PEAKS ERROR\n";
      cout << peak.strQuality();
      flag = false;
    }
  }
  return flag;
}


string Peaks::strTimesCSV(
  const PeakFncPtr& fptr,
  const string& text) const
{
  if (peaks.empty())
    return "";

  stringstream ss;
  ss << text << "n";
  unsigned i = 0;
  for (auto& peak: peaks)
  {
    if ((peak.* fptr)())
      ss << i++ << ";" <<
        fixed << setprecision(6) << peak.getTime() << "\n";
  }
  ss << endl;
  return ss.str();
}


string Peaks::str(
  const string& text,
  const unsigned& offset) const
{
  if (peaks.empty())
    return "";

  stringstream ss;
  if (text != "")
    ss << text << ": " << peaks.size() << "\n";
  ss << peaks.front().strHeader();

  for (auto& peak: peaks)
    ss << peak.str(offset);
  ss << endl;
  return ss.str();
}

