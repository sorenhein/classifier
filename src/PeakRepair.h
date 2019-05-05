#ifndef TRAIN_PEAKREPAIR_H
#define TRAIN_PEAKREPAIR_H

// This is really a part of PeakStructure which is hived off
// from the other car detectors.  Unlike the others, these ones
// look more carefully at adding and deleting peaks.

#include <vector>
#include <list>

#include "PeakPartial.h"
#include "Peak.h"

using namespace std;

class CarModels;
class PeakPool;
class PeakPtrs;
class PeakRange;
class CarDetect;


class PeakRepair
{
  private:

    unsigned offset;

    struct RepairRange
    {
      // If leftDirection then end < start.
      unsigned start;
      unsigned end;
      bool leftDirection;
    };

    list<PeakPartial> partialData;


    void init(
      const CarModels& models,
      const unsigned start);

    bool add(
      const unsigned end,
      const unsigned leftDirection,
      const unsigned pos1,
      const unsigned adder,
      unsigned& result) const;

    bool bracket(
      const RepairRange& range,
      const unsigned gap,
      unsigned& lower,
      unsigned& upper) const;

    bool updatePossibleModels(
      RepairRange& range,
      const bool specialFlag,
      const PeakPtrs& peakPtrsUsed,
      const unsigned peakNo,
      const CarModels& models);

    unsigned numMatches() const;

    bool getDominantModel(PeakPartial& dominantModel) const;

    string prefix(CarPosition carpos) const;

    void repairFourth(
      const CarPosition carpos,
      const PeakRange& range,
      PeakPool& peaks,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;


  public:

    PeakRepair();

    ~PeakRepair();

    void reset();

    bool edgeCar(
      const CarModels& models,
      const unsigned offsetIn,
      const CarPosition carpos,
      PeakPool& peaks,
      PeakRange& range,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused);

    void printMatches(const CarPosition carpos) const;

};

#endif
