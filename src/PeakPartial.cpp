#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "PeakPartial.h"
#include "Peak.h"
#include "PeakPtrs.h"

#define INDEX_NOT_USED 999


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

  comps.resize(4);
  for (unsigned i = 0; i < 4; i++)
  {
    comps[i].peak = nullptr;
    comps[i].lower = 0;
    comps[i].upper = 0;
    comps[i].target = 0;
    comps[i].indexUsed = 0;
  }

  numUsed = 0;

  peakSlots.reset();

  pstats.clear();
  pstats.resize(16);
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
  const unsigned lower,
  const unsigned upper)
{
  comps[peakNo].lower = lower;
  comps[peakNo].upper = upper;
  comps[peakNo].target = (lower + upper) / 2;
}


void PeakPartial::movePtr(
  const unsigned indexFrom,
  const unsigned indexTo)
{
  comps[indexTo] = comps[indexFrom];

  comps[indexFrom].peak = nullptr;
  comps[indexFrom].indexUsed = INDEX_NOT_USED;
}


void PeakPartial::registerIndexUsed(
  const unsigned peakNo,
  const unsigned indexUsed)
{
  comps[peakNo].indexUsed = indexUsed;
}


void PeakPartial::registerPtr(
  const unsigned peakNo,
  Peak const * pptr)
{
  comps[peakNo].peak = pptr;
  comps[peakNo].indexUsed = INDEX_NOT_USED;

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


void PeakPartial::registerPtr(
  const unsigned peakNo,
  Peak const * pptr,
  const unsigned indexUsedIn)
{
  PeakPartial::registerPtr(peakNo, pptr);
  PeakPartial::registerIndexUsed(peakNo, indexUsedIn);
}


void PeakPartial::registerFinished()
{
  newFlag = false;
  aliveFlag = false;
}


bool PeakPartial::getRange(
  const unsigned peakNo,
  unsigned& lowerOut,
  unsigned& upperOut) const
{
  if (comps[peakNo].upper == 0)
    return false;

  lowerOut = comps[peakNo].lower;
  upperOut = comps[peakNo].upper;
  return true;
}


void PeakPartial::getPeak(
  const unsigned peakNo,
  Peak& peak) const
{
  peak.logPosition(
    comps[peakNo].target, 
    comps[peakNo].lower, 
    comps[peakNo].upper);
}


bool PeakPartial::closeEnoughFull(
  const unsigned index,
  const unsigned peakNo,
  const unsigned posNo) const
{
  if ((peakNo < posNo && index > comps[posNo].upper) ||
      (peakNo > posNo && index < comps[posNo].lower))
    return true;
  else
    return PeakPartial::closeEnough(index, peakNo, posNo);
}


bool PeakPartial::closeEnough(
  const unsigned index,
  const unsigned peakNo,
  const unsigned posNo) const
{
  // TODO Divide by 2 or not?
  const unsigned delta = (comps[posNo].upper - comps[posNo].lower) / 2;

  if (peakNo < posNo)
    return (comps[posNo].lower - index < delta);
  else if (peakNo > posNo)
    return (index - comps[posNo].upper < delta);
  else if (index < comps[posNo].lower)
    return (comps[posNo].lower - index < delta);
  else if (index > comps[posNo].upper)
    return (index - comps[posNo].upper < delta);
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
      if (! p2.comps[i].peak)
        continue;
      while (index < 4 && 
          (! comps[index].peak || comps[index].peak != p2.comps[i].peak))
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
      if (! comps[i].peak || p2.comps[i].peak != comps[i].peak)
      {
        sameFlag = false;
        break;
      }
    }

    // The first peak (from the left) slipped to the left, but is
    // close enough.
    Peak const * ptr = p2.comps[3-numUsed].peak;
    if (sameFlag &&
        p2.comps[3-numUsed].peak && 
        ptr == comps[4-numUsed].peak &&
        p2.closeEnoughFull(ptr->getIndex(), 3-numUsed, 4-numUsed))
      return true;
  }

  for (unsigned i = 0; i < 4; i++)
  {
    if (! comps[i].peak && p2.comps[i].peak)
      return false;

    // If not the same peak, it's not so clear.
    if (comps[i].peak && 
        p2.comps[i].peak && 
        comps[i].peak != p2.comps[i].peak)
      return false;
  }
  return true;
}


bool PeakPartial::samePeaks(const PeakPartial& p2) const
{
  for (unsigned i = 0; i < 4; i++)
  {
    if ((comps[i].peak && ! p2.comps[i].peak) ||
        (! comps[i].peak && p2.comps[i].peak))
      return false;
    
    if (comps[i].peak != p2.comps[i].peak)
      return false;
  }
  return true;
}


bool PeakPartial::samePeakValues(const PeakPartial& p2) const
{
  // Could be p1 - p2 - nullptr - p4 and p1 - p2 - p4 - nullptr.
  // If so, pick either one.
  vector<const Peak *> v1, v2;
  for (unsigned i = 0; i < 4; i++)
  {
    if (comps[i].peak != p2.comps[i].peak)
    {
      v1.push_back(comps[i].peak);
      v2.push_back(p2.comps[i].peak);
    }
  }
  if (v1.size() != 2)
    return false;

  if (v1[0] != v2[1] || v1[1] != v2[0])
    return false;

  return (v1[0] == nullptr || v2[0] == nullptr);
}


void PeakPartial::extend(const PeakPartial& p2)
{
  for (unsigned i = 0; i < 4; i++)
  {
    if (comps[i].peak || p2.comps[i].peak)
      continue;

    if (p2.comps[i].lower != 0 && 
        (comps[i].lower == 0 || p2.comps[i].lower < comps[i].lower))
      comps[i].lower = p2.comps[i].lower;

    if (p2.comps[i].upper != 0 && 
        (comps[i].upper == 0 || p2.comps[i].upper > comps[i].upper))
      comps[i].upper = p2.comps[i].upper;
  }
}


bool PeakPartial::merge(const PeakPartial& p2)
{
  if (numUsed == 0 || numUsed == 4)
    return false;

  bool mergeFrom = true, mergeTo = true;
  for (unsigned i = 0; i < 4; i++ && mergeFrom && mergeTo)
  {
    if (comps[i].peak)
    {
      if (p2.comps[i].peak)
      {
        if (comps[i].peak == p2.comps[i].peak)
          continue;
        else
          return false;
      }
      else if (! p2.closeEnough(comps[i].peak->getIndex(), i, i))
        mergeTo = false;
    }
    else if (p2.comps[i].peak)
    {
      if (! PeakPartial::closeEnough(p2.comps[i].peak->getIndex(), i, i))
        mergeFrom = false;
    }
  }

  if (mergeFrom || mergeTo)
  {
    for (unsigned i = 0; i < 4; i++)
    {
      if (! comps[i].peak && p2.comps[i].peak)
      {
        comps[i] = p2.comps[i];
        numUsed++;
      }
    }
    return true;
  }
  else 
    return false;
}


bool PeakPartial::supersede(const PeakPartial& p2)
{
  if (PeakPartial::dominates(p2))
  {
    PeakPartial::extend(p2);
    return true;
  }
  else if (p2.dominates(* this))
  {
    // Kludge to keep p2 constant.
    PeakPartial p3 = p2;
    p3.extend(* this);
    * this = p3;
    return true;
  }
  else if (PeakPartial::samePeaks(p2))
  {
    PeakPartial::extend(p2);
    return true;
  }
  else if (PeakPartial::samePeakValues(p2))
    return true;
  else if (PeakPartial::merge(p2))
    return true;
  else
    return false;
}


string PeakPartial::strEntry(const unsigned n) const
{
  if (n == 0)
    return "-";
  else
    return to_string(n);
}


void PeakPartial::makeCodes(
  const PeakPtrs& peakPtrsUsed,
  const unsigned offset)
{
  peakSlots.log(comps[0].peak, comps[1].peak, comps[2].peak, comps[3].peak,
    peakPtrsUsed, offset);
}


void PeakPartial::printSituation(const bool firstFlag) const
{
  cout << peakSlots.str(firstFlag);
}


void PeakPartial::moveUnused(
  vector<Peak const *>& peakPtrsUsed,
  vector<Peak const *>& peakPtrsUnused) const
{
  const unsigned np = peakPtrsUsed.size();
  vector<unsigned> v(np, 0);
  for (unsigned i = 0; i < 4; i++)
  {
    if (comps[i].peak && comps[i].indexUsed != INDEX_NOT_USED)
      v[comps[i].indexUsed] = 1;
  }

  // Don't remove peaks in front of the car -- could be a new car.
  const unsigned preserveLimit = 
    (comps[0].peak ? comps[0].peak->getIndex() : 0);

  for (unsigned i = 0; i < np; i++)
  {
    if (v[i] == 0 && peakPtrsUsed[i]->getIndex() >= preserveLimit)
      peakPtrsUnused.push_back(peakPtrsUsed[i]);
  }

  peakPtrsUsed.clear();
  for (unsigned i = 0; i < 4; i++)
    peakPtrsUsed.push_back(comps[i].peak);
}


// TODO TMP: Should become a class (PeakPtrVector).  Then eliminate
// duplication with PeakRepair.
Peak const * PeakPartial::locatePeak(
  const unsigned lowerIn,
  const unsigned upperIn,
  const unsigned hint,
  const PeakFncPtr& fptr,
  vector<Peak const *>& peakPtrsUsed,
  unsigned& indexUsedOut) const
{
  unsigned num = 0;
  unsigned i = 0;
  unsigned dist = numeric_limits<unsigned>::max();
  Peak const * ptr = nullptr;
  for (auto& p: peakPtrsUsed)
  {
    if (! (p->* fptr)())
    {
      i++;
      continue;
    }

    const unsigned index = p->getIndex();
    if (index >= lowerIn && index <= upperIn)
    {
      num++;
      if ((index >= hint && index - hint < dist) ||
          (index < hint && hint - index < dist))
      {
        indexUsedOut = i;
        ptr = p;
      }
    }
    i++;
  }

  if (num == 0)
    return nullptr;

  if (num > 1)
    cout << "WARNING locatePeak: Multiple choices in (" <<
      lowerIn << ", " << upperIn << ")\n";

  return ptr;
}


Peak const * PeakPartial::closePeak(
  const float& factor,
  const bool upFlag,
  const PeakFncPtr& fptr,
  const vector<Peak const *>& peakPtrsUsed,
  const unsigned iuKnown,
  const unsigned iuCand) const
{
  const unsigned step = static_cast<unsigned>(factor * bogey);
  const unsigned start = peakPtrsUsed[iuKnown]->getIndex();

  unsigned nextLo, nextHi;
  if (upFlag)
  {
    nextLo = start + bogey - step;
    nextHi = start + bogey + step;
  }
  else
  {
    nextLo = start - bogey - step;
    nextHi = start - bogey + step;
  }

// cout << "CLOSE " << factor << " " << upFlag << ", " <<
  // nextLo << ", " << nextHi << " iuCand " << iuCand << endl;

  const unsigned cand = peakPtrsUsed[iuCand]->getIndex();
  if (cand >= nextLo && 
      cand <= nextHi && 
      (peakPtrsUsed[iuCand]->* fptr)())
    return peakPtrsUsed[iuCand];
  else
    return nullptr;
}


Peak const * PeakPartial::closePeak(
  const float& smallFactor,
  const float& largeFactor,
  const bool upFlag,
  const vector<Peak const *>& peakPtrsUsed,
  const unsigned iuKnown,
  const unsigned iuCand) const
{
  Peak const * pptr;
  pptr = PeakPartial::closePeak(smallFactor, upFlag, &Peak::goodQuality,
    peakPtrsUsed, iuKnown, iuCand);

  if (pptr)
    return pptr;

  return PeakPartial::closePeak(largeFactor, upFlag, &Peak::goodQuality,
    peakPtrsUsed, iuKnown, iuCand);
}


Peak const * PeakPartial::lookForPeak(
  const unsigned start,
  const unsigned step,
  const float& smallFactor,
  const float& largeFactor,
  const bool upFlag,
  vector<Peak const *>& peakPtrsUsed,
  unsigned& indexUsedOut)
{
  const unsigned smallStep = static_cast<unsigned>(smallFactor * step);
  const unsigned nextStart = (upFlag ? start + step : start - step);

  Peak const * pptr;
  pptr = PeakPartial::locatePeak(
    nextStart - smallStep, nextStart + smallStep, nextStart,
    &Peak::goodQuality, peakPtrsUsed, indexUsedOut);
  // This won't work, as peakPtrsUsed is a normal vector here.
  /*
  pptr = peakPtrsUsed.locate(
    nextStart - smallStep, nextStart + smallStep, nextStart,
    &Peak::goodQuality, indexUsedOut);
    */
    
  if (pptr)
    return pptr;

  const unsigned largeStep = static_cast<unsigned>(largeFactor * step);
  return PeakPartial::locatePeak(
    nextStart - largeStep, nextStart + largeStep, nextStart,
    &Peak::greatQuality, peakPtrsUsed, indexUsedOut);
}


void PeakPartial::getPeaks(
  const unsigned bogeyTypical,
  const unsigned longTypical,
  vector<Peak const *>& peakPtrsUsed,
  vector<Peak const *>& peakPtrsUnused)
{
  // Mostly for the sport, we try to use as many of the front peaks
  // as possible.  Mostly the alignment will pick up on it anyway.

  PeakPartial::getPeaksFromUsed(bogeyTypical, longTypical, 
    true, peakPtrsUsed);

  // if we didn't match a peak in the used vector, we move it to unused.
  PeakPartial::moveUnused(peakPtrsUsed, peakPtrsUnused);
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
  return (comps[peakNo].peak != nullptr);
}


bool PeakPartial::hasRange(const unsigned peakNo) const
{
  return (comps[peakNo].upper > 0);
}


string PeakPartial::strTarget(
  const unsigned peakNo,
  const unsigned offset) const
{
  return to_string(comps[peakNo].target + offset);
}


string PeakPartial::strIndex(
  const unsigned peakNo,
  const unsigned offset) const
{
  if (comps[peakNo].peak == nullptr)
  {
    if (comps[peakNo].lower == 0)
      return "-";
    else
      return "(" + to_string(comps[peakNo].lower + offset) + ", " +
        to_string(comps[peakNo].upper + offset) + ")";
  }
  else
    return to_string(comps[peakNo].peak->getIndex() + offset) +
      " (" + to_string(comps[peakNo].indexUsed) + ")";
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

