#ifndef TRAIN_ALIGN_H
#define TRAIN_ALIGN_H

#include <vector>

#include "align/Alignment.h"

#include "util/Motion.h"

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

      Motion motion;
    };


    bool trainMightFit(
      const PeaksInfo& peaksInfo,
      const string& sensorCountry,
      const TrainDB& trainDB,
      const Alignment& match) const;

    void estimateAlignedMotion(
      const vector<float>& refPeaks,
      const vector<float>& times,
      const vector<unsigned>& actualToRef,
      const int offsetRef,
      Shift& shift) const;
      
    bool scalePeaks(
      const vector<float>& refPeaks,
      const unsigned numRefCars,
      const PeaksInfo& peaksInfo,
      Shift& shift,
      vector<float>& scaledPeaks) const;

    void NeedlemanWunsch(
      const vector<float>& refPeaks,
      const vector<float>& scaledPeaks,
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
      const PeaksInfo& peaksInfo,
      vector<Alignment>& matches) const;
};

#endif
