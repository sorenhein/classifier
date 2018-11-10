#ifndef TRAIN_CARGEOM_H
#define TRAIN_CARGEOM_H

#include <string>

using namespace std;


class CarGeom
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

    CarGeom();

    ~CarGeom();

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

    void logLeftGap(const unsigned leftGapIn);
    void logRightGap(const unsigned rightGapIn);

    void operator += (const CarGeom& cg2);

    void average(
      const unsigned numLeftGap,
      const unsigned numCore,
      const unsigned numRightGap);

    unsigned leftGapValue() const;
    unsigned rightGapValue() const;

    bool hasLeftGap() const;
    bool hasRightGap() const;

    float relativeDistance(const CarGeom& cg2) const;

    bool sideGapsPlausible(const CarGeom& cgref) const;
    bool midGapPlausible() const;
    bool rightBogeyPlausible(const CarGeom& cgref) const;
    bool rightBogeyConvincing(const CarGeom& cgref) const;
    bool gapsPlausible(const CarGeom& cgref) const;

    string strHeader() const;

    string str() const;
};

#endif
