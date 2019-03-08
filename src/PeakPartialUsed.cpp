#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "PeakPartial.h"
#include "Peak.h"
#include "PeakPtrs.h"


void PeakPartial::recoverPeaks0001(
  const unsigned bogeyTypical,
  const unsigned longTypical,
  vector<Peak const *>& peakPtrsUsed)
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
  if (peakSlots.number() == 0 ||
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

  if (peakSlots.number() == 1 || iu == 1)
    return;

  // One possible pattern is: p0 - p1 - spurious - p2, to go with p3.
  unsigned iuNext;
  if (peakPtrsUsed[iu-2]->fantasticQuality())
    iuNext = iu-2;
  else if (peakSlots.number() >= 3 &&
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


void PeakPartial::recoverPeaks0011(vector<Peak const *>& peakPtrsUsed)
{
  // The four peaks: not found - not found - found - found.
  const unsigned iu = indexUsed[3];
  if (peakSlots.number() == 2 && 
      peakSlots.count(PEAKSLOT_LEFT_OF_P2) == 2 &&
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


void PeakPartial::recoverPeaks0101(vector<Peak const *>& peakPtrsUsed)
{
  // The four peaks: not found - found - not found - found.
  const unsigned iu = indexUsed[3];
  if (peakSlots.count(PEAKSLOT_BETWEEN_P1_P3) == 2 &&
      peakPtrsUsed[iu-2]->greatQuality() &&
      peakPtrsUsed[iu-1]->greatQuality())
  {
      // Probably we found p1 and p4, with these two in between.
cout << "Reinstating two peaks, should be p2 and p3\n";
    PeakPartial::movePtr(1, 0);

    PeakPartial::registerPtr(1, peakPtrsUsed[iu-2], iu-2);
    PeakPartial::registerPtr(2, peakPtrsUsed[iu-1], iu-1);
  }
  else if (peakSlots.count(PEAKSLOT_BETWEEN_P1_P3) == 1 &&
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


void PeakPartial::recoverPeaks1011(vector<Peak const *>& peakPtrsUsed)
{
  // The four peaks: found - not found - found - found.
  const unsigned bogey = peaks[3]->getIndex() - peaks[2]->getIndex();

  Peak const * pptr0 = nullptr, * pptr1 = nullptr;
  unsigned indexUsed0 = 0, indexUsed1 = 0;

  // Could be p0-null-p2-p3, missing the p1.
  const unsigned index0 = peaks[0]->getIndex();
  if (peakSlots.count(PEAKSLOT_BETWEEN_P0_P2) > 0)
    pptr1 = PeakPartial::lookForPeak(index0, bogey, 0.2f, 0.5f,
      true, peakPtrsUsed, indexUsed1);

  // Could be p1-null-p2-p3, missing the p0.
  if (peakSlots.count(PEAKSLOT_LEFT_OF_P0) > 0)
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


void PeakPartial::recoverPeaks1100(vector<Peak const *>& peakPtrsUsed)
{
  // The four peaks: found - found - not found - not found.
  const unsigned iu = indexUsed[0];
  if (peakSlots.number() == 2 && 
      peakSlots.count(PEAKSLOT_RIGHT_OF_P1) == 2 &&
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


void PeakPartial::recoverPeaks1101(vector<Peak const *>& peakPtrsUsed)
{
  // The four peaks: found - found - not found - found.
  const unsigned bogey = peaks[1]->getIndex() - peaks[0]->getIndex();

  Peak const * pptr2 = nullptr, * pptr3 = nullptr;
  unsigned indexUsed2 = 0, indexUsed3 = 0;

  // Could be p0-p1-null-p2, missing the p3.
  const unsigned index3 = peaks[3]->getIndex();
  if (peakSlots.count(PEAKSLOT_RIGHT_OF_P3) > 0)
    pptr3 = PeakPartial::lookForPeak(index3, bogey, 0.2f, 0.5f,
      true, peakPtrsUsed, indexUsed3);

  // Could be p0-p1-null-p3, missing the p2.
  if (peakSlots.count(PEAKSLOT_BETWEEN_P1_P3) > 0)
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


void PeakPartial::recoverPeaks1110(vector<Peak const *>& peakPtrsUsed)
{
  // The four peaks: found - found - found - not found.
  const unsigned bogey = peaks[1]->getIndex() - peaks[0]->getIndex();

  Peak const * pptr2 = nullptr, * pptr3 = nullptr;
  unsigned indexUsed2 = 0, indexUsed3 = 0;

  // Could be p0-p1-p2-null, missing the p3.
  const unsigned index2 = peaks[2]->getIndex();
  if (peakSlots.count(PEAKSLOT_RIGHT_OF_P2) > 0)
    pptr3 = PeakPartial::lookForPeak(index2, bogey, 0.2f, 0.5f,
      true, peakPtrsUsed, indexUsed3);

  // Could be p0-p1-p3-null, missing the p2.
  if (peakSlots.count(PEAKSLOT_BETWEEN_P1_P2) > 0)
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


void PeakPartial::recoverPeaksShared(
  Peak const *& pptrA,
  Peak const *& pptrB,
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


void PeakPartial::getPeaksFromUsed(
  const unsigned bogeyTypical,
  const unsigned longTypical,
  vector<Peak const *>& peakPtrsUsed)
{
  // Mostly for the sport, we try to use as many of the front peaks
  // as possible.  Mostly the alignment will pick up on it anyway.

  switch(peakSlots.code())
  {
    case 0b0001:
      PeakPartial::recoverPeaks0001(bogeyTypical, longTypical, peakPtrsUsed);
      break;
    case 0b0011:
      PeakPartial::recoverPeaks0011(peakPtrsUsed);
      break;
    case 0b0101:
      PeakPartial::recoverPeaks0101(peakPtrsUsed);
      break;
    case 0b0111:
      // We got the last three peaks.  Generally it's a pretty good bet
      // to throw away anything else.
      break;
    case 0b1011:
      PeakPartial::recoverPeaks1011(peakPtrsUsed);
      break;
    case 0b1100:
      PeakPartial::recoverPeaks1100(peakPtrsUsed);
      break;
    case 0b1101:
      PeakPartial::recoverPeaks1101(peakPtrsUsed);
      break;
    case 0b1110:
      PeakPartial::recoverPeaks1110(peakPtrsUsed);
      break;
    default:
      // Usually nothing left.
      break;
  }
}

