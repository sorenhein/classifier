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

    bool hasLeftGap() const;
    bool hasRightGap() const;

    string strHeader() const;

    string str() const;
};

#endif
