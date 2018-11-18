#ifndef TRAIN_PEAKMINIMA_H
#define TRAIN_PEAKDMINIMAH

#include <vector>
#include <list>
#include <string>

#include "struct.h"

using namespace std;

class Peak;


class PeakMinima
{
  private:

    struct Gap
    {
      unsigned lower;
      unsigned upper;
      unsigned count;
    };

    enum PrintQuality
    {
      PRINT_SEED = 0,
      PRINT_WHEEL = 1,
      PRINT_BOGEY = 2,
      PRINT_SIZE = 3
    };

    typedef bool (PeakMinima::*CandFncPtr)(
      const Peak * p1, 
      const Peak * p2) const;

    unsigned offset;


    void findFirstLargeRange(
      const vector<unsigned>& dists,
      Gap& gap,
      const unsigned lowerCount = 0) const;

    bool bothSelected(
      const Peak * p1,
      const Peak * p2) const;

    bool formBogeyGap(
      const Peak * p1,
      const Peak * p2) const;

    void guessNeighborDistance(
      const list<Peak *>& candidates,
      const CandFncPtr fptr,
      Gap& gap) const;

    void markWheelPair(
      Peak& p1,
      Peak& p2,
      const string& text) const;

    void markBogeyShortGap(
      Peak& p1,
      Peak& p2,
      const string& text) const;

    void markBogeyLongGap(
      Peak& p1,
      Peak& p2,
      const string& text) const;

    void reseedWheelUsingQuality(list<Peak *>& candidates) const;

    void reseedBogeysUsingQuality(
      list<Peak *>& candidates,
      const vector<Peak>& bogeyScale) const;

    void reseedLongGapsUsingQuality(
      list<Peak *>& candidates,
      const vector<Peak>& longGapScale) const;

    void makeWheelAverage(
      list<Peak *>& candidates,
      Peak& wheel) const;

    void makeBogeyAverages(
      list<Peak *>& candidates,
      vector<Peak>& wheels) const;

    void makeCarAverages(
      list<Peak *>& candidates,
      vector<Peak>& wheels) const;

    void setCandidates(
      list<Peak>& peaks,
      list<Peak *>& candidates) const;

    void markSinglePeaks(list<Peak *>& candidates) const;


    void markBogeysOfSelects(
      list<Peak *>& candidates,
      const Gap& wheelGap) const;

    void markBogeysOfUnpaired(
      list<Peak *>& candidates,
      const Gap& wheelGap) const;

    void markBogeys(list<Peak *>& candidates) const;


    void markShortGapsOfSelects(
      list<Peak *>& candidates,
      const Gap& wheelGap) const;

    void markShortGapsOfUnpaired(
      list<Peak *>& candidates,
      const Gap& wheelGap) const;

    void markLongGapsOfSelects(
      list<Peak *>& candidates,
      const Gap& wheelGap) const;

    void markShortGaps(
      list<Peak *>& candidates,
      Gap& shortGap);


    void guessLongGapDistance(
      const list<Peak *>& candidates,
      const unsigned shortGapCount,
      Gap& gap) const;

    void markLongGaps(
      list<Peak *>& candidates,
      const unsigned shortGapCount);


    void printPeakQuality(
      const Peak& peak,
      const string& text) const;

    void printRange(
      const unsigned start,
      const unsigned end,
      const string& text) const;

    void printAllCandidates(
      const list<Peak *>& candidates,
      const string& text) const;

    void printSelected(
      const list<Peak *>& candidates,
      const string& text) const;


  public:

    PeakMinima();

    ~PeakMinima();

    void reset();

    void mark(
      list<Peak>& peaks,
      list<Peak *>& candidates,
      const unsigned offsetIn);

};

#endif
