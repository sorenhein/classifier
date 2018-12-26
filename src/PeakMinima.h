#ifndef TRAIN_PEAKMINIMA_H
#define TRAIN_PEAKMINIMA_H

#include <vector>
#include <list>
#include <string>

#include "struct.h"

using namespace std;

class Peak;
typedef list<Peak *> PeakPtrList;
typedef PeakPtrList::const_iterator PeakCit;


class PeakMinima
{
  private:

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
      const PeakPtrList& candidates,
      const PeakFncPtr fptr) const;

    bool guessNeighborDistance(
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

    void reseedWheelUsingQuality(PeakPtrList& candidates) const;

    void reseedBogeysUsingQuality(
      PeakPtrList& candidates,
      const vector<Peak>& bogeyScale) const;

    void reseedLongGapsUsingQuality(
      PeakPtrList& candidates,
      const vector<Peak>& longGapScale) const;

    void makeWheelAverage(
      PeakPtrList& candidates,
      Peak& wheel) const;

    void makeBogeyAverages(
      PeakPtrList& candidates,
      vector<Peak>& wheels) const;

    void makeCarAverages(
      PeakPtrList& candidates,
      vector<Peak>& wheels) const;

    void setCandidates(
      list<Peak>& peaks,
      PeakPtrList& candidates) const;

    void markSinglePeaks(list<Peak *>& candidates) const;

    PeakCit nextWithProperty(
      PeakCit& it,
      PeakCit& endList,
      const PeakFncPtr fptr) const;

    PeakCit prevWithProperty(
      PeakCit& it,
      PeakCit& beginList,
      const PeakFncPtr fptr) const;


    void markBogeysOfSelects(
      PeakPtrList& candidates,
      const Gap& wheelGap) const;

    void markBogeysOfUnpaired(
      PeakPtrList& candidates,
      const Gap& wheelGap) const;

    void markBogeys(
      PeakPtrList& candidates,
      Gap& wheelGap) const;


    void markShortGapsOfSelects(
      PeakPtrList& candidates,
      const Gap& wheelGap) const;

    void markShortGapsOfUnpaired(
      PeakPtrList& candidates,
      const Gap& wheelGap) const;

    void markLongGapsOfSelects(
      PeakPtrList& candidates,
      const Gap& wheelGap) const;

    void markShortGaps(
      PeakPtrList& candidates,
      Gap& shortGap);


    void guessLongGapDistance(
      const PeakPtrList& candidates,
      const unsigned shortGapCount,
      Gap& gap) const;

    void markLongGaps(
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

    void printAllCandidates(
      const PeakPtrList& candidates,
      const string& text) const;

    void printSelected(
      const PeakPtrList& candidates,
      const string& text) const;


  public:

    PeakMinima();

    ~PeakMinima();

    void reset();

    void mark(
      list<Peak>& peaks,
      PeakPtrList& candidates,
      const unsigned offsetIn);

};

#endif
