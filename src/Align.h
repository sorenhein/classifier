#ifndef TRAIN_ALIGN_H
#define TRAIN_ALIGN_H

#include <vector>

#include "struct.h"

class Database;

using namespace std;


class Align
{
  private:

    void scaleToRef(
      const vector<PeakTime>& times,
      const vector<PeakPos>& refPeaks,
      const int left,
      const int right,
      vector<PeakPos>& positions) const;

    void getAlignment(
      const vector<PeakPos>& refPeaks,
      const vector<PeakPos>& positions,
      Alignment& alignment) const;


  public:

    Align();

    ~Align();

    void bestMatches(
      const vector<PeakTime>& times,
      const Database& db,
      const unsigned maxFronts,
      const vector<int>& matchesHist,
      const unsigned tops,
      vector<Alignment>& matches) const;
    
    void print(const string& fname) const;
};

#endif
