#ifndef TRAIN_PEAKSLOTS_H
#define TRAIN_PEAKSLOTS_H

#include <vector>
#include <string>

#include "struct.h"

using namespace std;

class PeakPtrs;
class Peak;


enum PeakSlot
{
  PEAKSLOT_RIGHT_OF_P3 = 0,
  PEAKSLOT_BETWEEN_P2_P3 = 1,
  PEAKSLOT_BETWEEN_P1_P2 = 2,
  PEAKSLOT_BETWEEN_P0_P1 = 3,
  PEAKSLOT_LEFT_OF_P0 = 4,
  PEAKSLOT_RIGHT_OF_P2 = 5,
  PEAKSLOT_BETWEEN_P1_P3 = 6,
  PEAKSLOT_BETWEEN_P0_P2 = 7,
  PEAKSLOT_LEFT_OF_P1 = 8,
  PEAKSLOT_LEFT_OF_P3 = 9,
  PEAKSLOT_LEFT_OF_P2 = 10,
  PEAKSLOT_RIGHT_OF_P0 = 11,
  PEAKSLOT_RIGHT_OF_P1 = 12,
  PEAKSLOT_NUM_INTERVALS = 13,
  PEAKSLOT_SIZE = 14
};


class PeakSlots
{
  private:

    // A binary code of the peaks found.
    unsigned peakCode;

    // A count of input peaks in positions relative to found peaks.
    vector<unsigned> slots;

    // Number of entries in intervalCount
    unsigned numEntries;


    string s(unsigned n) const;
    string s(const PeakSlot ps) const;


  public:

    PeakSlots();

    ~PeakSlots();

    void reset();

    void log(
      Peak const * p0,
      Peak const * p1,
      Peak const * p2,
      Peak const * p3,
      const PeakPtrs& peaksUsed,
      const unsigned offset);

    unsigned count(const PeakSlot ps) const;
    
    unsigned number() const;

    unsigned code() const;

    string str(const CarPosition carpos) const;

};

#endif
