#ifndef TRAIN_PEAKRANGE_H
#define TRAIN_PEAKRANGE_H

#include <list>
#include <string>

#include "PeakProfile.h"
#include "PeakPool.h"

#include "struct.h"

using namespace std;

class CarDetect;


class PeakRange2
{
  private:

    // The car after the range
    list<CarDetect>::const_iterator carAfter; 
    PeakSource source;
    unsigned start;
    unsigned endVal;
    bool leftGapPresent;
    bool rightGapPresent;
    bool leftOriginal;
    bool rightOriginal;

    PeakIterList peakIters;
    PeakPtrVector peakPtrs;
    PeakProfile profile;
    

  public:

    PeakRange2();

    ~PeakRange2();

    void reset();

    PIciterator begin() const { return peakIters.begin(); }
    PIciterator end() const { return peakIters.end(); }

    void init(
      const list<CarDetect>& cars,
      const PeakPool& peaks);

    void init(const PeakPtrVector& pv);

    void fill(const PeakPool& peaks);

    void shortenLeft(const CarDetect& car);

    void shortenRight(
      const CarDetect& car,
      const list<CarDetect>::iterator& carIt);

    unsigned startValue() const;
    unsigned endValue() const;
    bool hasLeftGap() const;
    bool hasRightGap() const;

    unsigned numGreat() const;
    unsigned numGood() const;

    PeakPtrVector& getPeakPtrs();

    void splitByQuality(
      const PeakFncPtr& fptr,
      PeakPtrVector& peakPtrsUsed,
      PeakPtrVector& peakPtrsUnused) const;

    bool match(const Recognizer& recog) const;
    bool looksEmpty() const;

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
