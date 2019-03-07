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
  const vector<Peak *> peaks,
  const PeakPtrs& peakPtrsUsed,
  const unsigned offset)
{
  PeakSlots::reset();

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

  for (PPLciterator it = peakPtrsUsed.cbegin(); 
      it != peakPtrsUsed.cend(); it++)
  {
    Peak * p = * it;
    if (p == peaks[0] || p == peaks[1] || p == peaks[2] || p == peaks[3])
      continue;

    numEntries++;
    const unsigned index = p->getIndex();

    if (peaks[0] && index <= p0)
      slots[PEAKSLOT_LEFT_OF_P0]++;
    else if (peaks[3] && index > p3)
      slots[PEAKSLOT_RIGHT_OF_P3]++;

    else if (peaks[0] && peaks[1] && index <= p1)
      slots[PEAKSLOT_BETWEEN_P0_P1]++;
    else if (peaks[1] && peaks[2] && index > p1 && index <= p2)
      slots[PEAKSLOT_BETWEEN_P1_P2]++;
    else if (peaks[2] && peaks[3] && index > p2 && index <= p3)
      slots[PEAKSLOT_BETWEEN_P2_P3]++;

    else if (peaks[1] && index <= p1)
      slots[PEAKSLOT_LEFT_OF_P1]++;
    else if (peaks[2] && index > p2)
      slots[PEAKSLOT_RIGHT_OF_P2]++;

    else if (peaks[0] && peaks[2] && index > p0 && index <= p2)
      slots[PEAKSLOT_BETWEEN_P0_P2]++;
    else if (peaks[1] && peaks[3] && index > p1 && index <= p3)
      slots[PEAKSLOT_BETWEEN_P1_P3]++;

    else if (! peaks[0] && ! peaks[1])
    {
      if (! peaks[2] && index <= p3)
        slots[PEAKSLOT_LEFT_OF_P3]++;
      else if (peaks[2] && index <= p2)
        slots[PEAKSLOT_LEFT_OF_P2]++;
      else
        cout << "PEAKSLOT ERROR1 " << index + offset << "\n";
    }
    else if (! peaks[2] && ! peaks[3])
    {
      if (! peaks[1] && index > p0)
        slots[PEAKSLOT_RIGHT_OF_P0]++;
      else if (peaks[1] && index > p1)
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


string PeakSlots::str(const bool firstFlag) const
{
  if (! numEntries)
    return "";

  stringstream ss;
  ss << (firstFlag ? "PEAKFIRST " : "PEAKLAST  ") << 
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

