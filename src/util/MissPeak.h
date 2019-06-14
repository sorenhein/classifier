/*
    A simple class to keep track of a missing, sought-after peak
    when completing a car.
 */

#ifndef TRAIN_MISSPEAK_H
#define TRAIN_MISSPEAK_H

#include <string>

using namespace std;

class Peak;


enum MissType
{
  MISS_UNUSED = 0,
  MISS_USED = 1,
  MISS_REPAIRABLE = 2,
  MISS_UNMATCHED = 3,
  MISS_SIZE = 4
};


class MissPeak
{
  private:

    unsigned mid;
    unsigned lower;
    unsigned upper;

    MissType _type;
    Peak * pptr;

    string strType() const;


  public:

    MissPeak();

    ~MissPeak();

    void reset();

    void set(
      const unsigned target,
      const unsigned tolerance);

    void nominate(
      const MissType typeIn,
      Peak * pptrIn = nullptr);

    MissType source() const;

    Peak * ptr();

    string strHeader() const;

    string str() const;
};

#endif
