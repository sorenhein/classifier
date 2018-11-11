#ifndef TRAIN_CARGAPS_H
#define TRAIN_CARGAPS_H

#include <string>

using namespace std;


class CarGaps
{
  private:

    bool leftGapSet;
    bool rightGapSet;

    unsigned leftGap;
    unsigned leftBogeyGap; // Zero if single wheel
    unsigned midGap;
    unsigned rightBogeyGap; // Zero if single wheel
    unsigned rightGap;

    float relativeComponent(
      const unsigned a,
      const unsigned b) const;

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
      const unsigned leftBogeyGapIn, // Zero if single wheel
      const unsigned midGapIn,
      const unsigned rightBogeyGapIn, // Zero if single wheel
      const unsigned rightGapIn);

    void logCore(
      const unsigned leftBogeyGapIn, // Zero if single wheel
      const unsigned midGapIn,
      const unsigned rightBogeyGap); // Zero if single wheel

    bool logLeftGap(const unsigned leftGapIn);
    bool logRightGap(const unsigned rightGapIn);

    void operator += (const CarGaps& cg2);

    void average(
      const unsigned numLeftGap,
      const unsigned numCore,
      const unsigned numRightGap);

    unsigned leftGapValue() const;
    unsigned rightGapValue() const;

    bool hasLeftGap() const;
    bool hasRightGap() const;
    bool isPartial() const;

    float relativeDistance(const CarGaps& cg2) const;

    bool sideGapsPlausible(const CarGaps& cgref) const;
    bool midGapPlausible() const;
    bool rightBogeyPlausible(const CarGaps& cgref) const;
    bool rightBogeyConvincing(const CarGaps& cgref) const;
    bool gapsPlausible(const CarGaps& cgref) const;

    string strHeader() const;

    string str() const;
};

#endif
