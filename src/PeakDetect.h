#ifndef TRAIN_PEAKDETECT_H
#define TRAIN_PEAKDETECT_H

#include <vector>
#include <list>
#include <string>
#include <sstream>

#include "Peak.h"
#include "CarModels.h"
#include "CarDetect.h"
#include "CarPeaks.h"
#include "struct.h"

using namespace std;

class PeakStats;


class PeakDetect
{
  private:

    struct Sharp
    {
      unsigned index;
      float level;
      float range1;
      float range2;
      float rangeRatio;
      float grad1;
      float grad2;
      float gradRatio;
      float fill1;
      float fillRatio;
      unsigned clusterNo;

      Sharp()
      {
        index = 0;
        level = 0.f;
        range1 = 0.f;
        rangeRatio = 0.f;
        grad1 = 0.f;
        gradRatio = 0.f;
        fill1 = 0.f;
        fillRatio = 0.f;
        clusterNo = 0;
      };

      void operator += (const Sharp& s2)
      {
        level += s2.level;
        range1 += s2.range1;
        rangeRatio += s2.rangeRatio;
        grad1 += s2.grad1;
        gradRatio += s2.gradRatio;
        fill1 += s2.fill1;
        fillRatio += s2.fillRatio;
      };

      void operator /= (const unsigned n)
      {
        level /= n;
        range1 /= n;
        rangeRatio /= n;
        grad1 /= n;
        gradRatio /= n;
        fill1 /= n;
        fillRatio /= n;
      };

      float distance(const Sharp& s2, const Sharp& scale) const
      {
        const float ldiff = (level - s2.level) / scale.level;
        const float lrange = (range1 - s2.range1) / scale.range1;
        const float lrratio = (rangeRatio - s2.rangeRatio);
        const float lgrad = (grad1 - s2.grad1) / scale.grad1;
        const float lgratio = (gradRatio - s2.gradRatio);
        const float lfill = (fill1 - s2.fill1) / scale.fill1;
        const float lfratio = (fillRatio - s2.fillRatio);

        return ldiff * ldiff + 
          lrange * lrange + lrratio * lrratio + 
          lgrad * lgrad + lgratio * lgratio +
          lfill * lfill + lfratio * lfratio;
      };

      float distToScale(const Sharp& scale) const
      {
        // scale is supposed to be negative.  Large negative peaks are OK.
        const float ldiff = (level > scale.level ?
          (level - scale.level) / scale.level : 0.f);
        // ranges are always positive.
        const float lrange1 = (range1 < scale.range1 ? 
          (range1 - scale.range1) / scale.range1 : 0.f);
        const float lrange2 = (range2 < scale.range2 ?
          (range2 - scale.range2) / scale.range2 : 0.f);
        const float lgrad1 = (grad1 - scale.grad1) / scale.grad1;
        const float lgrad2 = (grad2 - scale.grad2) / scale.grad2;

        return ldiff * ldiff + 
          lrange1 * lrange1 +
          lrange2 * lrange2 +
          lgrad1 * lgrad1 +
          lgrad2 * lgrad2;
      };

      float distToScaleQ(const Sharp& scale) const
      {
        // Ranges are always positive.
        const float lrange1 = (range1 < 0.5f * scale.range1 ? 
          (range1 - scale.range1) / scale.range1 : 0.f);
        const float lrange2 = (range2 < 0.5f * scale.range2 ?
          (range2 - scale.range2) / scale.range2 : 0.f);
        // Gradients are always positive.
        const float lgrad1 = (grad1 < scale.grad1 ? 
          (grad1 - scale.grad1) / scale.grad1 : 0.f);
        const float lgrad2 = (grad2 < scale.grad2 ?
          (grad2 - scale.grad2) / scale.grad2 : 0.f);

        return
          lrange1 * lrange1 +
          lrange2 * lrange2 +
          lgrad1 * lgrad1 +
          lgrad2 * lgrad2;
      };


      string strHeader() const
      {
        stringstream ss;
        ss << setw(6) << right << "Index" <<
          setw(8) << right << "Level" <<
          setw(8) << right << "Range1" <<
          setw(8) << right << "Range2" <<
          // setw(8) << right << "Ratio" <<
          setw(8) << right << "Grad1" <<
          setw(8) << right << "Grad2" <<
          // setw(8) << right << "Ratio" <<
          setw(8) << right << "Fill1" <<
          setw(8) << right << "Ratio" << endl;
        return ss.str();
      };

      string strHeaderQ() const
      {
        stringstream ss;
        ss << setw(6) << right << "Index" <<
          setw(8) << right << "Level" <<
          setw(8) << right << "Range1" <<
          setw(8) << right << "Range2" <<
          setw(8) << right << "Grad1" <<
          setw(8) << right << "Grad2" <<
          setw(8) << right << "SQual" << 
          setw(8) << right << "Quality" << endl;
        return ss.str();
      };

      string str(const unsigned offset = 0) const
      {
        stringstream ss;
        ss << 
          setw(6) << right << index + offset <<
          setw(8) << fixed << setprecision(2) << level <<
          setw(8) << fixed << setprecision(2) << range1 <<
          setw(8) << fixed << setprecision(2) << range2 <<
          setw(8) << fixed << setprecision(2) << grad1 <<
          setw(8) << fixed << setprecision(2) << grad2 <<
          setw(8) << fixed << setprecision(2) << fill1 <<
          setw(8) << fixed << setprecision(2) << fillRatio << endl;
        return ss.str();
      };

      string strQ(
        const float quality, 
        const float qualityShape, 
        const unsigned offset = 0) const
      {
        stringstream ss;
        ss << 
          setw(6) << right << index + offset <<
          setw(8) << fixed << setprecision(2) << level <<
          setw(8) << fixed << setprecision(2) << range1 <<
          setw(8) << fixed << setprecision(2) << range2 <<
          setw(8) << fixed << setprecision(2) << grad1 <<
          setw(8) << fixed << setprecision(2) << grad2 <<
          setw(8) << fixed << setprecision(2) << qualityShape <<
          setw(8) << fixed << setprecision(2) << quality << endl;
        return ss.str();
      };
    };

  CarModels models;


  // Peaks between abutting interval lists tend to be large and thus
  // to be "real" peaks.

  enum WheelType
  {
    WHEEL_LEFT = 0,
    WHEEL_RIGHT = 1,
    WHEEL_ONLY = 2,
    WHEEL_SIZE = 3
  };

  enum BogeyType
  {
    BOGEY_LEFT = 0,
    BOGEY_RIGHT = 1,
    BOGEY_SIZE = 2
  };

  struct PeakEntry
  {
    Peak * peakPtr;
    Peak * nextPeakPtr;
    Peak * nextLargePeakPtr;
    Peak * prevLargePeakPtr;
    Sharp sharp;
    bool tallFlag;
    float quality; // Always >= 0, low is good
    float qualityShape; // Always >= 0, low is good

    bool wheelFlag;
    WheelType wheelSide;

    BogeyType bogeySide;
  };

  typedef bool (PeakDetect::*PeakFncPtr)(
    const PeakEntry& pe1, const PeakEntry& pe2) const;


    unsigned len;
    unsigned offset;
    list<Peak> peaks;
    PeakData scales;
    Peak scalesList;

    unsigned numCandidates;
    unsigned numTentatives;


    float integrate(
      const vector<float>& samples,
      const unsigned i0,
      const unsigned i1) const;

    void annotate();

    bool check(const vector<float>& samples) const;

    void logFirst(const vector<float>& samples);
    void logLast(const vector<float>& samples);

    const list<Peak>::iterator collapsePeaks(
      const list<Peak>::iterator peak1,
      const list<Peak>::iterator peak2);

    void reduceSmallRanges(
      const float rangeLimit,
      const bool preserveFlag);
    void reduceSmallAreas(const float areaLimit);

    void eliminateKinks();

    void estimateScales();

    void setOrigPointers();

    void findFirstSize(
      const vector<unsigned>& dists,
      unsigned& lower,
      unsigned& upper,
      unsigned& counted,
      const unsigned lowerCount = 0) const;

    void pos2time(
      const vector<PeakPos>& posTrue,
      const double speed,
      vector<PeakTime>& timesTrue) const;

    bool advance(list<Peak>::iterator& peak) const;

    double simpleScore(
      const vector<PeakTime>& timesTrue,
      const double offsetScore,
      const bool logFlag,
      double& shift);

    void setOffsets(
      const vector<PeakTime>& timesTrue,
      vector<double>& offsetList) const;

    bool findMatch(
      const vector<PeakTime>& timesTrue,
      double& shift);

    PeakType findCandidate(
      const double time,
      const double shift) const;

    float getFirstPeakTime() const;

    bool checkQuantity(
      const unsigned actual,
      const unsigned reference,
      const float factor,
      const string& text) const;

    bool matchesModel(
      const CarDetect& car,
      unsigned& index) const;

    bool findFourWheeler(
      const unsigned start,
      const unsigned end,
      const bool leftGapPresent,
      const bool rightGapPresent,
      const vector<PeakEntry>& peaksAnnot,
      const vector<unsigned>& peakNos,
      const vector<unsigned>& peakIndices,
      CarDetect& car) const;

    bool findLastTwoOfFourWheeler(
      const unsigned start,
      const unsigned end,
      const bool rightGapPresent,
      const vector<PeakEntry>& peaksAnnot,
      const vector<unsigned>& peakNos,
      const vector<unsigned>& peakIndices,
      CarDetect& car) const;

    bool findLastThreeOfFourWheeler(
      const unsigned start,
      const unsigned end,
      const vector<PeakEntry>& peaksAnnot,
      const vector<unsigned>& peakNos,
      const vector<unsigned>& peakIndices,
      CarDetect& car,
      unsigned& numWheels) const;

    void fixTwoWheels(
      vector<PeakEntry>& peaksAnnot,
      const vector<unsigned>& peakIndices,
      const unsigned p0,
      const unsigned p1) const;

    void fixThreeWheels(
      vector<PeakEntry>& peaksAnnot,
      const vector<unsigned>& peakIndices,
      const unsigned p0,
      const unsigned p1,
      const unsigned p2) const;

    void fixFourWheels(
      vector<PeakEntry>& peaksAnnot,
      const vector<unsigned>& peakIndices,
      const unsigned p0,
      const unsigned p1,
      const unsigned p2,
      const unsigned p3) const;

    void updateCars(
      vector<CarDetect>& cars,
      CarDetect& car);

    bool findCars(
      const unsigned start,
      const unsigned end,
      const bool leftGapPresent,
      const bool rightGapPresent,
      vector<PeakEntry>& peaksAnnot,
      vector<CarDetect>& cars);

    bool bothTall(
      const PeakEntry& pe1,
      const PeakEntry& pe2) const;

    bool areBogeyGap(
      const PeakEntry& pe1,
      const PeakEntry& pe2) const;

    void guessDistance(
      const vector<PeakEntry>& peaksAnnot,
      const PeakFncPtr fptr,
      unsigned& distLower,
      unsigned& distUpper,
      unsigned& count) const;

    unsigned countWheels(const vector<PeakEntry>& peaksAnnot) const;

    void printCarStats(const string& text) const;

    void printCars(
      const vector<CarDetect>& cars,
      const string& text) const;

    void printPeaks(const vector<PeakTime>& timesTrue) const;

  public:

    PeakDetect();

    ~PeakDetect();

    void reset();

    void log(
      const vector<float>& samples,
      const unsigned offsetSamples);

    void reduce();

    void reduceNew();
    void reduceNewer();

    void logPeakStats(
      const vector<PeakPos>& posTrue,
      const string& trainTrue,
      const double speedTrue,
      PeakStats& peakStats);

    void makeSynthPeaks(vector<float>& synthPeaks) const;

    void getPeakTimes(vector<PeakTime>& times) const;

    void print() const;
};

#endif
