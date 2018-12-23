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

      Gap()
      {
        lower = 0;
        upper = 0;
        count = 0;
      };
    };

    typedef bool (PeakMinima::*CandFncPtr)(
      const Peak * p1, 
      const Peak * p2) const;

    typedef bool (Peak::*PeakFncPtr)() const;

    unsigned offset;


    void findFirstLargeRange(
      const vector<unsigned>& dists,
      Gap& gap,
      const unsigned lowerCount = 0) const;

    bool bothSelected(
      const Peak * p1,
      const Peak * p2) const;

    bool bothPlausible(
      const Peak * p1,
      const Peak * p2) const;

    bool formBogeyGap(
      const Peak * p1,
      const Peak * p2) const;

    unsigned countPeaks(
      const list<Peak *>& candidates,
      const PeakFncPtr fptr) const;

    bool guessNeighborDistance(
      const list<Peak *>& candidates,
      const CandFncPtr fptr,
      Gap& gap,
      const unsigned minCount = 0) const;

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

    void markBogeys(
      list<Peak *>& candidates,
      Gap& wheelGap) const;


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
      const Gap& wheelGap,
      const unsigned shortGapCount);


    void printPeakQuality(
      const Peak& peak,
      const string& text) const;

    void printDists(
      const unsigned start,
      const unsigned end,
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
