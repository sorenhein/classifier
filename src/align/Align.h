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
  friend class Crosses;
   
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
      const PeaksInfo& refInfo,
      const PeaksInfo& peaksInfo,
      Alignment& match,
      int& offsetRef) const;

    void storeResiduals(
      const vector<float>& x,
      const vector<float>& y,
      const unsigned lt,
      const float peakScale,
      Alignment& match) const;

    bool scalePeaks(
      const PeaksInfo& refInfo,
      const PeaksInfo& peaksInfo,
      Alignment& match,
      vector<float>& scaledPeaks) const;

    void initNeedlemanWunsch(
      const unsigned lreff,
      const unsigned lteff,
      vector<vector<Mentry>>& matrix) const;

    void fillNeedlemanWunsch(
      const vector<float>& refPeaks,
      const vector<float>& scaledPeaks,
      const Alignment& alignment,
      const unsigned lreff,
      const unsigned lteff,
      vector<vector<Mentry>>& matrix) const;

    void backtrackNeedlemanWunsch(
      const unsigned lreff,
      const unsigned lteff,
      const vector<vector<Mentry>>& matrix,
      Alignment& match) const;

    void NeedlemanWunsch(
      const vector<float>& refPeaks,
      const vector<float>& scaledPeaks,
      Alignment& alignment) const;

    void printAlignPeaks(
      const string& refTrain,
      const vector<float>& times,
      const vector<float>& refPeaks,
      const vector<float>& scaledPeaks) const;

    Motion const * getMatchingMotion(const string& trainName) const;

    void pos2time(
      const vector<float>& refPeaks,
      const vector<float>& posTrace,
      const Motion& motion,
      const float sampleRate,
      vector<float>& refTimes) const;

  protected:

    bool trainMightFitGeometrically(
      const PeaksInfo& peaksInfo,
      const Alignment& match) const;

    bool alignPeaks(
      const PeaksInfo& refInfo,
      const PeaksInfo& peaksInfo,
      Alignment& match) const;

    void regressTrain(
      const vector<float>& times,
      const vector<float>& refPeaks,
      const bool storeFlag,
      Alignment& match) const;


  public:

    Align();

    ~Align();

    bool realign(
      const Control& control,
      const TrainDB& trainDB,
      const string& country,
      const PeaksInfo& peaksInfo);

    void regress(
      const TrainDB& trainDB,
      const vector<float>& times);

    void getBest(
      string& trainDetected,
      float& distDetected) const;

    unsigned getMatchRank(const unsigned trainNoTrue) const;

    void updateStats() const;

    string strMatches(const string& title) const;

    string strRegress(const Control& control) const;

    string strMatchingResiduals(
      const string& trainTrue,
      const string& pickAny,
      const string& heading) const;

    string strDeviation() const;

    void writeTrain(
      const TrainDB& trainDB,
      const string& filename,
      const vector<float>& posTrace,
      const unsigned offset,
      const float sampleRate,
      const string& trainName) const;
};

#endif
