#ifndef TRAIN_PEAKSHORT_H
#define TRAIN_PEAKSHORT_H

#include "struct.h"

using namespace std;

class CarModels;
class CarDetect;
class PeakRange;
class PeakPool;
class PeakPtrs;


class PeakShort
{
  private:

    unsigned offset;

    CarDetect const * carBeforePtr;
    CarDetect const * carAfterPtr;

    RangeData rangeData;

    unsigned bogieTypical;
    unsigned longTypical;
    unsigned sideTypical;
    unsigned lenTypical;

    unsigned lenShortLo;
    unsigned lenShortHi;


    bool setGlobals(
      const CarModels& models,
      const PeakRange& range,
      const unsigned offsetIn);

    void getShorty(const CarModels& models);


  public:

    PeakShort();

    ~PeakShort();

    void reset();

    bool locate(
      const CarModels& models,
      PeakPool& peaks,
      const PeakRange& range,
      const unsigned offsetIn,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused);
};

#endif
