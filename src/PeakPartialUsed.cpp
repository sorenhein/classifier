#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "PeakPartial.h"
#include "Peak.h"
#include "PeakPtrs.h"


// 1.2x is a normal range for short cars, plus a bit of buffer.
#define BOGIE_GENERAL_FACTOR 0.3f

// When we're comparing within a given car, we are stricter.
#define BOGIE_SPECIFIC_FACTOR 0.1f

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
      (index + bogieHi < comps[3].peak->getIndex() ||
      ! peakPtrsUsed[iu-1]->fantasticQuality()))
    return;

  // Complete the right bogie.
  
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

  // Take a stab at the first wheel of the left bogie, too.
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
    static_cast<unsigned>((1.0f - BOGIE_SPECIFIC_FACTOR) * d);

  const unsigned hi = 
    static_cast<unsigned>((1.0f + BOGIE_SPECIFIC_FACTOR) * d);

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


void PeakPartial::recoverPeaks1000(vector<Peak const *>& peakPtrsUsed)
{
  pstats[0b1000].num++;

  // TODO Could also be missing a single peak, among other things.

  const unsigned iu0 = comps[0].indexUsed;

  // Look for the left pair.

  Peak const * pptr;
  unsigned indexUsed;

  const unsigned num01 = PeakPartial::recoverPeak(
    0.3f, 0.5f, true, iu0, iu0+1, peakPtrsUsed.size(), 
    peakPtrsUsed, pptr, indexUsed);
  
  if (num01 == 1)
  {
    if (verboseFlag)
      cout << "Reinstating one 1000 peak, should be p1\n";

    PeakPartial::registerPtr(1, peakPtrsUsed[indexUsed], indexUsed);
    pstats[0b1000].hits++;

// cout << "PPX moving to 1100" << endl;
    PeakPartial::recoverPeaks1100(peakPtrsUsed);
    return;
  }
}


void PeakPartial::recoverPeaks1010(vector<Peak const *>& peakPtrsUsed)
{
  // Take a simple look for a pair that looks like a bogie.

  pstats[0b1010].num++;

  // Start from 3+ peaks.
  if (peakPtrsUsed.size() <= 2)
    return;

  const unsigned iu0 = comps[0].indexUsed;
  const unsigned iu2 = comps[2].indexUsed;

  Peak const * pptr;
  unsigned indexUsed = 0;

  // Look forward from p0.
  const unsigned num01 = PeakPartial::recoverPeak(
    0.3f, 0.5f, true, iu0, iu0+1, iu2, peakPtrsUsed, pptr, indexUsed);

  if (num01 > 1)
  {
    cout << "recoverPeaks1010: " << num01 << " 02up options\n";
    return;
  }
  else if (num01)
  {
    if (verboseFlag)
      cout << "Reinstating one 1010 peak, should be p1\n";

    PeakPartial::registerPtr(1, peakPtrsUsed[indexUsed], indexUsed);
    pstats[0b1010].hits++;

    PeakPartial::recoverPeaks1110(peakPtrsUsed);
    return;
  }

  // Look backward from p2.
  const unsigned num12 = PeakPartial::recoverPeak(
    0.3f, 0.5f, true, iu2, iu0+1, iu2, peakPtrsUsed, pptr, indexUsed);

  if (num12 > 1)
  {
    cout << "recoverPeaks1010: " << num12 << " 02down options\n";
    return;
  }
  else if (num12)
  {
    if (verboseFlag)
      cout << "Reinstating one 1010 peak, should be p2(p3)\n";

    PeakPartial::movePtr(2, 3);
    PeakPartial::registerPtr(2, peakPtrsUsed[indexUsed], indexUsed);
    pstats[0b1010].hits++;

    PeakPartial::recoverPeaks1011(peakPtrsUsed);
    return;
  }

  // Look forward from p2.
  const unsigned num23 = PeakPartial::recoverPeak(
    0.3f, 0.5f, true, iu2, iu2+1, peakPtrsUsed.size(), 
    peakPtrsUsed, pptr, indexUsed);

  if (num23 > 1)
  {
    cout << "recoverPeaks1010: " << num23 << " 2up options\n";
    return;
  }
  else if (num23)
  {
    if (verboseFlag)
      cout << "Reinstating one 1010 peak, should be p3\n";

    PeakPartial::registerPtr(3, peakPtrsUsed[indexUsed], indexUsed);
    pstats[0b1010].hits++;

    PeakPartial::recoverPeaks1011(peakPtrsUsed);
    return;
  }
}


void PeakPartial::recoverPeaks1011(vector<Peak const *>& peakPtrsUsed)
{
  pstats[0b1011].num++;

  const unsigned bogieSpecific = 
    comps[3].peak->getIndex() - comps[2].peak->getIndex();

  Peak const * pptr0 = nullptr, * pptr1 = nullptr;
  unsigned indexUsed0 = 0, indexUsed1 = 0;

  // Could be p0-null-p2-p3, missing the p1.
  const unsigned index0 = comps[0].peak->getIndex();

  if (peakSlots.count(PEAKSLOT_BETWEEN_P0_P2) > 0)
    pptr1 = PeakPartial::lookForPeak(index0, bogieSpecific, 0.2f, 0.5f,
      true, peakPtrsUsed, indexUsed1);

  // Could be p1-null-p2-p3, missing the p0.
  if (peakSlots.count(PEAKSLOT_LEFT_OF_P0) > 0)
    pptr0 = PeakPartial::lookForPeak(index0, bogieSpecific, 0.2f, 0.5f,
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

  // Look for the right bogie.
  // TODO Could also be a single peak.

  const unsigned dlo = comps[1].peak->getIndex() -
    comps[0].peak->getIndex();
  const unsigned dlimLo = 
    static_cast<unsigned>((1.0f - BOGIE_GENERAL_FACTOR) * dlo);
  const unsigned dlimHi = 
    static_cast<unsigned>((1.0f + BOGIE_GENERAL_FACTOR) * dlo);

// cout << "PPX d " << dlo << " " << dlimLo << " " << dlimHi << endl;
  for (unsigned iu = comps[1].indexUsed+1; iu < peakPtrsUsed.size()-1; iu++)
  {
    // TODO Don't have to be consecutive.
    if (! peakPtrsUsed[iu]->greatQuality() ||
        ! peakPtrsUsed[iu+1]->greatQuality())
      continue;

    const unsigned dhi = peakPtrsUsed[iu+1]->getIndex() -
      peakPtrsUsed[iu]->getIndex();

    if (dhi < dlimLo || dhi > dlimHi)
      continue;

    if (verboseFlag)
      cout << "Reinstating two 1100 peaks, should be p2+p3\n";

    PeakPartial::registerPtr(2, peakPtrsUsed[iu], iu);
    PeakPartial::registerPtr(3, peakPtrsUsed[iu+1], iu+1);
    pstats[0b1100].hits++;
    return;
  }
}


void PeakPartial::recoverPeaks1101(vector<Peak const *>& peakPtrsUsed)
{
  pstats[0b1101].num++;

  const unsigned bogieSpecific = 
    comps[1].peak->getIndex() - comps[0].peak->getIndex();

  Peak const * pptr2 = nullptr, * pptr3 = nullptr;
  unsigned indexUsed2 = 0, indexUsed3 = 0;

  // Could be p0-p1-null-p2, missing the p3.
  const unsigned index3 = comps[3].peak->getIndex();
  if (peakSlots.count(PEAKSLOT_RIGHT_OF_P3) > 0)
    pptr3 = PeakPartial::lookForPeak(index3, bogieSpecific, 0.2f, 0.5f,
      true, peakPtrsUsed, indexUsed3);

  // Could be p0-p1-null-p3, missing the p2.
  if (peakSlots.count(PEAKSLOT_BETWEEN_P1_P3) > 0)
    pptr2 = PeakPartial::lookForPeak(index3, bogieSpecific, 0.2f, 0.5f,
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

  const unsigned bogieSpecific = 
    comps[1].peak->getIndex() - comps[0].peak->getIndex();

  Peak const * pptr2 = nullptr, * pptr3 = nullptr;
  unsigned indexUsed2 = 0, indexUsed3 = 0;

  // Could be p0-p1-p2-null, missing the p3.
  const unsigned index2 = comps[2].peak->getIndex();

  if (peakSlots.count(PEAKSLOT_RIGHT_OF_P2) > 0)
    pptr3 = PeakPartial::lookForPeak(index2, bogieSpecific, 0.2f, 0.5f,
      true, peakPtrsUsed, indexUsed3);

  // Could be p0-p1-p3-null, missing the p2.
  if (peakSlots.count(PEAKSLOT_BETWEEN_P1_P2) > 0)
    pptr2 = PeakPartial::lookForPeak(index2, bogieSpecific, 0.2f, 0.5f,
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


unsigned PeakPartial::recoverPeak(
  const float& smallFactor,
  const float& largeFactor,
  const bool upFlag,
  const unsigned iuRef,
  const unsigned iuStart,
  const unsigned iuEnd,
  const vector<Peak const *>& peakPtrsUsed,
  Peak const *& pptr,
  unsigned& indexUsed) const
{
  unsigned num = 0;
  for (unsigned iu = iuStart; iu < iuEnd; iu++)
  {
    pptr = PeakPartial::closePeak(smallFactor, largeFactor, upFlag,
      peakPtrsUsed, iuRef, iu);
    
    if (pptr)
    {
      num++;
      indexUsed = iu;
    }
  }
  return num;
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
  const unsigned bogieTypical,
  const unsigned longTypical,
  const bool verboseFlagIn,
  vector<Peak const *>& peakPtrsUsed)
{
  // Mostly for the sport, we try to use as many of the front peaks
  // as possible.  Mostly the alignment will pick up on it anyway.

  bogie = bogieTypical;
  bogieLo = 
    static_cast<unsigned>((1.f - BOGIE_GENERAL_FACTOR) * bogieTypical);
  bogieHi = 
    static_cast<unsigned>((1.f + BOGIE_GENERAL_FACTOR) * bogieTypical);

  mid = static_cast<unsigned>(MID_FACTOR * longTypical);
  verboseFlag = verboseFlagIn;

cout << "CODE " << peakSlots.code() << endl;
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
    case 0b1000:
      PeakPartial::recoverPeaks1000(peakPtrsUsed);
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

