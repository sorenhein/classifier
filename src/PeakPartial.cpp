#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "PeakPartial.h"
#include "Peak.h"

// Used to count input peaks relative to used peaks.
#define LEFT_OF_P0 4
#define BETWEEN_P0_P1 3
#define BETWEEN_P1_P2 2
#define BETWEEN_P2_P3 1
#define RIGHT_OF_P3 0
#define LEFT_OF_P1 8
#define BETWEEN_P0_P2 7
#define BETWEEN_P1_P3 6
#define RIGHT_OF_P2 5
#define LEFT_OF_P3 9
#define LEFT_OF_P2 10
#define RIGHT_OF_P0 11
#define RIGHT_OF_P1 12
#define NUM_INTERVALS 13

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

  peaks.resize(4, nullptr);
  lower.resize(4, 0);
  upper.resize(4, 0);
  target.resize(4, 0);
  indexUsed.resize(4, 0);
  numUsed = 0;

  peakCode = 0;
  intervalEntries = 0;
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


void PeakPartial::movePtr(
  const unsigned indexFrom,
  const unsigned indexTo)
{
  peaks[indexTo] = peaks[indexFrom];
  indexUsed[indexTo] = indexUsed[indexFrom];

  peaks[indexFrom] = nullptr;
  indexUsed[indexFrom] = INDEX_NOT_USED;
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
  indexUsed[peakNo] = INDEX_NOT_USED;

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
  Peak * pptr,
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
  if (upper[peakNo] == 0)
    return false;

  lowerOut = lower[peakNo];
  upperOut = upper[peakNo];
  return true;
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
  // TODO Divide by 2 or not?
  const unsigned delta = (upper[posNo] - lower[posNo]) / 2;

  if (peakNo < posNo)
    return (lower[posNo] - index < delta);
  else if (peakNo > posNo)
    return (index - upper[posNo] < delta);
  else if (index < lower[posNo])
    return (lower[posNo] - index < delta);
  else if (index > upper[posNo])
    return (index - upper[posNo] < delta);
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


bool PeakPartial::samePeakValues(const PeakPartial& p2) const
{
  // Could be p1 - p2 - nullptr - p4 and p1 - p2 - p4 - nullptr.
  // If so, pick either one.
  vector<const Peak *> v1, v2;
  for (unsigned i = 0; i < 4; i++)
  {
    if (peaks[i] != p2.peaks[i])
    {
      v1.push_back(peaks[i]);
      v2.push_back(p2.peaks[i]);
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
    if (peaks[i] || p2.peaks[i])
      continue;

    if (p2.lower[i] != 0 && 
        (lower[i] == 0 || p2.lower[i] < lower[i]))
      lower[i] = p2.lower[i];

    if (p2.upper[i] != 0 && 
        (upper[i] == 0 || p2.upper[i] > upper[i]))
      upper[i] = p2.upper[i];
  }
}


bool PeakPartial::merge(const PeakPartial& p2)
{
  if (numUsed == 0 || numUsed == 4)
    return false;

  bool mergeFrom = true, mergeTo = true;
  for (unsigned i = 0; i < 4; i++ && mergeFrom && mergeTo)
  {
    if (peaks[i])
    {
      if (p2.peaks[i])
      {
        if (peaks[i] == p2.peaks[i])
          continue;
        else
          return false;
      }
      else if (! p2.closeEnough(peaks[i]->getIndex(), i, i))
        mergeTo = false;
    }
    else if (p2.peaks[i])
    {
      if (! PeakPartial::closeEnough(p2.peaks[i]->getIndex(), i, i))
        mergeFrom = false;
    }
  }

  if (mergeFrom || mergeTo)
  {
    for (unsigned i = 0; i < 4; i++)
    {
      if (! peaks[i] && p2.peaks[i])
      {
        peaks[i] = p2.peaks[i];
        indexUsed[i] = p2.indexUsed[i];
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
  const vector<Peak *>& peakPtrsUsed,
  const unsigned offset)
{
  peakCode =
    (peaks[0] ? 8 : 0) | 
    (peaks[1] ? 4 : 0) | 
    (peaks[2] ? 2 : 0) | 
    (peaks[3] ? 1 : 0);

  //      p0      p1      p2      p3
  //   i4 >|< i3 >|<  i2 >|<  i1 >|<  i0
  //                      |<  i5
  //              |<      i6     >|
  //       |<     i7     >|
  //       i8    >|

  const unsigned p0 = (peaks[0] ? peaks[0]->getIndex() : 0);
  const unsigned p1 = (peaks[1] ? peaks[1]->getIndex() : 0);
  const unsigned p2 = (peaks[2] ? peaks[2]->getIndex() : 0);
  const unsigned p3 = (peaks[3] ? peaks[3]->getIndex() : 0);

  intervalEntries = 0;
  intervalCount.clear();
  intervalCount.resize(NUM_INTERVALS);

  for (auto& p: peakPtrsUsed)
  {
    if (p == peaks[0] || p == peaks[1] || p == peaks[2] || p == peaks[3])
      continue;

    intervalEntries++;
    const unsigned index = p->getIndex();

    if (peaks[0] && index <= p0)
      intervalCount[LEFT_OF_P0]++;
    else if (peaks[3] && index > p3)
      intervalCount[RIGHT_OF_P3]++;

    else if (peaks[0] && peaks[1] && index <= p1)
      intervalCount[BETWEEN_P0_P1]++;
    else if (peaks[1] && peaks[2] && index > p1 && index <= p2)
      intervalCount[BETWEEN_P1_P2]++;
    else if (peaks[2] && peaks[3] && index > p2 && index <= p3)
      intervalCount[BETWEEN_P2_P3]++;

    else if (peaks[1] && index <= p1)
      intervalCount[LEFT_OF_P1]++;
    else if (peaks[2] && index > p2)
      intervalCount[RIGHT_OF_P2]++;

    else if (peaks[0] && peaks[2] && index > p0 && index <= p2)
      intervalCount[BETWEEN_P0_P2]++;
    else if (peaks[1] && peaks[3] && index > p1 && index <= p3)
      intervalCount[BETWEEN_P1_P3]++;

    else if (! peaks[0] && ! peaks[1])
    {
      if (! peaks[2] && index <= p3)
        intervalCount[LEFT_OF_P3]++;
      else if (peaks[2] && index <= p2)
        intervalCount[LEFT_OF_P2]++;
      else
        cout << "PEAKPARTIALERROR1 " << index + offset << "\n";
    }
    else if (! peaks[2] && ! peaks[3])
    {
      if (! peaks[1] && index > p0)
        intervalCount[RIGHT_OF_P0]++;
      else if (peaks[1] && index > p1)
        intervalCount[RIGHT_OF_P1]++;
      else
        cout << "PEAKPARTIALERROR2 " << index + offset << "\n";
    }
    else
      cout << "PEAKPARTIALERROR3 " << index + offset << "\n";
  }
}


void PeakPartial::printSituation(const bool firstFlag) const
{
  if (! intervalEntries)
    return;

  cout << (firstFlag ? "PEAKFIRST " : "PEAKLAST  ") << 
    setw(2) << peakCode <<
    setw(4) << PeakPartial::strEntry(peaks[0] == nullptr ? 0 : 1) <<
    setw(2) << PeakPartial::strEntry(peaks[1] == nullptr ? 0 : 1) <<
    setw(2) << PeakPartial::strEntry(peaks[2] == nullptr ? 0 : 1) <<
    setw(2) << PeakPartial::strEntry(peaks[3] == nullptr ? 0 : 1) <<

    setw(4) << PeakPartial::strEntry(intervalCount[LEFT_OF_P0]) <<
    setw(2) << PeakPartial::strEntry(intervalCount[BETWEEN_P0_P1]) <<
    setw(2) << PeakPartial::strEntry(intervalCount[BETWEEN_P1_P2]) <<
    setw(2) << PeakPartial::strEntry(intervalCount[BETWEEN_P2_P3]) <<
    setw(2) << PeakPartial::strEntry(intervalCount[RIGHT_OF_P3]) <<
    setw(4) << PeakPartial::strEntry(intervalCount[LEFT_OF_P1]) <<
    setw(2) << PeakPartial::strEntry(intervalCount[BETWEEN_P0_P2]) <<
    setw(2) << PeakPartial::strEntry(intervalCount[BETWEEN_P1_P3]) <<
    setw(2) << PeakPartial::strEntry(intervalCount[RIGHT_OF_P2]) << 
    setw(3) << PeakPartial::strEntry(intervalCount[LEFT_OF_P3]) <<
    setw(2) << PeakPartial::strEntry(intervalCount[LEFT_OF_P2]) << 
    setw(2) << PeakPartial::strEntry(intervalCount[RIGHT_OF_P1]) << 
    setw(2) << PeakPartial::strEntry(intervalCount[RIGHT_OF_P0]) << 
    endl;
}


void PeakPartial::moveUnused(
  vector<Peak *>& peakPtrsUsed,
  vector<Peak *>& peakPtrsUnused) const
{
  const unsigned np = peakPtrsUsed.size();
  vector<unsigned> v(np, 0);
  for (unsigned i = 0; i < 4; i++)
  {
    if (peaks[i] && indexUsed[i] != INDEX_NOT_USED)
      v[indexUsed[i]] = 1;
  }

  // Don't remove peaks in front of the car -- could be a new car.
  const unsigned preserveLimit = (peaks[0] ? peaks[0]->getIndex() : 0);

  for (unsigned i = 0; i < np; i++)
  {
    if (v[i] == 0 && peakPtrsUsed[i]->getIndex() >= preserveLimit)
      peakPtrsUnused.push_back(peakPtrsUsed[i]);
  }

  peakPtrsUsed = peaks;
}


void PeakPartial::recoverPeaks0101(vector<Peak *>& peakPtrsUsed)
{
  // The four peaks: not found - found - not found - found.
  const unsigned iu = indexUsed[3];
  if (intervalCount[BETWEEN_P1_P3] == 2 &&
      peakPtrsUsed[iu-2]->greatQuality() &&
      peakPtrsUsed[iu-1]->greatQuality())
  {
      // Probably we found p1 and p4, with these two in between.
cout << "Reinstating two peaks, should be p2 and p3\n";
    PeakPartial::movePtr(1, 0);

    PeakPartial::registerPtr(1, peakPtrsUsed[iu-2], iu-2);
    PeakPartial::registerPtr(2, peakPtrsUsed[iu-1], iu-1);
  }
  else if (intervalCount[BETWEEN_P1_P3] == 1 &&
      peakPtrsUsed[iu-1]->greatQuality())
  {
    if (peaks[3]->getIndex() - peakPtrsUsed[iu-1]->getIndex() <
        peakPtrsUsed[iu-1]->getIndex() - peaks[1]->getIndex())
    {
      // Closer to p3.
cout << "Reinstating one peak, should be p2\n";
      PeakPartial::registerPtr(2, peakPtrsUsed[iu-1], iu-1);
    }
    else
    {
      // Assume that the peaks were really p0 and p2.
cout << "Reinstating one peak, should be p1\n";
      PeakPartial::movePtr(1, 0);
      PeakPartial::movePtr(3, 2);

      PeakPartial::registerPtr(1, peakPtrsUsed[iu-1], iu-1);
    }
  }
}


void PeakPartial::recoverPeaks0011(vector<Peak *>& peakPtrsUsed)
{
  // The four peaks: not found - not found - found - found.
  const unsigned iu = indexUsed[3];
  if (intervalEntries == 2 && 
      intervalCount[LEFT_OF_P2] == 2 &&
      upper[1] == 0 &&
      peakPtrsUsed[iu-3]->greatQuality() &&
      peakPtrsUsed[iu-2]->greatQuality())
  {
    // Could also check that the spacing of the two peaks is not
    // completely off vs. the two last peaks.
cout << "Reinstating two peaks, should be p0 and p1\n";
    PeakPartial::registerPtr(0, peakPtrsUsed[iu-3], iu-3);
    PeakPartial::registerPtr(1, peakPtrsUsed[iu-2], iu-2);
  }
}


void PeakPartial::recoverPeaks1100(vector<Peak *>& peakPtrsUsed)
{
  // The four peaks: found - found - not found - not found.
  const unsigned iu = indexUsed[0];
  if (intervalEntries == 2 && 
      intervalCount[RIGHT_OF_P1] == 2 &&
      upper[2] == 0 &&
      peakPtrsUsed[iu+2]->greatQuality() &&
      peakPtrsUsed[iu+3]->greatQuality())
  {
    // Could also check that the spacing of the two peaks is not
    // completely off vs. the two last peaks.
cout << "Reinstating two peaks, should be p2 and p3\n";
    PeakPartial::registerPtr(2, peakPtrsUsed[iu+2], iu+2);
    PeakPartial::registerPtr(3, peakPtrsUsed[iu+3], iu+3);
  }
}


void PeakPartial::recoverPeaks0001(
  const unsigned bogeyTypical,
  const unsigned longTypical,
  vector<Peak *>& peakPtrsUsed)
{
  // The four peaks: not found - not found - not found - found.
  // This has the most varied set of misses and reasons.
  const unsigned iu = indexUsed[3];
  if (iu == 0)
    return;

  const unsigned index = peakPtrsUsed[iu-1]->getIndex();

  // 1.2x is a normal range for short cars, plus a bit of buffer.
  const unsigned bogey = static_cast<unsigned>(1.3f * bogeyTypical);

  // Even a short car has a certain middle length.
  const unsigned mid = static_cast<unsigned>(0.4f * longTypical);
  
  // Give up if we have nothing to work with.
  if (intervalEntries == 0 ||
      index >= peaks[3]->getIndex() || 
      upper[2] == 0)
    return;

  // Give up if the rightmost candidate is not plausible.
  // So if index = 590 and range = 600-610, it would just work.
  // Effectively we double up the interval.
  // If this is a new car type, we have higher quality demands.
  if ((index + upper[2] < 2*lower[2] ||
      ! peakPtrsUsed[iu-1]->greatQuality()) &&
      (index + bogey < peaks[3]->getIndex() ||
      ! peakPtrsUsed[iu-1]->fantasticQuality()))
    return;

  // Complete the right bogey.
cout << "Reinstating one 0001 peak, should be p2\n";
  PeakPartial::registerPtr(2, peakPtrsUsed[iu-1], iu-1);

  if (intervalEntries == 1)
    return;

  // One possible pattern is: p0 - p1 - spurious - p2, to go with p3.
  unsigned iuNext;
  if (peakPtrsUsed[iu-2]->fantasticQuality())
    iuNext = iu-2;
  else if (intervalEntries >= 3 &&
      peakPtrsUsed[iu-3]->fantasticQuality())
    iuNext = iu-3;
  else
    return;

cout << "Trying to reinstate p1: iuNext " << iuNext << ", index " <<
  index << ", nextptr " << peakPtrsUsed[iuNext]->getIndex() << endl;
  if (index - peakPtrsUsed[iuNext]->getIndex() >= mid)
  {
    // Take a stab at the first wheel of the left bogey, too.
    // Theoretically there should be an upper limit too.
cout << "Reinstating one 0001 peak, should be p1\n";
    PeakPartial::registerPtr(1, peakPtrsUsed[iuNext], iuNext);
  }
  else
    return;

  if (iuNext == 0)
    return;

  if (! peakPtrsUsed[iuNext-1]->fantasticQuality())
    return;

  const unsigned d = peaks[3]->getIndex() - peaks[2]->getIndex();
  const unsigned lo = static_cast<unsigned>(0.9f * d);
  const unsigned hi = static_cast<unsigned>(1.1f * d);
  const unsigned limitLo = peaks[1]->getIndex() - hi;
  const unsigned limitHi = peaks[1]->getIndex() - lo;
  if (peakPtrsUsed[iuNext-1]->getIndex() >= limitLo &&
      peakPtrsUsed[iuNext-1]->getIndex() <= limitHi)
  {
    PeakPartial::registerPtr(0, peakPtrsUsed[iuNext-1], iuNext-1);
cout << "Reinstating one 0001 peak, should be p0\n";
  }
}


// TODO TMP: Should become a class (PeakPtrVector).  Then eliminate
// duplication with PeakRepair.
Peak * PeakPartial::locatePeak(
  const unsigned lowerIn,
  const unsigned upperIn,
  const unsigned hint,
  const PeakFncPtr& fptr,
  PeakPtrVector& peakPtrsUsed,
  unsigned& indexUsedOut) const
{
  unsigned num = 0;
  unsigned i = 0;
  unsigned dist = numeric_limits<unsigned>::max();
  Peak * ptr = nullptr;
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


void PeakPartial::recoverPeaksShared(
  Peak *& pptrA,
  Peak *& pptrB,
  const unsigned npA,
  const unsigned npB,
  const unsigned indexUsedA,
  const unsigned indexUsedB,
  const string& source)
{
  if (pptrA && pptrB)
  {
    cout << "ERROR recoverPeaks1110: pA and pB are both there?\n";
    cout << "Without offset:\n";
    cout << pptrA->strQuality();
    cout << pptrB->strQuality();
    return;
  }

  if (pptrA)
  {
cout << "Reinstating one " << source << " peak, should be pA\n";
    if (peaks[npA])
      PeakPartial::movePtr(npA, npB);
    PeakPartial::registerPtr(npA, pptrA, indexUsedA);
  }
  else if (pptrB)
  {
cout << "Reinstating one " << source << " peak, should be pB\n";
    if (peaks[npB])
      PeakPartial::movePtr(npB, npA);
    PeakPartial::registerPtr(npB, pptrB, indexUsedB);
  }
}


Peak * PeakPartial::lookForPeak(
  const unsigned start,
  const unsigned step,
  const float& smallFactor,
  const float& largeFactor,
  const bool upFlag,
  vector<Peak *>& peakPtrsUsed,
  unsigned& indexUsedOut)
{
  const unsigned smallStep = static_cast<unsigned>(smallFactor * step);
  const unsigned nextStart = (upFlag ? start + step : start - step);

  Peak * pptr;
  pptr = PeakPartial::locatePeak(
    nextStart - smallStep, nextStart + smallStep, nextStart,
    &Peak::goodQuality, peakPtrsUsed, indexUsedOut);
    
  if (pptr)
    return pptr;

  const unsigned largeStep = static_cast<unsigned>(largeFactor * step);
  return PeakPartial::locatePeak(
    nextStart - largeStep, nextStart + largeStep, nextStart,
    &Peak::greatQuality, peakPtrsUsed, indexUsedOut);
}


void PeakPartial::recoverPeaks1011(vector<Peak *>& peakPtrsUsed)
{
  // The four peaks: found - not found - found - found.
  const unsigned bogey = peaks[3]->getIndex() - peaks[2]->getIndex();

  Peak * pptr0 = nullptr, * pptr1 = nullptr;
  unsigned indexUsed0 = 0, indexUsed1 = 0;

  // Could be p0-null-p2-p3, missing the p1.
  const unsigned index0 = peaks[0]->getIndex();
  if (intervalCount[BETWEEN_P0_P2] > 0)
    pptr1 = PeakPartial::lookForPeak(index0, bogey, 0.2f, 0.5f,
      true, peakPtrsUsed, indexUsed1);

  // Could be p1-null-p2-p3, missing the p0.
  if (intervalCount[LEFT_OF_P0] > 0)
    pptr0 = PeakPartial::lookForPeak(index0, bogey, 0.2f, 0.5f,
      false, peakPtrsUsed, indexUsed0);

  if (pptr1)
  {
    const unsigned pp1 = pptr1->getIndex();
    if (pp1 >= peaks[2]->getIndex() ||
        pp1 - peaks[0]->getIndex() > peaks[2]->getIndex() - pp1)
    {
      // The putative p1 would be badly positioned to p2.
      pptr1 = nullptr;
    }
  }

  PeakPartial::recoverPeaksShared(pptr0, pptr1, 0, 1,
    indexUsed0, indexUsed1, "1011");
}


void PeakPartial::recoverPeaks1101(vector<Peak *>& peakPtrsUsed)
{
  // The four peaks: found - found - not found - found.
  const unsigned bogey = peaks[1]->getIndex() - peaks[0]->getIndex();

  Peak * pptr2 = nullptr, * pptr3 = nullptr;
  unsigned indexUsed2 = 0, indexUsed3 = 0;

  // Could be p0-p1-null-p2, missing the p3.
  const unsigned index3 = peaks[3]->getIndex();
  if (intervalCount[RIGHT_OF_P3] > 0)
    pptr3 = PeakPartial::lookForPeak(index3, bogey, 0.2f, 0.5f,
      true, peakPtrsUsed, indexUsed3);

  // Could be p0-p1-null-p3, missing the p2.
  if (intervalCount[BETWEEN_P1_P3] > 0)
    pptr2 = PeakPartial::lookForPeak(index3, bogey, 0.2f, 0.5f,
      false, peakPtrsUsed, indexUsed2);

  if (pptr2)
  {
    const unsigned pp2 = pptr2->getIndex();
    if (pp2 <= peaks[1]->getIndex() ||
        pp2 - peaks[1]->getIndex() < peaks[3]->getIndex() - pp2)
    {
      // The putative p2 would be badly positioned to p1.
      pptr2 = nullptr;
    }
  }

  PeakPartial::recoverPeaksShared(pptr2, pptr3, 2, 3,
    indexUsed2, indexUsed3, "1101");
}


void PeakPartial::recoverPeaks1110(vector<Peak *>& peakPtrsUsed)
{
  // The four peaks: found - found - found - not found.
  const unsigned bogey = peaks[1]->getIndex() - peaks[0]->getIndex();

  Peak * pptr2 = nullptr, * pptr3 = nullptr;
  unsigned indexUsed2 = 0, indexUsed3 = 0;

  // Could be p0-p1-p2-null, missing the p3.
  const unsigned index2 = peaks[2]->getIndex();
  if (intervalCount[RIGHT_OF_P2] > 0)
    pptr3 = PeakPartial::lookForPeak(index2, bogey, 0.2f, 0.5f,
      true, peakPtrsUsed, indexUsed3);

  // Could be p0-p1-p3-null, missing the p2.
  if (intervalCount[BETWEEN_P1_P2] > 0)
    pptr2 = PeakPartial::lookForPeak(index2, bogey, 0.2f, 0.5f,
      false, peakPtrsUsed, indexUsed2);

  if (pptr2)
  {
    const unsigned pp2 = pptr2->getIndex();
    if (pp2 <= peaks[1]->getIndex() ||
        pp2 - peaks[1]->getIndex() < peaks[2]->getIndex() - pp2)
    {
      // The putative p2 would be badly positioned to p1.
      pptr2 = nullptr;
    }
  }
  PeakPartial::recoverPeaksShared(pptr2, pptr3, 2, 3,
    indexUsed2, indexUsed3, "1110");
}


void PeakPartial::getPeaks(
  const unsigned bogeyTypical,
  const unsigned longTypical,
  vector<Peak *>& peakPtrsUsed,
  vector<Peak *>& peakPtrsUnused)
{
  // Mostly for the sport, we try to use as many of the front peaks
  // as possible.  Mostly the alignment will pick up on it anyway.

  if (peakCode == 0b0001)
    PeakPartial::recoverPeaks0001(bogeyTypical, longTypical, peakPtrsUsed);
  else if (peakCode == 0b0011)
    PeakPartial::recoverPeaks0011(peakPtrsUsed);
  else if (peakCode == 0xb0101)
    PeakPartial::recoverPeaks0101(peakPtrsUsed);
  else if (peakCode == 0b0111)
  {
    // We got the last three peaks.  Generally it's a pretty good bet
    // to throw away anything else.
  }
  else if (peakCode == 0b1100)
    PeakPartial::recoverPeaks1100(peakPtrsUsed);
  else if (peakCode == 0b1011)
    PeakPartial::recoverPeaks1011(peakPtrsUsed);
  else if (peakCode == 0b1101)
    PeakPartial::recoverPeaks1101(peakPtrsUsed);
  else if (peakCode == 0b1110)
    PeakPartial::recoverPeaks1110(peakPtrsUsed);
  else
  {
    // Usually nothing left.
  }

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

