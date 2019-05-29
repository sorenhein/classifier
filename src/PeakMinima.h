#ifndef TRAIN_PEAKMINIMA_H
#define TRAIN_PEAKMINIMA_H

#include <vector>
#include <list>
#include <string>

#include "PeakPool.h"
#include "PeakPieces.h"

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

    struct PieceEntryOld
    {
      unsigned modality; // Unimodal, bimodel etc.
      DistEntry summary;
      list<DistEntry> extrema;

      string str() const
      {
        stringstream ss;
        ss << "Modality " << modality << "\n";
        for (auto& e: extrema)
          ss << "index " << e.index << 
          (e.indexHi == 0 ? "" : "-" + to_string(e.indexHi)) << ", " <<
          (e.direction == 1 ? "MAX" : "min") << ", " <<
          e.cumul << "\n";
        return ss.str();
      };
    };

    list<PeakPiece> pieces;
    DistEntry summary;

    PeakPieces peakPieces;


    unsigned offset;


    void makeSteps(
      const vector<unsigned>& dists,
      list<DistEntry>& steps) const;

    void makePieces(const list<DistEntry>& steps);

    void eraseSmallPieces();

    void eraseSmallMaxima();

    void splitPieces();

    void unjitterPieces();


    void findFirstLargeRange(
      const vector<unsigned>& dists,
      Gap& gap,
      const unsigned lowerCount = 0) const;

    bool formBogieGap(
      const Peak * p1,
      const Peak * p2) const;

    void markWheelPair(
      Peak& p1,
      Peak& p2,
      const string& text) const;

    void markBogieShortGap(
      Peak& p1,
      Peak& p2,
      PeakPool& peaks,
      PPLiterator& cit,
      PPLiterator& ncit,
      const string& text) const;

    void markBogieLongGap(
      Peak& p1,
      Peak& p2,
      const string& text) const;

    void reseedWheelUsingQuality(PeakPool& peaks) const;

    void reseedBogiesUsingQuality(
      PeakPool& peaks,
      const vector<Peak>& bogieScale) const;

    void reseedLongGapsUsingQuality(
      PeakPool& peaks,
      const vector<Peak>& longGapScale) const;

    void makeWheelAverage(
      const PeakPool& peaks,
      Peak& wheel) const;

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

    void updateGap(
      Gap& gap,
      const Gap& actualGap) const;

    void markBogies(
      PeakPool& peaks,
      Gap& wheelGap) const;


    void guessBogieDistance(Gap& wheelGap) const;

    void markShortGapsOfSelects(
      PeakPool& peaks,
      const Gap& wheelGap) const;

    void markShortGapsOfUnpaired(
      PeakPool& peaks,
      const Gap& wheelGap) const;

    void markLongGapsOfSelects(
      PeakPool& peaks,
      const Gap& wheelGap) const;

    bool hasStragglerBogies(
      const PeakPiece& piece,
      Gap& wheelGap) const;

    void markShortGaps(
      PeakPool& peaks,
      Gap& wheelGap,
      Gap& shortGap);


    void guessLongGapDistance(
      const PeakPool& peaks,
      const unsigned shortGapCount,
      Gap& gap) const;

    void markLongGaps(
      PeakPool& peaks,
      const Gap& wheelGap,
      const unsigned shortGapCount,
      Gap& longGap);

    void makePieceList(
      const PeakPool& peaks,
      const PeakPairFncPtr& includePtr);


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
