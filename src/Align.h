#ifndef TRAIN_ALIGN_H
#define TRAIN_ALIGN_H

#include <vector>

#include "align/Alignment.h"

class TrainDB;
class Control;
struct PeaksInfo;

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

      vector<float> motion;
    };

    struct OverallShift
    {
      unsigned firstRefNo;
      unsigned firstTimeNo;
    };


    bool countTooDifferent(
      const vector<float>& times,
      const unsigned refCount) const;

    float interpolateTime(
      const vector<float>& times,
      const float index) const;

    void estimateAlignedMotion(
      const vector<float>& refPeaks,
      const vector<float>& times,
      const vector<int>& actualToRef,
      const int offsetRef,
      Shift& shift) const;
      
    void estimateMotion(
      const vector<float>& refPeaks,
      const vector<float>& times,
      Shift& shift) const;
      
    float simpleScore(
      const vector<float>& refPeaks,
      const vector<float>& shiftedPeaks) const;
      
    void makeShiftCandidates(
      vector<Shift>& candidates,
      const vector<OverallShift>& shifts,
      const unsigned lt,
      const unsigned lp,
      const vector<int>& actualToRef,
      const int offsetRef,
      const bool makeShiftFlag) const;

    bool betterSimpleScore(
      const float score,
      const unsigned index,
      const float bestScore,
      const unsigned bestIndex,
      const vector<Shift>& candidates,
      const unsigned lt) const;

    void scalePeaks(
      const vector<float>& refPeaks,
      const vector<float>& times,
      const vector<int>& actualToRef,
      const unsigned numFrontWheels,
      const bool fullTrainFlag,
      Shift& shift,
      vector<float>& scaledPeaks) const;

    void NeedlemanWunsch(
      const vector<float>& refPeaks,
      const vector<float>& scaledPeaks,
      const float peakScale,
      // const Imperfections& imperf,
      const Shift& shift,
      Alignment& alignment) const;

    void printAlignPeaks(
      const string& refTrain,
      const vector<float>& times,
      const vector<float>& refPeaks,
      const vector<float>& scaledPeaks) const;


  public:

    Align();

    ~Align();

    void bestMatches(
      const Control& control,
      const TrainDB& trainDB,
      const string& country,

      // const vector<float>& times,
      const vector<int>& actualToRef,
      const unsigned numFrontWheels,
      const bool fullTrainFlag,
      const PeaksInfo& peaksInfo,
      vector<Alignment>& matches) const;
};

#endif
