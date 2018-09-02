#ifndef TRAIN_ALIGN_H
#define TRAIN_ALIGN_H

#include <vector>

#include "struct.h"

class Database;

using namespace std;


class Align
{
  private:

    struct Shift
    {
      unsigned firstRefNo;
      unsigned firstTimeNo;
      int firstHalfNetInsert;
      int secondHalfNetInsert;
    };


    bool countTooDifferent(
      const vector<PeakTime>& times,
      const unsigned refCount) const;

    double interpolateTime(
      const vector<PeakTime>& times,
      const double index) const;

    void estimateMotion(
      const vector<PeakPos>& refPeaks,
      const vector<PeakTime>& times,
      const unsigned firstRefNo,
      const unsigned firstTimeNo,
      double& speed,
      double& accel) const;
      
    double simpleScore(
      const vector<PeakPos>& refPeaks,
      const vector<PeakPos>& shiftedPeaks) const;
      
    void scalePeaks(
      const vector<PeakPos>& refPeaks,
      const vector<PeakTime>& times,
      Shift& shift,
      vector<PeakPos>& scaledPeaks) const;

    void NeedlemanWunsch(
      const vector<PeakPos>& refPeaks,
      const vector<PeakPos>& scaledPeaks,
      const double peakScale,
      Alignment& alignment) const;

  public:

    Align();

    ~Align();

    void bestMatches(
      const vector<PeakTime>& times,
      const Database& db,
      const string& country,
      const unsigned tops,
      const bool writeFlag,
      vector<Alignment>& matches) const;
};

#endif
