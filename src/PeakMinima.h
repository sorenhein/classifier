#ifndef TRAIN_PEAKMINIMA_H
#define TRAIN_PEAKMINIMA_H

#include <vector>
#include <list>
#include <string>

#include "PeakPool.h"
#include "PeakPieces.h"
#include "PeakSeeds.h"

#include "struct.h"

using namespace std;

class Peak;
class PeakPool;
class Gap;


class PeakMinima
{
  private:

    typedef void (PeakMinima::*MinFncPtr)(Peak& p1, Peak& p2) const;

    PeakSeeds seeds;

    PeakPieces peakPieces;

    unsigned offset;


    void markWheelPair(
      Peak& p1,
      Peak& p2) const;

    void markBogieShortGap(
      Peak& p1,
      Peak& p2) const;

    void markBogieLongGap(
      Peak& p1,
      Peak& p2) const;

    void reseedWheelUsingQuality(PeakPool& peaks) const;

    void reseedBogiesUsingQuality(
      PeakPool& peaks,
      const vector<Peak>& bogieScale) const;

    void reseedLongGapsUsingQuality(
      PeakPool& peaks,
      const vector<Peak>& longGapScale) const;

    void makeBogieAverages(
      const PeakPool& peaks,
      vector<Peak>& wheels) const;

    void makeCarAverages(
      const PeakPool& peaks,
      vector<Peak>& wheels) const;

    void markSinglePeaks(
      PeakPool& peaks,
      const vector<Peak>& peakCenters) const;

    void markBogiesOfSelects(
      PeakPool& peaks,
      const PeakFncPtr& fptr,
      const Gap& wheelGap,
      Gap& actualGap) const;


    void fixBogieOrphans(PeakPool& peaks) const;

    void markBogies(
      PeakPool& peaks,
      Gap& wheelGap);


    unsigned markByDistance(
      PeakPool& peaks,
      const PeakFncPtr& fptr1,
      const PeakFncPtr& fptr2,
      const PeakFncPtr& qptr,
      const MinFncPtr& fptrMark,
      const Gap& gap) const;

    void markShortGaps(
      PeakPool& peaks,
      Gap& wheelGap,
      Gap& shortGap);

    void markGapsOfSelects(
      PeakPool& peaks,
      const PeakFncPtr& fptr1,
      const PeakFncPtr& fptr2,
      const MinFncPtr& fptrMark,
      const Gap& wheelGap) const;

    unsigned makeLabelsConsistent(
      PeakPool& peaks,
      const PeakFncPtr& wptr1,
      const PeakFncPtr& wptr2,
      const PeakFncPtr& bptrLeft,
      const PeakFncPtr& bptrRight,
      const BogieType bogieMatchLeft,
      const BogieType bogieMatchRight,
      const Gap& gap) const;

    void markLongGaps(
      PeakPool& peaks,
      const Gap& wheelGap,
      const unsigned shortGapCount,
      Gap& longGap);

    void makePieceList(
      const PeakPool& peaks,
      const PeakFncPtr& fptr1,
      const PeakFncPtr& fptr2,
      const PeakPairFncPtr& includePtr,
      const unsigned smallPieceLimit = 0);



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
      const float scale,
      const unsigned offsetIn);

};

#endif
