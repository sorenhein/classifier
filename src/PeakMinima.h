#ifndef TRAIN_PEAKMINIMA_H
#define TRAIN_PEAKMINIMA_H

#include <vector>
#include <list>
#include <string>

#include "PeakPool.h"

#include "struct.h"

using namespace std;

class Peak;
class PeakPool;


class PeakMinima
{
  private:

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

    bool bothPlausible(
      const Peak * p1,
      const Peak * p2) const;

    bool formBogeyGap(
      const Peak * p1,
      const Peak * p2) const;

    bool guessNeighborDistance(
      const PeakPool& peaks,
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
      PeakPool& peaks,
      PPiterator& cit,
      PPiterator& ncit,
      const string& text) const;

    void markBogeyLongGap(
      Peak& p1,
      Peak& p2,
      const string& text) const;

    void reseedWheelUsingQuality(PeakPool& peaks) const;

    void reseedBogeysUsingQuality(
      PeakPool& peaks,
      const vector<Peak>& bogeyScale) const;

    void reseedLongGapsUsingQuality(
      PeakPool& peaks,
      const vector<Peak>& longGapScale) const;

    void makeWheelAverage(
      const PeakPool& peaks,
      Peak& wheel) const;

    void makeBogeyAverages(
      const PeakPool& peaks,
      vector<Peak>& wheels) const;

    void makeCarAverages(
      const PeakPool& peaks,
      vector<Peak>& wheels) const;

    void markSinglePeaks(
      PeakPool& peaks,
      const vector<Peak>& peakCenters) const;


    void markBogeysOfSelects(
      PeakPool& peaks,
      const Gap& wheelGap) const;

    void markBogeysOfUnpaired(
      PeakPool& peaks,
      const Gap& wheelGap) const;

    void markBogeys(
      PeakPool& peaks,
      Gap& wheelGap) const;


    void markShortGapsOfSelects(
      PeakPool& peaks,
      const Gap& wheelGap) const;

    void markShortGapsOfUnpaired(
      PeakPool& peaks,
      const Gap& wheelGap) const;

    void markLongGapsOfSelects(
      PeakPool& peaks,
      const Gap& wheelGap) const;

    void markShortGaps(
      PeakPool& peaks,
      Gap& shortGap);


    void guessLongGapDistance(
      const PeakPool& peaks,
      const unsigned shortGapCount,
      Gap& gap) const;

    void markLongGaps(
      PeakPool& peaks,
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


  public:

    PeakMinima();

    ~PeakMinima();

    void reset();

    void mark(
      PeakPool& peaks,
      const vector<Peak>& peakCenters,
      const unsigned offsetIn);

};

#endif
