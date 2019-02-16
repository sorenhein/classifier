#ifndef TRAIN_PEAKREPAIR_H
#define TRAIN_PEAKREPAIR_H

// This is really a part of PeakStructure which is hived off
// from the other car detectors.  Unlike the others, these ones
// look more carefully at adding and deleting peaks.

#include <vector>
#include <list>

#include "Peak.h"

using namespace std;

class CarModels;
class PeakPool;
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

    struct ModelData
    {
      unsigned mno;
      bool reverseFlag;

      bool modelUsedFlag;
      bool matchFlag;
      bool skippedFlag;
      bool newFlag;
      vector<Peak *> peaks;
      unsigned lastIndex;

      vector<unsigned> lower;
      vector<unsigned> upper;
      vector<unsigned> target;
      vector<unsigned> indexUsed;
    };

    list<ModelData> modelData;


    void init(
      const unsigned msize,
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

    Peak * locatePeak(
      const unsigned lower,
      const unsigned upper,
      PeakPtrVector& PeakPtrsUsed,
      unsigned& indexUsed) const;

    bool updatePossibleModels(
      RepairRange& range,
      const bool specialFlag,
      PeakPtrVector& peakPtrsUsed,
      const unsigned peakNo,
      const CarModels& models);

    unsigned numMatches() const;

    string strIndex(
      const ModelData& m,
      const unsigned pno) const;


  public:

    PeakRepair();

    ~PeakRepair();

    void reset();

    bool firstCar(
      const CarModels& models,
      const unsigned offsetIn,
      PeakPool& peaks,
      PeakRange& range,
      CarDetect& car);

    void printMatches() const;

};

#endif
