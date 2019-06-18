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
  COMP_UNUSED = 0,
  COMP_REPAIRABLE = 1,
  COMP_REPAIRED = 2,
  COMP_UNMATCHED = 3,
  COMP_SIZE = 4
};


class PeakCompletion
{
  private:

    unsigned mid;
    unsigned lower;
    unsigned upper;

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
      const CompletionType typeIn,
      unsigned& dist);

    void nominate(
      const CompletionType typeIn,
      unsigned& dist,
      Peak * pptrIn = nullptr);

    bool operator > (const PeakCompletion& pc2) const;

    bool consistentWith(const PeakCompletion& pc2);

    CompletionType source() const;

    Peak * ptr();

    void fill(Peak& peak);

    string strHeader() const;

    string str(const unsigned offset = 0) const;
};

#endif
