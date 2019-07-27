#ifndef TRAIN_CARDETECT_H
#define TRAIN_CARDETECT_H

#include <list>
#include <string>

#include "CarGaps.h"
#include "CarPeaks.h"
#include "Peak.h"

using namespace std;

class PeakRange;


struct MatchData
{
  float distance;
  unsigned index;
  bool reverseFlag;

  string strIndex() const
  {
    return to_string(index) + (reverseFlag ? "R" : "");
  };
};


class CarDetect
{
  private:

    // Excluding offset
    unsigned start; 
    unsigned end;

    CarGaps gaps;

    // The peaks aren't used much at the moment.
    CarPeaksPtr peaksPtr;

    MatchData match;

    void updateStars(
      const Peak * peakPtr,
      string& best) const;

    string starsQuality() const;
    string starsDistance() const;

  public:

    CarDetect();

    ~CarDetect();

    void reset();

    void setLimits(
      const unsigned startIn,
      const unsigned endIn);

    void setStartAndGap(const unsigned startIn);
    void setEndAndGap(const unsigned endIn);

    void logCore(
      const unsigned leftBogieGap, // Zero if single wheel
      const unsigned midGap,
      const unsigned rightBogieGap); // Zero if single wheel

    void logLeftGap(const unsigned leftGap);
    void logRightGap(const unsigned rightGap);

    void logPeakPointers(
      Peak * firstBogieLeftPtr,
      Peak * firstBogieRightPtr,
      Peak * secondBogieLeftPtr,
      Peak * secondBogieRightPtr);

    void makeLastTwoOfFourWheeler(
      const PeakRange& range,
      PeakPtrs& peakPtrs);

    void makeLastThreeOfFourWheeler(
      const PeakRange& range,
      PeakPtrs& peakPtrs);

    void makeFourWheeler(
      const PeakRange& range,
      PeakPtrs& peakPtrs);

    void makeAnyWheeler(
      const PeakRange& range,
      PeakPtrs& peakPtrs);

    void logStatIndex(const unsigned index);

    void logDistance(const float d);

    void logMatchData(const MatchData& match);
    MatchData const * getMatchData() const;

    void operator += (const CarDetect& c2);

    bool operator < (const CarDetect& c2) const;

    void increment(CarDetectNumbers& cdn) const;

    void reverse();

    unsigned startValue() const;
    unsigned endValue() const;
    unsigned index() const;
    bool isReversed() const;

    unsigned firstPeakMinus1() const;
    unsigned lastPeakPlus1() const;
    unsigned firstPeak() const;

    unsigned getLeftGap() const;
    unsigned getLeftBogieGap() const;
    unsigned getMidGap() const;
    unsigned getRightBogieGap() const;
    unsigned getRightGap() const;

    const CarPeaksPtr& getPeaksPtr() const;

    void getCarPoints(list<unsigned>& carPoints) const;

    bool hasLeftGap() const;
    bool hasRightGap() const;
    bool hasLeftBogieGap() const;
    bool hasRightBogieGap() const;
    bool hasMidGap() const;

    bool isSymmetric() const;
    bool isPartial() const;
    unsigned numNulls() const;

    void averageGaps(const CarDetectNumbers& cdn);
    void averageGaps(const unsigned count);

    float distance(const CarDetect& cref) const;
    void distanceSymm(
      const CarDetect& cref,
      const bool partialFlag,
      MatchData& matchIn) const;
    bool distancePartialSymm(
      const PeakPtrs& peakPtrs,
      const float& limit,
      MatchData& matchIn,
      Peak& peakCand) const;

    bool gapsPlausible(const CarDetect& cref) const;
    bool sideGapsPlausible(const CarDetect& cref) const;
    bool midGapPlausible() const;
    bool rightBogiePlausible(const CarDetect& cref) const;
    bool corePlausible() const;

    unsigned getGap(
      const bool reverseFlag,
      const bool specialFlag,
      const bool skippedFlag,
      const unsigned peakNo) const;

    bool peakPrecedesCar(const Peak& peak) const;
    bool carPrecedesPeak(const Peak& peak) const;

    unsigned numFrontWheels() const;

    string strHeaderGaps() const;
    string strHeaderFull() const;

    string strGaps(
      const unsigned no,
      const unsigned count = 0,
      const bool containedFlag = false,
      const unsigned containedIndex = 0) const;

    string strFull(
      const unsigned no,
      const unsigned offset) const;

    string strLimits(
      const unsigned offset,
      const string& text) const;
};

#endif
