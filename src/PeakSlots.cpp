#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "PeakSlots.h"
#include "PeakPtrs.h"


PeakSlots::PeakSlots()
{
  PeakSlots::reset();
}


PeakSlots::~PeakSlots()
{
}


void PeakSlots::reset()
{
  peakCode = 0;
  slots.clear();
  slots.resize(PEAKSLOT_SIZE);
  numEntries = 0;
}


void PeakSlots::log(
  Peak const * pk0,
  Peak const * pk1,
  Peak const * pk2,
  Peak const * pk3,
  const PeakPtrs& peakPtrsUsed,
  const unsigned offset)
{
  PeakSlots::reset();

  peakCode = (pk0 ? 8u : 0u) | (pk1 ? 4u : 0u) | 
    (pk2 ? 2u : 0u) | (pk3 ? 1u : 0u);

  //      p0      p1      p2      p3
  //   i4 >|< i3 >|<  i2 >|<  i1 >|<  i0
  //                      |<  i5
  //              |<      i6     >|
  //       |<     i7     >|
  //       i8    >|

  const unsigned p0 = (pk0 ? pk0->getIndex() : 0);
  const unsigned p1 = (pk1 ? pk1->getIndex() : 0);
  const unsigned p2 = (pk2 ? pk2->getIndex() : 0);
  const unsigned p3 = (pk3 ? pk3->getIndex() : 0);

  for (PPLciterator it = peakPtrsUsed.cbegin(); 
      it != peakPtrsUsed.cend(); it++)
  {
    Peak * p = * it;
    if (p == pk0 || p == pk1 || p == pk2 || p == pk3)
      continue;

    numEntries++;
    const unsigned index = p->getIndex();

    if (pk0 && index <= p0)
      slots[PEAKSLOT_LEFT_OF_P0]++;
    else if (pk3 && index > p3)
      slots[PEAKSLOT_RIGHT_OF_P3]++;

    else if (pk0 && pk1 && index <= p1)
      slots[PEAKSLOT_BETWEEN_P0_P1]++;
    else if (pk1 && pk2 && index > p1 && index <= p2)
      slots[PEAKSLOT_BETWEEN_P1_P2]++;
    else if (pk2 && pk3 && index > p2 && index <= p3)
      slots[PEAKSLOT_BETWEEN_P2_P3]++;

    else if (pk1 && index <= p1)
      slots[PEAKSLOT_LEFT_OF_P1]++;
    else if (pk2 && index > p2)
      slots[PEAKSLOT_RIGHT_OF_P2]++;

    else if (pk0 && pk2 && index > p0 && index <= p2)
      slots[PEAKSLOT_BETWEEN_P0_P2]++;
    else if (pk1 && pk3 && index > p1 && index <= p3)
      slots[PEAKSLOT_BETWEEN_P1_P3]++;

    else if (! pk0 && ! pk1)
    {
      if (! pk2 && index <= p3)
        slots[PEAKSLOT_LEFT_OF_P3]++;
      else if (pk2 && index <= p2)
        slots[PEAKSLOT_LEFT_OF_P2]++;
      else
        cout << "PEAKSLOT ERROR1 " << index + offset << "\n";
    }
    else if (! pk2 && ! pk3)
    {
      if (! pk1 && index > p0)
        slots[PEAKSLOT_RIGHT_OF_P0]++;
      else if (pk1 && index > p1)
        slots[PEAKSLOT_RIGHT_OF_P1]++;
      else
        cout << "PEAKSLOT ERROR2 " << index + offset << "\n";
    }
    else
      cout << "PEAKSLOT ERROR3 " << index + offset << "\n";
  }
}


unsigned PeakSlots::count(const PeakSlot ps) const
{
  return slots[ps];
}


unsigned PeakSlots::number() const
{
  return numEntries;
}


unsigned PeakSlots::code() const
{
  return peakCode;
}


string PeakSlots::s(unsigned n) const
{
  if (n == 0)
    return "-";
  else
    return to_string(n);
}


string PeakSlots::s(const PeakSlot ps) const
{
  if (slots[ps] == 0)
    return "-";
  else
    return to_string(slots[ps]);
}


string PeakSlots::str(const CarPosition carpos) const
{
  if (! numEntries)
    return "";

  stringstream ss;
  string cp;
  if (carpos == CARPOSITION_FIRST)
    cp = "FIRST ";
  else if (carpos == CARPOSITION_INNER_SINGLE ||
      carpos == CARPOSITION_INNER_MULTI)
    cp = "INNER ";
  else if (carpos == CARPOSITION_LAST)
    cp = "LAST  ";
  else
    cp = "";
    
  ss << "PEAK" << cp << 
    setw(2) << peakCode <<
    setw(4) << PeakSlots::s(peakCode & 0x8 ? 1 : 0) <<
    setw(2) << PeakSlots::s(peakCode & 0x4 ? 1 : 0) <<
    setw(2) << PeakSlots::s(peakCode & 0x2 ? 1 : 0) <<
    setw(2) << PeakSlots::s(peakCode & 0x1 ? 1 : 0) <<

    setw(4) << PeakSlots::s(PEAKSLOT_LEFT_OF_P0) <<
    setw(2) << PeakSlots::s(PEAKSLOT_BETWEEN_P0_P1) <<
    setw(2) << PeakSlots::s(PEAKSLOT_BETWEEN_P1_P2) <<
    setw(2) << PeakSlots::s(PEAKSLOT_BETWEEN_P2_P3) <<
    setw(2) << PeakSlots::s(PEAKSLOT_RIGHT_OF_P3) <<
    setw(4) << PeakSlots::s(PEAKSLOT_LEFT_OF_P1) <<
    setw(2) << PeakSlots::s(PEAKSLOT_BETWEEN_P0_P2) <<
    setw(2) << PeakSlots::s(PEAKSLOT_BETWEEN_P1_P3) <<
    setw(2) << PeakSlots::s(PEAKSLOT_RIGHT_OF_P2) << 
    setw(3) << PeakSlots::s(PEAKSLOT_LEFT_OF_P3) <<
    setw(2) << PeakSlots::s(PEAKSLOT_LEFT_OF_P2) << 
    setw(2) << PeakSlots::s(PEAKSLOT_RIGHT_OF_P1) << 
    setw(2) << PeakSlots::s(PEAKSLOT_RIGHT_OF_P0) << 
    endl;
  
  return ss.str();
}

