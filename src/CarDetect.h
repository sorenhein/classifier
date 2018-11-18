#ifndef TRAIN_CARDETECT_H
#define TRAIN_CARDETECT_H

#include <string>

#include "CarGaps.h"
#include "CarPeaks.h"
#include "Peak.h"

using namespace std;

struct CarDetectNumbers
{
  unsigned numLeftGaps;
  unsigned numCoreGaps;
  unsigned numRightGaps;
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

    void logStatIndex(const unsigned index);

    void logDistance(const float d);

    void operator += (const CarDetect& c2);

    bool operator < (const CarDetect& c2) const;

    void increment(CarDetectNumbers& cdn) const;

    const unsigned startValue() const;
    const unsigned endValue() const;

    const CarPeaksPtr& getPeaksPtr() const;

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
