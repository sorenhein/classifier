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

    struct DistEntry
    {
      unsigned index;
      unsigned indexHi; // Sometimes set to denote a range
      int direction;
      unsigned origin;
      int count;
      int cumul;

      bool operator < (const DistEntry& de2)
      {
        return (index < de2.index);
      };
    };

    struct PieceEntry
    {
      unsigned modality; // Unimodal, bimodel etc.
      DistEntry summary;
      list<DistEntry> extrema;

      string str()
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


    unsigned offset;


    void makeDistances(
      const PeakPool& peaks,
      const PeakFncPtr& fptr,
      vector<unsigned>& dists) const;

    void makeSteps(
      const vector<unsigned>& dists,
      list<DistEntry>& steps) const;

    void summarizePiece(PieceEntry& pe) const;

    void makePieces(
      const list<DistEntry>& steps,
      list<PieceEntry>& pieces,
      DistEntry& summary) const;

    void eraseSmallPieces(
      list<PieceEntry>& pieces,
      DistEntry& summary) const;

    void eraseSmallMaxima(
      list<PieceEntry>& pieces,
      DistEntry& summary) const;

    void splitPiece(
      list<PieceEntry>& pieces,
      list<PieceEntry>::iterator pit,
      const unsigned indexLeft,
      const unsigned indexRight) const;

    bool splitPieceOnDip(
      list<PieceEntry>& pieces,
      list<PieceEntry>::iterator pit) const;

    bool splitPieceOnGap(
      list<PieceEntry>& pieces,
      list<PieceEntry>::iterator pit) const;

    void splitPieces(list<PieceEntry>& pieces) const;

    bool setGap(
      const PieceEntry& piece,
      Gap& gap) const;

    bool tripartite(
      const list<PieceEntry>& pieces,
      Gap& wheelGap,
      Gap& shortGap,
      Gap& longGap) const;

    void unjitterPieces(list<PieceEntry>& pieces) const;


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

    bool formBogieGap(
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
      const Gap& wheelGap) const;

    void markBogiesOfUnpaired(
      PeakPool& peaks,
      const Gap& wheelGap) const;

    void fixBogieOrphans(PeakPool& peaks) const;

    void markBogies(
      PeakPool& peaks,
      Gap& wheelGap) const;


    bool guessBogieDistance(
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
      const unsigned shortGapCount,
      Gap& longGap);


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
