#ifndef TRAIN_CARGAPS_H
#define TRAIN_CARGAPS_H

#include <string>

#include "Peak.h"

using namespace std;

class PeakPtrs;


struct CarDetectNumbers
{
  unsigned numLeftGaps;
  unsigned numLeftBogieGaps;
  unsigned numMidGaps;
  unsigned numRightBogieGaps;
  unsigned numRightGaps;

  void reset()
  {
    numLeftGaps = 0;
    numLeftBogieGaps = 0;
    numMidGaps = 0;
    numRightBogieGaps = 0;
    numRightGaps = 0;
  };
};


class CarGaps
{
  private:

    bool leftGapSet;
    bool leftBogieGapSet;
    bool midGapSet;
    bool rightBogieGapSet;
    bool rightGapSet;

    unsigned leftGap;
    unsigned leftBogieGap; // Zero if single wheel
    unsigned midGap;
    unsigned rightBogieGap; // Zero if single wheel
    unsigned rightGap;

    float relativeComponent(
      const unsigned a,
      const unsigned b) const;

    float distance(const CarGaps& cg2) const;
    float reverseDistance(const CarGaps& cg2) const;

    bool checkTwoSided(
      const unsigned actual,
      const unsigned reference,
      const float factor,
      const string& text) const;

    bool checkTooShort(
      const unsigned actual,
      const unsigned reference,
      const float factor,
      const string& text) const;


  public:

    CarGaps();

    ~CarGaps();

    void reset();

    void logAll(
      const unsigned leftGapIn,
      const unsigned leftBogieGapIn, // Zero if single wheel
      const unsigned midGapIn,
      const unsigned rightBogieGapIn, // Zero if single wheel
      const unsigned rightGapIn);

    void logCore(
      const unsigned leftBogieGapIn, // Zero if single wheel
      const unsigned midGapIn,
      const unsigned rightBogieGap); // Zero if single wheel

    unsigned logLeftGap(const unsigned leftGapIn);
    unsigned logRightGap(const unsigned rightGapIn);

    void operator += (const CarGaps& cg2);

    void increment(CarDetectNumbers& cdn) const;

    void reverse();

    void average(const CarDetectNumbers& cdn);
    void average(const unsigned count);

    unsigned leftGapValue() const;
    unsigned leftBogieGapValue() const;
    unsigned midGapValue() const;
    unsigned rightBogieGapValue() const;
    unsigned rightGapValue() const;

    unsigned getGap(
      const bool reverseFlag,
      const bool specialFlag,
      const bool skippedFlag,
      const unsigned peakNo) const;

    bool hasLeftGap() const;
    bool hasRightGap() const;
    bool hasLeftBogieGap() const;
    bool hasRightBogieGap() const;
    bool hasMidGap() const;
    bool isPartial() const;

    unsigned sidelobe(
      const float& limit,
      const float& dist,
      const unsigned gap) const;

    float distancePartialBest(
      const unsigned g1,
      const unsigned g2,
      const unsigned g3,
      const PeakPtrs& peakPtrs,
      const float& limit,
      Peak& peakCand) const;

    float distanceForGapMatch(const CarGaps& cg2) const;
    float distanceForReverseMatch(const CarGaps& cg2) const;
    float distanceForGapInclusion(const CarGaps& cg2) const;
    float distanceForReverseInclusion(const CarGaps& cg2) const;
    float distancePartial(
      const PeakPtrs& peakPtrs,
      const float& limit,
      Peak& peakCand) const;
    float distancePartialReverse(
      const PeakPtrs& peakPtrs,
      const float& limit,
      Peak& peakCand) const;

    bool sideGapsPlausible(const CarGaps& cgref) const;
    bool midGapPlausible() const;
    bool rightBogiePlausible(const CarGaps& cgref) const;
    bool rightBogieConvincing(const CarGaps& cgref) const;
    bool corePlausible() const;

    string strHeader(const bool numberFlag) const;

    string str() const;
    string str(const unsigned no) const;
};

#endif
