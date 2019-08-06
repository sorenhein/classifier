#ifndef TRAIN_ALIGN_H
#define TRAIN_ALIGN_H

#include <vector>
#include <string>

#include "../align/Alignment.h"

#include "../util/Motion.h"

using namespace std;

class Control;
class TrainDB;
struct PeaksInfo;


class Align
{
  private:

    // Used in Needleman-Wunsch.
    enum Origin
    {
      NW_MATCH = 0,
      NW_DELETE = 1,
      NW_INSERT = 2
    };

    struct Mentry
    {
      float dist;
      Origin origin;
    };


    vector<Alignment> matches;

    
    bool trainMightFit(
      const PeaksInfo& peaksInfo,
      const string& sensorCountry,
      const TrainDB& trainDB,
      const Alignment& match) const;

    bool alignFronts(
      const unsigned numRefCars,
      const PeaksInfo& peaksInfo,
      Alignment& match,
      int& offsetRef) const;

    bool scalePeaks(
      const vector<float>& refPeaks,
      const unsigned numRefCars,
      const PeaksInfo& peaksInfo,
      Alignment& match,
      vector<float>& scaledPeaks) const;

    void initNeedlemanWunsch(
      const unsigned lreff,
      const unsigned lteff,
      vector<vector<Mentry>>& matrix) const;

    void NeedlemanWunsch(
      const vector<float>& refPeaks,
      const vector<float>& scaledPeaks,
      Alignment& alignment) const;

    void storeResiduals(
      const vector<float>& x,
      const vector<float>& y,
      const unsigned lt,
      const float peakScale,
      Alignment& match) const;

    void printAlignPeaks(
      const string& refTrain,
      const vector<float>& times,
      const vector<float>& refPeaks,
      const vector<float>& scaledPeaks) const;


  public:

    Align();

    ~Align();

    void specificMatch(
      const vector<float>& times,
      const vector<float>& refPeaks,
      const bool storeFlag,
      Alignment& match) const;

    void bestMatches(
      const Control& control,
      const TrainDB& trainDB,
      const string& country,
      const PeaksInfo& peaksInfo);

    void bestMatch(
      const TrainDB& trainDB,
      const vector<float>& times);

    bool empty() const;

    void getBest(
      const unsigned& trainNoTrue,
      string& trainDetected,
      float& distDetected,
      unsigned& rankDetected) const;

    string strMatches(const string& title) const;

    string str(const Control& control) const;

    string strMatchingResiduals(
      const string& trainTrue,
      const string& pickAny,
      const string& heading) const;
};

#endif
