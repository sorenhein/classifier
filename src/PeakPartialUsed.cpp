#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "PeakPartial.h"
#include "Peak.h"
#include "PeakPtrs.h"


// 1.2x is a normal range for short cars, plus a bit of buffer.
#define BOGEY_GENERAL_FACTOR 0.3f

// When we're comparing within a given car, we are stricter.
#define BOGEY_SPECIFIC_FACTOR 0.1f

// Even a short car has a certain middle length.
#define MID_FACTOR 0.4f


void PeakPartial::recoverPeaks0001(vector<Peak const *>& peakPtrsUsed)
{
  pstats[0b0001].num++;

  const unsigned iu = comps[3].indexUsed;
  if (iu == 0)
    return;

  const unsigned index = peakPtrsUsed[iu-1]->getIndex();
  
  // Give up if we have nothing to work with.

  if (peakSlots.number() == 0 ||
      index >= comps[3].peak->getIndex() || 
      comps[2].upper == 0)
    return;

  // Give up if the rightmost candidate is not plausible.
  // So if index = 590 and range = 600-610, it would just work.
  // Effectively we double up the interval.
  // If this is a new car type, we have higher quality demands.

  if ((index + comps[2].upper < 2 * comps[2].lower ||
      ! peakPtrsUsed[iu-1]->greatQuality()) &&
      (index + bogeyHi < comps[3].peak->getIndex() ||
      ! peakPtrsUsed[iu-1]->fantasticQuality()))
    return;

  // Complete the right bogey.
  
  if (verboseFlag)
    cout << "Reinstating one 0001 peak, should be p2\n";

  PeakPartial::registerPtr(2, peakPtrsUsed[iu-1], iu-1);
  pstats[0b0001].hits++;

  PeakPartial::recoverPeaks0011(peakPtrsUsed);
}


void PeakPartial::recoverPeaks0011(vector<Peak const *>& peakPtrsUsed)
{
  // One of the two previous peaks should be fantastic.  There could
  // be a spurious peak first.

  pstats[0b0011].num++;

  const unsigned iu = comps[2].indexUsed;
  if (iu == 0)
    return;

  unsigned iuNext;
  if (peakPtrsUsed[iu-1]->fantasticQuality())
    iuNext = iu-1;
  else if (peakSlots.number() >= 3 && peakPtrsUsed[iu-2]->fantasticQuality())
    iuNext = iu-2;
  else
    return;

  const unsigned index = peakPtrsUsed[iu]->getIndex();

  if (index - peakPtrsUsed[iuNext]->getIndex() < mid)
    return;

  // Take a stab at the first wheel of the left bogey, too.
  // Theoretically there should be an upper limit too.

  if (verboseFlag)
    cout << "Reinstating one 0011 peak, should be p1\n";

  PeakPartial::registerPtr(1, peakPtrsUsed[iuNext], iuNext);
  pstats[0b0011].hits++;

  PeakPartial::recoverPeaks0111(peakPtrsUsed);
}


void PeakPartial::recoverPeaks0101(vector<Peak const *>& peakPtrsUsed)
{
  pstats[0b0101].num++;

  const unsigned iu = comps[3].indexUsed;

  if (peakSlots.count(PEAKSLOT_BETWEEN_P1_P3) == 2 &&
      peakPtrsUsed[iu-2]->greatQuality() &&
      peakPtrsUsed[iu-1]->greatQuality())
  {
    if (verboseFlag)
      cout << "Reinstating two 0101 peaks, should be p1+p2 (between)\n";

    PeakPartial::movePtr(1, 0);

    PeakPartial::registerPtr(1, peakPtrsUsed[iu-2], iu-2);
    PeakPartial::registerPtr(2, peakPtrsUsed[iu-1], iu-1);
    pstats[0b0101].hits++;
  }
  else if (peakSlots.count(PEAKSLOT_BETWEEN_P1_P3) == 1 &&
      peakPtrsUsed[iu-1]->greatQuality())
  {
    if (comps[3].peak->getIndex() - peakPtrsUsed[iu-1]->getIndex() <
        peakPtrsUsed[iu-1]->getIndex() - comps[1].peak->getIndex())
    {
      // Closer to p3.
      
      if (verboseFlag)
        cout << "Reinstating one 0101 peak, should be p2\n";

      PeakPartial::registerPtr(2, peakPtrsUsed[iu-1], iu-1);
      pstats[0b0101].hits++;

      // TODO Call 0111
    }
    else
    {
      // Assume that the peaks were really p0 and p2.

      if (verboseFlag)
        cout << "Reinstating one 0101 peak, should be (p0) p1 (p2)\n";

      PeakPartial::movePtr(1, 0);
      PeakPartial::movePtr(3, 2);

      PeakPartial::registerPtr(1, peakPtrsUsed[iu-1], iu-1);
      pstats[0b0101].hits++;

      // TODO Call 1110
    }
  }
}


void PeakPartial::recoverPeaks0111(vector<Peak const *>& peakPtrsUsed)
{
  pstats[0b0111].num++;

  const unsigned iu = comps[1].indexUsed;
  if (iu == 0)
    return;

  if (! peakPtrsUsed[iu-1]->fantasticQuality())
    return;

  const unsigned d = comps[3].peak->getIndex() - comps[2].peak->getIndex();

  const unsigned lo = 
    static_cast<unsigned>((1.0f - BOGEY_SPECIFIC_FACTOR) * d);

  const unsigned hi = 
    static_cast<unsigned>((1.0f + BOGEY_SPECIFIC_FACTOR) * d);

  const unsigned limitLo = comps[1].peak->getIndex() - hi;
  const unsigned limitHi = comps[1].peak->getIndex() - lo;

  if (peakPtrsUsed[iu-1]->getIndex() >= limitLo &&
      peakPtrsUsed[iu-1]->getIndex() <= limitHi)
  {
    if (verboseFlag)
      cout << "Reinstating one 0111 peak, should be p0\n";

    PeakPartial::registerPtr(0, peakPtrsUsed[iu-1], iu-1);
    pstats[0b0111].hits++;
  }
}


void PeakPartial::recoverPeaks1010(vector<Peak const *>& peakPtrsUsed)
{
  pstats[0b1010].num++;

  // Start from 3+ peaks.
  if (peakPtrsUsed.size() <= 2)
    return;

  // Take a simple look for a pair that looks like a bogey.
  const unsigned iu0 = comps[0].indexUsed;
  const unsigned iu2 = comps[2].indexUsed;

  Peak const * pptr;
  unsigned indexUsed = 0;

  // Look between the peaks.
  Peak const * ptmp;
  unsigned num01 = 0, num12 = 0;
  for (unsigned iu = iu0+1; iu < iu2; iu++)
  {
    // Look forward from p0.
    ptmp = PeakPartial::closePeak(0.3f, 0.5f, true, peakPtrsUsed, iu0, iu);

    if (ptmp)
    {
      num01++;
      pptr = ptmp;
      indexUsed = iu;
    }

    // Look backward from p2.
    ptmp = PeakPartial::closePeak(0.3f, 0.5f, false, peakPtrsUsed, iu2, iu);

    if (ptmp)
    {
      num12++;
      pptr = ptmp;
      indexUsed = iu;
    }
  }

  if (num01 + num12 > 1)
  {
    cout << "recoverPeaks1010: " << 
      num01 << " + " << num12 << " options\n";
    return;
  }

  if (num01)
  {
    if (verboseFlag)
      cout << "Reinstating one 1010 peak, should be p1\n";

    PeakPartial::registerPtr(1, peakPtrsUsed[indexUsed], indexUsed);
    pstats[0b1010].hits++;

    // TODO Call 1110
  }
  else if (num12)
  {
    if (verboseFlag)
      cout << "Reinstating one 1010 peak, should be p2(p3)\n";

    PeakPartial::movePtr(2, 3);
    PeakPartial::registerPtr(2, peakPtrsUsed[indexUsed], indexUsed);
    pstats[0b1010].hits++;

    // TODO Call 1011
  }
  else
  {
    unsigned num23 = 0;

    for (unsigned iu = iu2+1; iu < peakPtrsUsed.size(); iu++)
    {
      // Look forward from p2.
      ptmp = PeakPartial::closePeak(0.3f, 0.5f, true, peakPtrsUsed, 
          iu2, iu);

      if (ptmp)
      {
        num23++;
        pptr = ptmp;
        indexUsed = iu;
      }
    }

    if (num23 > 1)
    {
      cout << "recoverPeaks1010: " << num23 << " options\n";
      return;
    }
    else if (num23)
    {
      if (verboseFlag)
        cout << "Reinstating one 1010 peak, should be p3\n";

      PeakPartial::registerPtr(3, peakPtrsUsed[indexUsed], indexUsed);
      pstats[0b1010].hits++;
  
      // TODO Call 1011
    }

  }
}


void PeakPartial::recoverPeaks1011(vector<Peak const *>& peakPtrsUsed)
{
  pstats[0b1011].num++;

  const unsigned bogeySpecific = 
    comps[3].peak->getIndex() - comps[2].peak->getIndex();

  Peak const * pptr0 = nullptr, * pptr1 = nullptr;
  unsigned indexUsed0 = 0, indexUsed1 = 0;

  // Could be p0-null-p2-p3, missing the p1.
  const unsigned index0 = comps[0].peak->getIndex();

  if (peakSlots.count(PEAKSLOT_BETWEEN_P0_P2) > 0)
    pptr1 = PeakPartial::lookForPeak(index0, bogeySpecific, 0.2f, 0.5f,
      true, peakPtrsUsed, indexUsed1);

  // Could be p1-null-p2-p3, missing the p0.
  if (peakSlots.count(PEAKSLOT_LEFT_OF_P0) > 0)
    pptr0 = PeakPartial::lookForPeak(index0, bogeySpecific, 0.2f, 0.5f,
      false, peakPtrsUsed, indexUsed0);

  if (pptr1)
  {
    const unsigned pp1 = pptr1->getIndex();

    if (pp1 >= comps[2].peak->getIndex() ||
        pp1 - comps[0].peak->getIndex() > comps[2].peak->getIndex() - pp1)
    {
      // The putative p1 would be badly positioned to p2.
      pptr1 = nullptr;
    }
  }

  PeakPartial::recoverPeaksShared(pptr0, pptr1, 0, 1,
    indexUsed0, indexUsed1, "1011", 0b1011);
}


void PeakPartial::recoverPeaks1100(vector<Peak const *>& peakPtrsUsed)
{
  pstats[0b1100].num++;

  const unsigned iu = comps[0].indexUsed;

  if (peakSlots.number() == 2 && 
      peakSlots.count(PEAKSLOT_RIGHT_OF_P1) == 2 &&
      comps[2].upper == 0 &&
      peakPtrsUsed[iu+2]->greatQuality() &&
      peakPtrsUsed[iu+3]->greatQuality())
  {
    // Could also check that the spacing of the two peaks is not
    // completely off vs. the two last peaks.
    
    if (verboseFlag)
      cout << "Reinstating two 1100 peaks, should be p2+p3\n";

    PeakPartial::registerPtr(2, peakPtrsUsed[iu+2], iu+2);
    PeakPartial::registerPtr(3, peakPtrsUsed[iu+3], iu+3);
    pstats[0b1100].hits++;
  }

  // TODO Could also be a single peak, I suppose
}


void PeakPartial::recoverPeaks1101(vector<Peak const *>& peakPtrsUsed)
{
  pstats[0b1101].num++;

  const unsigned bogeySpecific = 
    comps[1].peak->getIndex() - comps[0].peak->getIndex();

  Peak const * pptr2 = nullptr, * pptr3 = nullptr;
  unsigned indexUsed2 = 0, indexUsed3 = 0;

  // Could be p0-p1-null-p2, missing the p3.
  const unsigned index3 = comps[3].peak->getIndex();
  if (peakSlots.count(PEAKSLOT_RIGHT_OF_P3) > 0)
    pptr3 = PeakPartial::lookForPeak(index3, bogeySpecific, 0.2f, 0.5f,
      true, peakPtrsUsed, indexUsed3);

  // Could be p0-p1-null-p3, missing the p2.
  if (peakSlots.count(PEAKSLOT_BETWEEN_P1_P3) > 0)
    pptr2 = PeakPartial::lookForPeak(index3, bogeySpecific, 0.2f, 0.5f,
      false, peakPtrsUsed, indexUsed2);

  if (pptr2)
  {
    const unsigned pp2 = pptr2->getIndex();
    if (pp2 <= comps[1].peak->getIndex() ||
        pp2 - comps[1].peak->getIndex() < comps[3].peak->getIndex() - pp2)
    {
      // The putative p2 would be badly positioned to p1.
      pptr2 = nullptr;
    }
  }

  PeakPartial::recoverPeaksShared(pptr2, pptr3, 2, 3,
    indexUsed2, indexUsed3, "1101", 0b1101);
}


void PeakPartial::recoverPeaks1110(vector<Peak const *>& peakPtrsUsed)
{
  pstats[0b1110].num++;

  const unsigned bogeySpecific = 
    comps[1].peak->getIndex() - comps[0].peak->getIndex();

  Peak const * pptr2 = nullptr, * pptr3 = nullptr;
  unsigned indexUsed2 = 0, indexUsed3 = 0;

  // Could be p0-p1-p2-null, missing the p3.
  const unsigned index2 = comps[2].peak->getIndex();

  if (peakSlots.count(PEAKSLOT_RIGHT_OF_P2) > 0)
    pptr3 = PeakPartial::lookForPeak(index2, bogeySpecific, 0.2f, 0.5f,
      true, peakPtrsUsed, indexUsed3);

  // Could be p0-p1-p3-null, missing the p2.
  if (peakSlots.count(PEAKSLOT_BETWEEN_P1_P2) > 0)
    pptr2 = PeakPartial::lookForPeak(index2, bogeySpecific, 0.2f, 0.5f,
      false, peakPtrsUsed, indexUsed2);

  if (pptr2)
  {
    const unsigned pp2 = pptr2->getIndex();
    if (pp2 <= comps[1].peak->getIndex() ||
        pp2 - comps[1].peak->getIndex() < comps[2].peak->getIndex() - pp2)
    {
      // The putative p2 would be badly positioned to p1.
      pptr2 = nullptr;
    }
  }

  PeakPartial::recoverPeaksShared(pptr2, pptr3, 2, 3,
    indexUsed2, indexUsed3, "1110", 0b1110);
}


void PeakPartial::recoverPeaksShared(
  Peak const *& pptrA,
  Peak const *& pptrB,
  const unsigned npA,
  const unsigned npB,
  const unsigned indexUsedA,
  const unsigned indexUsedB,
  const string& source,
  const unsigned bits)
{
  if (pptrA && pptrB)
  {
    cout << "ERROR recoverPeaks " << source << 
      ": pA and pB are both there?\n";
    cout << "Without offset:\n";
    cout << pptrA->strQuality();
    cout << pptrB->strQuality();
    return;
  }

  if (pptrA)
  {
    if (verboseFlag)
      cout << "Reinstating one " << source << " peak, should be pA\n";

    if (comps[npA].peak)
      PeakPartial::movePtr(npA, npB);
    PeakPartial::registerPtr(npA, pptrA, indexUsedA);
    pstats[bits].hits++;
  }
  else if (pptrB)
  {
    if (verboseFlag)
      cout << "Reinstating one " << source << " peak, should be pB\n";

    if (comps[npB].peak)
      PeakPartial::movePtr(npB, npA);
    PeakPartial::registerPtr(npB, pptrB, indexUsedB);
    pstats[bits].hits++;
  }
}


void PeakPartial::getPeaksFromUsed(
  const unsigned bogeyTypical,
  const unsigned longTypical,
  const bool verboseFlagIn,
  vector<Peak const *>& peakPtrsUsed)
{
  // Mostly for the sport, we try to use as many of the front peaks
  // as possible.  Mostly the alignment will pick up on it anyway.

  bogey = bogeyTypical;
  bogeyLo = 
    static_cast<unsigned>((1.f - BOGEY_GENERAL_FACTOR) * bogeyTypical);
  bogeyHi = 
    static_cast<unsigned>((1.f + BOGEY_GENERAL_FACTOR) * bogeyTypical);

  mid = static_cast<unsigned>(MID_FACTOR * longTypical);
  verboseFlag = verboseFlagIn;

  switch(peakSlots.code())
  {
    case 0b0001:
      PeakPartial::recoverPeaks0001(peakPtrsUsed);
      break;
    case 0b0011:
      PeakPartial::recoverPeaks0011(peakPtrsUsed);
      break;
    case 0b0101:
      PeakPartial::recoverPeaks0101(peakPtrsUsed);
      break;
    case 0b0111:
      PeakPartial::recoverPeaks0111(peakPtrsUsed);
      break;
    case 0b1010:
      PeakPartial::recoverPeaks1010(peakPtrsUsed);
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
  PeakPartial::printHits();
}


void PeakPartial::printHits() const
{
  string s;
  if (verboseFlag)
  {
    s = "PPNUM";
    for (unsigned i = 0; i < 16; i++)
      s += " " + to_string(pstats[i].num);
    cout << s << "\n";

    s = "PPHIT";
    for (unsigned i = 0; i < 16; i++)
      s += " " + to_string(pstats[i].hits);
    cout << s << "\n";
  }
}

