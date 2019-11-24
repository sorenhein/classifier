#ifndef TRAIN_ALIGN_H
#define TRAIN_ALIGN_H

#include <vector>
#include <list>
#include <string>

#include "DynProg.h"
#include "../PeakGeneral.h"
#include "Alignment.h"

#include "../util/Motion.h"

using namespace std;

class Control;
class TrainDB;
struct PeaksInfo;


class Align
{
  friend class Crosses;
   
  private:

    vector<Alignment> matches;

    DynProg dynprog;

    
    bool trainMightFit(
      const PeaksInfo& peaksInfo,
      const string& sensorCountry,
      const TrainDB& trainDB,
      const Alignment& match) const;

    bool promisingPartial(
      const vector<int>& actualToRef,
      const bool allowAddFlag) const;

    bool plausibleCounts(const Alignment& match) const;

    unsigned getGoodCount(
      const vector<int>& actualToRef) const;

    bool alignFronts(
      const PeaksInfo& refInfo,
      const PeaksInfo& peaksInfo,
      Alignment& match,
      int& offsetRef) const;

    void storeResiduals(
      const vector<float>& x,
      const vector<float>& y,
      const unsigned lt,
      Alignment& match) const;
    
    void distributeBogies(
      const list<BogieTimes>& bogieTimes,
      const list<BogieTimes>::const_iterator& bitStart,
      const float posOffset,
      const float speed,
      vector<float>& scaledPeaks) const;

    bool scalePeaks(
      const PeaksInfo& refInfo,
      const PeaksInfo& peaksInfo,
      Alignment& match,
      vector<float>& scaledPeaks) const;

    bool bootstrapMotion(
      const PeaksInfo& refInfo,
      const list<BogieTimes>& bogieTimes,
      Alignment& match) const;

    bool scaleLastBogies(
      const PeaksInfo& refInfo,
      const list<BogieTimes>& bogieTimes,
      const unsigned numBogies,
      vector<float>& scaledPeaks) const;

    void getPartialMatch(
      const vector<float>& times,
      const vector<float>& scaledPeaks,
      const unsigned scaledBackCount,
      const vector<float>& refPositions,
      Alignment& match) const;

    void setupDynRun(
      const unsigned refSize,
      const unsigned seenSize,
      vector<float>& penaltyFactor,
      Alignment& match) const;

    void printMatch(
      const Alignment& match,
      const unsigned len,
      const string& text) const;

    void printVector(
      const vector<float>& v,
      const string& text) const;

    void alignIteration(
      const vector<float>& refPositions,
      const DynamicPenalties& penalties,
      const vector<float>& timesSeen,
      const list<BogieTimes>& bogieTimes,
      const unsigned scaledCount,
      const string& text,
      vector<float>& penaltyFactor,
      vector<float>& scaledPeaks,
      Alignment& match);

    Motion const * getMatchingMotion(const string& trainName) const;

    void getBoxTraces(
      const TrainDB& trainDB,
      const Alignment& match,
      const unsigned offset,
      const float sampleRate,
      vector<unsigned>& refTimes,
      vector<unsigned>& refPeakTypes,
      vector<float>& refGrades,
      vector<float>& refPeaks) const;

    void writeTrainBox(
      const Control& control,
      const TrainDB& trainDB,
      const Alignment& match,
      const string& dir,
      const string& filename,
      const unsigned offset,
      const float sampleRate) const;


  protected:

    bool trainMightFitGeometrically(
      const PeaksInfo& peaksInfo,
      const Alignment& match) const;

    bool alignPeaks(
      const PeaksInfo& refInfo,
      const PeaksInfo& peaksInfo,
      Alignment& match) const;

    void regressPosLinear(
      const vector<float>& x,
      const vector<float>& y,
      vector<float>& estimate) const;

    void regressPosQuadratic(
      const vector<float>& x,
      const vector<float>& y,
      const unsigned lcommon,
      vector<float>& estimate) const;

    void regressTrain(
      const vector<float>& times,
      const vector<float>& refPeaks,
      const bool accelFlag,
      const bool storeFlag,
      Alignment& match) const;

    void stopAtLargeJump(Alignment& match);

    unsigned cutoff(Alignment& match);

    bool iterate(
      const vector<float>& refPositions,
      const DynamicPenalties& penalties,
      const vector<float>& timesSeen,
      const list<BogieTimes>& bogieTimes,
      Alignment& match,
      unsigned& numPlausible);



  public:

    Align();

    ~Align();

    bool realign(
      const TrainDB& trainDB,
      const string& country,
      const PeaksInfo& peaksInfo);

    bool realign(
      const TrainDB& trainDB,
      const string& country,
      const list<BogieTimes>& bogieTimes);

    void regress(
      const TrainDB& trainDB,
      const PeaksInfo& peaksInfo,
      const bool topFlag = false);

    const Alignment& best() const;

    void getBest(
      string& trainDetected,
      float& distDetected) const;

    unsigned getMatchRank(const unsigned trainNoTrue) const;

    void updateStats() const;

    void writeTrain(
      const TrainDB& trainDB,
      const string& filename,
      const vector<float>& posTrace,
      const unsigned offset,
      const float sampleRate,
      const string& trainName) const;

    void writeTrainBoxes(
      const Control& control,
      const TrainDB& trainDB,
      const string& filename,
      const unsigned offset,
      const float sampleRate,
      const string& trueTrainName) const;

    string strMatches(const string& title) const;

    string strRegress(const Control& control) const;

    string strMatchingResiduals(
      const string& trainTrue,
      const string& pickAny,
      const string& heading) const;

    string strDeviation() const;
};

#endif
