#ifndef TRAIN_CARDETECT_H
#define TRAIN_CARDETECT_H

#include <list>
#include <string>

#include "CarGaps.h"
#include "CarPeaks.h"
#include "Peak.h"

using namespace std;

class PeakRange;


class CarDetect
{
  private:

    // Excluding offset
    unsigned start; 
    unsigned end;

    CarGaps gaps;

    // The peaks aren't used much at the moment.
    CarPeaksPtr peaksPtr;

    unsigned statIndex;
    float distanceValue;

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
      const unsigned leftBogeyGap, // Zero if single wheel
      const unsigned midGap,
      const unsigned rightBogeyGap); // Zero if single wheel

    void logLeftGap(const unsigned leftGap);
    void logRightGap(const unsigned rightGap);

    void logPeakPointers(
      Peak * firstBogeyLeftPtr,
      Peak * firstBogeyRightPtr,
      Peak * secondBogeyLeftPtr,
      Peak * secondBogeyRightPtr);

    void makeLastTwoOfFourWheeler(
      const PeakRange& range,
      const vector<Peak *>& peakPtrs);

    void makeLastThreeOfFourWheeler(
      const PeakRange& range,
      const vector<Peak *>& peakPtrs);

    void makeFourWheeler(
      const PeakRange& range,
      const vector<Peak *>& peakPtrs);

    void logStatIndex(const unsigned index);

    void logDistance(const float d);

    void operator += (const CarDetect& c2);

    bool operator < (const CarDetect& c2) const;

    void increment(CarDetectNumbers& cdn) const;

    const unsigned startValue() const;
    const unsigned endValue() const;

    const unsigned firstPeakMinus1() const;
    const unsigned lastPeakPlus1() const;
    // TODO TMP
    const unsigned getMidGap() const;

    const CarPeaksPtr& getPeaksPtr() const;

    void getCarPoints(list<unsigned>& carPoints) const;

    bool hasLeftGap() const;
    bool hasRightGap() const;

    bool fillSides(
      const unsigned leftGap,
      const unsigned rightGap);

    bool fillSides(const CarDetect& cref);

    bool isPartial() const;

    void averageGaps(const CarDetectNumbers& cdn);

    float distance(const CarDetect& cref) const;

    bool gapsPlausible(const CarDetect& cref) const;
    bool sideGapsPlausible(const CarDetect& cref) const;
    bool midGapPlausible() const;
    bool rightBogeyPlausible(const CarDetect& cref) const;

    bool peakPrecedesCar(const Peak& peak) const;
    bool carPrecedesPeak(const Peak& peak) const;

    string strHeaderGaps() const;
    string strHeaderFull() const;

    string strGaps(const unsigned no) const;

    string strFull(
      const unsigned no,
      const unsigned offset) const;

    string strLimits(
      const unsigned offset,
      const string& text) const;
};

#endif
