#ifndef TRAIN_PEAKRANGE_H
#define TRAIN_PEAKRANGE_H

#include <list>
#include <string>
#include <iostream>
#include <iomanip>

#include "PeakProfile.h"
#include "PeakPool.h"

using namespace std;

class CarModels;
class CarDetect;

typedef list<CarDetect>::iterator CarListIter;
typedef list<CarDetect>::const_iterator CarListConstIter;


enum RangeQuality
{
  QUALITY_ACTUAL_GAP = 0,
  QUALITY_BY_SYMMETRY = 1,
  QUALITY_BY_ANALOGY = 2,
  QUALITY_NONE = 3
};


struct RangeData
{
  RangeQuality qualLeft;
  RangeQuality qualRight;
  RangeQuality qualBest;
  RangeQuality qualWorst;

  unsigned gapLeft;
  unsigned gapRight;
  unsigned indexLeft;
  unsigned indexRight;
  unsigned lenRange;

  RangeData()
  {
    qualLeft = QUALITY_NONE;
    qualRight = QUALITY_NONE;
    qualBest = QUALITY_NONE;
    qualWorst = QUALITY_NONE;

    gapLeft = 0;
    gapRight = 0;

    indexLeft = 0;
    indexRight = 0;
    lenRange = 0;
  };

  string strQuality(const RangeQuality qual) const
  {
    if (qual == QUALITY_ACTUAL_GAP)
      return "ACTUAL";
    else if (qual == QUALITY_BY_SYMMETRY)
      return "SYMMETRY";
    else
      return "(none)";
  };

  string str(
    const string& title,
    const unsigned offset) const
  {
    stringstream ss;
    ss << title << "\n" <<
      setw(10) << left << "qualLeft " <<
        RangeData::strQuality(qualLeft) << "\n" <<
      setw(10) << left << "qualRight " <<
        RangeData::strQuality(qualRight) << "\n" <<
      setw(10) << left << "gapLeft " << gapLeft << "\n" <<
      setw(10) << left << "gapRight " << gapLeft << "\n" <<
      setw(10) << left << "indices " <<
        indexLeft + offset << " - " <<
        (indexRight == 0 ? "open" : to_string(indexRight + offset)) <<  
        "\n" <<
      setw(10) << left << "length " << 
        (lenRange == 0 ? "open" : to_string(lenRange)) << endl;
    return ss.str();
  };
};


class PeakRange
{
  private:

    // The car after the range
    CarListConstIter carAfter; 

    CarDetect const * _carBeforePtr;
    CarDetect const * _carAfterPtr;

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


    bool getQuality(
      const CarModels& models,
      CarDetect const * carPtr,
      const unsigned bogieTypical,
      const bool leftFlag,
      RangeQuality& quality,
      unsigned& gap) const;
    

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

    bool characterize(
      const CarModels& models,
      const unsigned bogieTypical,
      RangeData& rdata) const;

    void shortenLeft(const CarDetect& car);

    void shortenRight(
      const CarDetect& car,
      const list<CarDetect>::iterator& carIt);

    const CarListConstIter& carAfterIter() const;
    CarDetect const * carBeforePtr() const;
    CarDetect const * carAfterPtr() const;

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

    string strInterval(
      const unsigned offset,
      const string& text) const;
    
    string strPos() const;

    string strFull(const unsigned offset) const;

    string strProfile() const;
};

#endif
