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
typedef list<Peak *> PeakPtrList;
typedef PeakPtrList::const_iterator PeakCit;


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
      const PeakPtrList& candidates,
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
      PeakCit& cit,
      PeakCit& ncit,
      PeakCit& cbegin,
      PeakCit& cend,
      const string& text) const;

    void markBogeyLongGap(
      Peak& p1,
      Peak& p2,
      const string& text) const;

    void reseedWheelUsingQuality(
      PeakPool& peaks,
      PeakPtrList& candidates) const;

    void reseedBogeysUsingQuality(
      PeakPool& peaks,
      PeakPtrList& candidates,
      const vector<Peak>& bogeyScale) const;

    void reseedLongGapsUsingQuality(
      PeakPool& peaks,
      PeakPtrList& candidates,
      const vector<Peak>& longGapScale) const;

    void makeWheelAverage(
      PeakPool& peaks,
      PeakPtrList& candidates,
      Peak& wheel) const;

    void makeBogeyAverages(
      PeakPool& peaks,
      PeakPtrList& candidates,
      vector<Peak>& wheels) const;

    void makeCarAverages(
      PeakPool& peaks,
      PeakPtrList& candidates,
      vector<Peak>& wheels) const;

    void setCandidates(
      PeakPool& peaks,
      PeakPtrList& candidates) const;

    void markSinglePeaks(
      PeakPool& peaks,
      list<Peak *>& candidates) const;

    PeakCit nextWithProperty(
      PeakCit& it,
      PeakCit& endList,
      const PeakFncPtr fptr) const;

    PeakCit prevWithProperty(
      PeakCit& it,
      PeakCit& beginList,
      const PeakFncPtr fptr) const;


    void markBogeysOfSelects(
      PeakPool& peaks,
      PeakPtrList& candidates,
      const Gap& wheelGap) const;

    void markBogeysOfUnpaired(
      PeakPool& peaks,
      PeakPtrList& candidates,
      const Gap& wheelGap) const;

    void markBogeys(
      PeakPool& peaks,
      PeakPtrList& candidates,
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
      PeakPtrList& candidates,
      Gap& shortGap);


    void guessLongGapDistance(
      const PeakPool& peaks,
      const unsigned shortGapCount,
      Gap& gap) const;

    void markLongGaps(
      PeakPool& peaks,
      PeakPtrList& candidates,
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
      PeakPtrList& candidates,
      const unsigned offsetIn);

};

#endif
