#ifndef TRAIN_ALIGN_H
#define TRAIN_ALIGN_H

#include <vector>

#include "struct.h"

class Database;

using namespace std;


class Align
{
  private:

    void scalePeaks(
      const vector<PeakTime>& times,
      const double len, // In m
      vector<PeakPos>& scaledPeaks) const;

    void NeedlemanWunsch(
        const vector<PeakPos>& refPeaks,
        const vector<PeakPos>& scaledPeaks,
        Alignment& alignment) const;


  public:

    Align();

    ~Align();

    void bestMatches(
      const vector<PeakTime>& times,
      const Database& db,
      const vector<HistMatch>& matchesHist,
      const unsigned tops,
      vector<Alignment>& matches) const;
};

#endif
