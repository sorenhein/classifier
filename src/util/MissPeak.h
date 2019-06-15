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
  MISS_REPAIRABLE = 1,
  MISS_UNMATCHED = 2,
  MISS_SIZE = 3
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

    bool markWith(
      Peak * pptrIn,
      const MissType typeIn);

    void nominate(
      const MissType typeIn,
      Peak * pptrIn = nullptr);

    bool consistentWith(const MissPeak& miss2);

    MissType source() const;

    Peak * ptr();

    string strHeader() const;

    string str(const unsigned offset = 0) const;
};

#endif
