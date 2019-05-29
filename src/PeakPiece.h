#ifndef TRAIN_PEAKPIECE_H
#define TRAIN_PEAKPIECE_H

#include <list>
#include <string>

#include "struct.h"

using namespace std;


class PeakPiece
{
  private:

    unsigned _modality; // Unimodal, bimodel etc.

    DistEntry _summary;

    list<DistEntry> _extrema;


  public:

    PeakPiece();

    ~PeakPiece();

    void reset();

    void logExtremum(
      const unsigned index,
      const int cumul,
      const int direction);

    bool operator < (const unsigned limit) const;
    bool operator > (const unsigned limit) const;

    void copySummaryFrom(const PeakPiece& piece2);

    void summarize();

    void eraseSmallMaxima(const unsigned limit);

    void splitOnIndices(
      const unsigned indexLeft,
      const unsigned indexRight,
      PeakPiece& piece2);

    bool splittableOnDip(
      unsigned& indexLeft,
      unsigned& indexRight) const;

    bool splittableOnGap(
      unsigned& indexLeft,
      unsigned& indexRight) const;

    void unjitter();

    unsigned modality() const;
    const DistEntry& summary() const;
    bool getGap(Gap& gap) const;

    string str() const;

};

#endif
