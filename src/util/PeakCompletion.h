/*
    A simple class to keep track of a missing, sought-after peak
    when completing a car.
 */

#ifndef TRAIN_PEAKCOMPLETION_H
#define TRAIN_PEAKCOMPLETION_H

#include <string>

using namespace std;

class Peak;


enum CompletionType
{
  COMP_USED = 0,
  COMP_UNUSED = 1,
  COMP_REPAIRABLE = 2,
  COMP_REPAIRED = 3,
  COMP_UNMATCHED = 4,
  COMP_SIZE = 5
};


class PeakCompletion
{
  private:

    unsigned mid;
    unsigned lower;
    unsigned upper;

    // If all peaks in a car appear to be off by a similar shift,
    // we adjust them.
    unsigned adjusted; 

    CompletionType _type;
    Peak * pptr;

    string strType() const;


  public:

    PeakCompletion();

    ~PeakCompletion();

    void reset();

    void addMiss(
      const unsigned target,
      const unsigned tolerance);

    void addMatch(
      const unsigned target,
      Peak * ptr);

    bool markWith(
      Peak& peak,
      const CompletionType typeIn);

    bool operator > (const PeakCompletion& pc2) const;

    bool consistentWith(const PeakCompletion& pc2);

    CompletionType source() const;

    Peak * ptr();

    void fill(Peak& peak);

    int distance() const;
    unsigned distanceShiftSquared() const;
    float qualityShape() const;
    float qualityPeak() const;

    void adjust(const int shift);

    string strHeader() const;

    string str(const unsigned offset = 0) const;
};

#endif
