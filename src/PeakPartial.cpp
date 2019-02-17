#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "PeakPartial.h"
#include "Peak.h"


PeakPartial::PeakPartial()
{
  PeakPartial::reset();
}


PeakPartial::~PeakPartial()
{
}


void PeakPartial::reset()
{
  modelNo = 0;
  reverseFlag = false;

  modelUsedFlag = false;
  aliveFlag = true;
  skippedFlag = false;
  newFlag = true;
  lastIndex = 0;

  peaks.resize(4, nullptr);
  lower.resize(4, 0);
  upper.resize(4, 0);
  target.resize(4, 0);
  indexUsed.resize(4, 0);
  numUsed = 0;
}


void PeakPartial::init(
  const unsigned modelNoIn,
  const bool reverseFlagIn,
  const unsigned startIn)
{
  modelNo = modelNoIn;
  reverseFlag = reverseFlagIn;
  lastIndex = startIn;
}


void PeakPartial::registerRange(
  const unsigned peakNo,
  const unsigned lowerIn,
  const unsigned upperIn)
{
  lower[peakNo] = lowerIn;
  upper[peakNo] = upperIn;
  target[peakNo] = (lowerIn + upperIn) / 2;
}


void PeakPartial::registerIndexUsed(
  const unsigned peakNo,
  const unsigned indexUsedIn)
{
  indexUsed[peakNo] = indexUsedIn;
}


void PeakPartial::registerPtr(
  const unsigned peakNo,
  Peak * pptr)
{
  peaks[peakNo] = pptr;

  if (pptr)
  {
    modelUsedFlag = true;
    newFlag = true;
    skippedFlag = false;
    lastIndex = pptr->getIndex();
    numUsed++;
  }
  else
  {
    newFlag = false;
    if (skippedFlag)
      aliveFlag = false;
    else
      skippedFlag = true;
  }
}


void PeakPartial::registerFinished()
{
  newFlag = false;
  aliveFlag = false;
}


void PeakPartial::getPeak(
  const unsigned peakNo,
  Peak& peak) const
{
  peak.logPosition(target[peakNo], lower[peakNo], upper[peakNo]);
}


bool PeakPartial::closeEnoughFull(
  const unsigned index,
  const unsigned peakNo,
  const unsigned posNo) const
{
  if ((peakNo < posNo && index > upper[posNo]) ||
      (peakNo > posNo && index < lower[posNo]))
    return true;
  else
    return PeakPartial::closeEnough(index, peakNo, posNo);
}


bool PeakPartial::closeEnough(
  const unsigned index,
  const unsigned peakNo,
  const unsigned posNo) const
{
  if (peakNo < posNo)
    return (lower[posNo] - index < (upper[posNo] - lower[posNo]) / 2);
  else if (peakNo > posNo)
    return (index - upper[posNo] < (upper[posNo] - lower[posNo]) / 2);
  else if (index < lower[posNo])
    return (lower[posNo] - index < (upper[posNo] - lower[posNo]) / 2);
  else if (index > upper[posNo])
    return (index - upper[posNo] < (upper[posNo] - lower[posNo]) / 2);
  else
    return true;
}


bool PeakPartial::dominates(const PeakPartial& p2) const
{
  if (p2.numUsed == 0)
    return true;

  // This is a bunch of heuristics followed by the actual rule.

  if (p2.numUsed <= 2 && numUsed > p2.numUsed)
  {
    // p2 is subsumed in this.
    bool subsumedFlag = true;
    unsigned index = 0;
    for (unsigned i = 0; i < 4; i++)
    {
      if (! p2.peaks[i])
        continue;
      while (index < 4 && 
          (! peaks[index] || peaks[index] != p2.peaks[i]))
        index++;
      if (index == 4)
      {
        subsumedFlag = false;
        break;
      }
    }
    if (subsumedFlag)
      return true;
  }

  if (numUsed == p2.numUsed && numUsed < 4)
  {
    // Start out the same (from the right).
    bool sameFlag = true;
    for (unsigned i = 5-numUsed; i < 4; i++)
    {
      if (! peaks[i] || p2.peaks[i] != peaks[i])
      {
        sameFlag = false;
        break;
      }
    }

    // The first peak (from the left) slipped to the left, but is
    // close enough.
    Peak * ptr = p2.peaks[3-numUsed];
    if (sameFlag &&
        p2.peaks[3-numUsed] && ptr == peaks[4-numUsed] &&
        p2.closeEnoughFull(ptr->getIndex(), 3-numUsed, 4-numUsed))
      return true;
  }

  for (unsigned i = 0; i < 4; i++)
  {
    if (! peaks[i] && p2.peaks[i])
      return false;

    // If not the same peak, it's not so clear.
    if (peaks[i] && p2.peaks[i] && peaks[i] != p2.peaks[i])
      return false;
  }
  return true;
}


bool PeakPartial::samePeaks(const PeakPartial& p2) const
{
  for (unsigned i = 0; i < 4; i++)
  {
    if ((peaks[i] && ! p2.peaks[i]) ||
        (! peaks[i] && p2.peaks[i]))
      return false;
    
    if (peaks[i] != p2.peaks[i])
      return false;
  }
  return true;
}


void PeakPartial::extend(const PeakPartial& p2)
{
  for (unsigned i = 0; i < 4; i++)
  {
    if (peaks[i] || p2.peaks[i])
      continue;

    if (p2.lower[i] != 0 && p2.lower[i] < lower[i])
      lower[i] = p2.lower[i];

    if (p2.upper[i] != 0 && p2.upper[i] > upper[i])
      upper[i] = p2.upper[i];
  }
}


bool PeakPartial::merge(const PeakPartial& p2)
{
  // This is a heuristic.
  if (numUsed != p2.numUsed || numUsed == 4)
    return false;

  // Start out the same (from the right).
  bool sameFlag = true;
  for (unsigned i = 5-numUsed; i < 4; i++)
  {
    if (! peaks[i] || p2.peaks[i] != peaks[i])
    {
      sameFlag = false;
      break;
    }
  }
  if (! sameFlag)
    return false;

  const unsigned start = 3-numUsed;
  if (peaks[start] && p2.peaks[start+1])
  {
    if (peaks[start] == p2.peaks[start+1])
      return false;

    if (PeakPartial::closeEnough(p2.peaks[start+1]->getIndex(),
        start+1, start+1))
    {
      // Merge in p2.peaks[start+1] into peaks[start+1].
      peaks[start+1] = p2.peaks[start+1];
      indexUsed[start+1] = p2.indexUsed[start+1];
      numUsed++;
      return true;
    }
    else
      return false;
      
  }
  else if (peaks[start+1] && p2.peaks[start])
  {
    if (peaks[start+1] == p2.peaks[start])
      return false;

    if (p2.closeEnough(peaks[start+1]->getIndex(), start+1, start+1))
    {
      // Merge in p2.peaks[start] into peaks[start].
      peaks[start] = p2.peaks[start];
      indexUsed[start] = p2.indexUsed[start];
      numUsed++;
      return true;
    }
    else
      return false;
  }
  else
    return false;
}


bool PeakPartial::supersede(const PeakPartial& p2)
{
  if (PeakPartial::dominates(p2))
  {
    return true;
  }
  else if (p2.dominates(* this))
  {
    * this = p2;
    return true;
  }
  else if (PeakPartial::samePeaks(p2))
  {
    PeakPartial::extend(p2);
    return true;
  }
  else if (PeakPartial::merge(p2))
  {
    return true;
  }
  else
    return false;
}


void PeakPartial::getPeaks(
  vector<Peak *>& peakPtrsUsed,
  vector<Peak *>& peakPtrsUnused) const
{
  // if we didn't match a peak in the used vector, we move it to unused.
  const unsigned np = peakPtrsUsed.size();
  vector<unsigned> v(np, 0);
  for (unsigned i = 0; i < 4; i++)
  {
    if (peaks[i])
      v[indexUsed[i]] = 1;
  }

  for (unsigned i = 0; i < np; i++)
  {
    if (v[i] == 0)
      peakPtrsUnused.push_back(peakPtrsUsed[i]);
  }

  peakPtrsUsed = peaks;
}


unsigned PeakPartial::number() const
{
  return modelNo;
}


unsigned PeakPartial::count() const
{
  return numUsed;
}


unsigned PeakPartial::latest() const
{
  return lastIndex;
}


bool PeakPartial::reversed() const
{
  return reverseFlag;
}


bool PeakPartial::skipped() const
{
  return skippedFlag;
}


bool PeakPartial::alive() const
{
  return aliveFlag;
}


bool PeakPartial::used() const
{
  return modelUsedFlag;
}


bool PeakPartial::hasPeak(const unsigned peakNo) const
{
  return (peaks[peakNo] != nullptr);
}


bool PeakPartial::hasRange(const unsigned peakNo) const
{
  return (upper[peakNo] > 0);
}


string PeakPartial::strTarget(
  const unsigned peakNo,
  const unsigned offset) const
{
  return to_string(target[peakNo] + offset);
}


string PeakPartial::strIndex(
  const unsigned peakNo,
  const unsigned offset) const
{
  if (peaks[peakNo] == nullptr)
  {
    if (lower[peakNo] == 0)
      return "-";
    else
      return "(" + to_string(lower[peakNo] + offset) + ", " +
        to_string(upper[peakNo] + offset) + ")";
  }
  else
    return to_string(peaks[peakNo]->getIndex() + offset) +
      " (" + to_string(indexUsed[peakNo]) + ")";
}


string PeakPartial::strHeader() const
{
  stringstream ss;
  ss <<
    setw(4) << left << "mno" <<
    setw(4) << left << "rev" <<
    setw(16) << left << "p1" <<
    setw(16) << left << "p2" <<
    setw(16) << left << "p3" <<
    setw(16) << left << "p4" << endl;
  return ss.str();
}


string PeakPartial::str(const unsigned offset) const
{
  stringstream ss;
  ss <<
    setw(4) << left << modelNo <<
    setw(4) << left << (reverseFlag ? "R" : "") <<
    setw(16) << left << PeakPartial::strIndex(0, offset) <<
    setw(16) << left << PeakPartial::strIndex(1, offset) <<
    setw(16) << left << PeakPartial::strIndex(2, offset) <<
    setw(16) << left << PeakPartial::strIndex(3, offset) << "\n";
  return ss.str();
}

