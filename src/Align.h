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

      vector<double> motion;
    };

    struct OverallShift
    {
      unsigned firstRefNo;
      unsigned firstTimeNo;
    };


    bool countTooDifferent(
      const vector<PeakTime>& times,
      const unsigned refCount) const;

    double interpolateTime(
      const vector<PeakTime>& times,
      const double index) const;

    void estimateAlignedMotion(
      const vector<double>& refPeaks,
      const vector<PeakTime>& times,
      const vector<int>& actualToRef,
      const int offsetRef,
      Shift& shift) const;
      
    void estimateMotion(
      const vector<double>& refPeaks,
      const vector<PeakTime>& times,
      Shift& shift) const;
      
    double simpleScore(
      const vector<double>& refPeaks,
      const vector<double>& shiftedPeaks) const;
      
    void makeShiftCandidates(
      vector<Shift>& candidates,
      const vector<OverallShift>& shifts,
      const unsigned lt,
      const unsigned lp,
      const vector<int>& actualToRef,
      const int offsetRef,
      const bool makeShiftFlag) const;

    bool betterSimpleScore(
      const double score,
      const unsigned index,
      const double bestScore,
      const unsigned bestIndex,
      const vector<Shift>& candidates,
      const unsigned lt) const;

    void scalePeaks(
      const vector<double>& refPeaks,
      const vector<PeakTime>& times,
      const vector<int>& actualToRef,
      const unsigned numFrontWheels,
      const bool fullTrainFlag,
      Shift& shift,
      vector<double>& scaledPeaks) const;

    void NeedlemanWunsch(
      const vector<double>& refPeaks,
      const vector<double>& scaledPeaks,
      const double peakScale,
      const Imperfections& imperf,
      const Shift& shift,
      Alignment& alignment) const;

    void printAlignPeaks(
      const string& refTrain,
      const vector<PeakTime>& times,
      const vector<double>& refPeaks,
      const vector<double>& scaledPeaks) const;


  public:

    Align();

    ~Align();

    void bestMatches(
      const vector<PeakTime>& times,
      const vector<int>& actualToRef,
      const unsigned numFrontWheels,
      const bool fullTrainFlag,
      const Imperfections& imperf,
      const Database& db,
      const string& country,
      const unsigned tops,
      const Control& control,
      vector<Alignment>& matches) const;
};

#endif
