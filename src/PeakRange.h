#ifndef TRAIN_PEAKRANGE_H
#define TRAIN_PEAKRANGE_H

#include <list>
#include <string>

#include "PeakProfile.h"
#include "PeakPool.h"

#include "struct.h"

using namespace std;

class CarDetect;

typedef list<CarDetect>::iterator CarListIter;
typedef list<CarDetect>::const_iterator CarListConstIter;


class PeakRange
{
  private:

    // The car after the range
    CarListConstIter carAfter; 
    PeakSource source;
    unsigned start;
    unsigned endVal;
    bool leftGapPresent;
    bool rightGapPresent;
    bool leftOriginal;
    bool rightOriginal;

    PeakIterList peakIters;
    PeakPtrs peakPtrs;
    PeakProfile profile;
    

  public:

    PeakRange();

    ~PeakRange();

    void reset();

    PIciterator begin() const { return peakIters.begin(); }
    PIciterator end() const { return peakIters.end(); }

    void init(
      const list<CarDetect>& cars,
      const PeakPtrs& candidates);

    void init(const PeakPtrs& candidates);

    void fill(PeakPool& peaks);

    void shortenLeft(const CarDetect& car);

    void shortenRight(
      const CarDetect& car,
      const list<CarDetect>::iterator& carIt);

    const CarListConstIter& carAfterIter() const;
    unsigned startValue() const;
    unsigned endValue() const;
    bool hasLeftGap() const;
    bool hasRightGap() const;

    unsigned numGreat() const;
    unsigned numGood() const;
    unsigned num() const;

    PeakPtrs& getPeakPtrs();

    void split(
      const PeakFncPtr& fptr,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused) const;

    void split(
      const vector<Peak const *>& flattened,
      PeakPtrs& peakPtrsUsed,
      PeakPtrs& peakPtrsUnused);

    bool isFirstCar() const;
    bool isLastCar() const;
    bool match(const Recognizer& recog) const;
    bool looksEmpty() const;
    bool looksEmptyLast() const;

    bool updateImperfections(
      const list<CarDetect>& cars,
      Imperfections& imperf) const;

    string strInterval(
      const unsigned offset,
      const string& text) const;
    
    string strPos() const;

    string strFull(const unsigned offset) const;

    string strProfile() const;
};

#endif
