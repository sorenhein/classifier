#ifndef TRAIN_PEAKDETECT_H
#define TRAIN_PEAKDETECT_H

#include <vector>
#include <list>
#include <string>
#include <sstream>

#include "Peak.h"
#include "struct.h"

using namespace std;

class PeakStats;


class PeakDetect
{
  private:

    struct PeakCluster
    {
      Peak centroid;
      unsigned len;
      unsigned numConvincing;
      bool good;
    };

    struct Sharp
    {
      unsigned index;
      float level;
      float range1;
      float rangeRatio;
      float grad1;
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

      string strHeader() const
      {
        stringstream ss;
        ss << setw(6) << right << "Index" <<
          setw(8) << right << "Level" <<
          setw(8) << right << "Range1" <<
          setw(8) << right << "Ratio" <<
          setw(8) << right << "Grad1" <<
          setw(8) << right << "Ratio" <<
          setw(8) << right << "Fill1" <<
          setw(8) << right << "Ratio" << endl;
        return ss.str();
      };

      string str(const unsigned offset = 0) const
      {
        stringstream ss;
        ss << 
          setw(6) << right << index + offset <<
          setw(8) << fixed << setprecision(2) << level <<
          setw(8) << fixed << setprecision(2) << range1 <<
          setw(8) << fixed << setprecision(2) << rangeRatio <<
          setw(8) << fixed << setprecision(2) << grad1 <<
          setw(8) << fixed << setprecision(2) << gradRatio <<
          setw(8) << fixed << setprecision(2) << fill1 <<
          setw(8) << fixed << setprecision(2) << fillRatio << endl;
        return ss.str();
      };
    };

    struct SharpCluster
    {
      Sharp centroid;
      unsigned len;
      unsigned numConvincing;
      bool good;
    };


    unsigned len;
    unsigned offset;
    list<Peak> peaks;
    PeakData scales;
    Peak scalesList;

    list<Peak> peaksNew;

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

    void collapsePeaks(
      const list<Peak>::iterator peak1,
      const list<Peak>::iterator peak2);

    const list<Peak>::iterator collapsePeaksNew(
      const list<Peak>::iterator peak1,
      const list<Peak>::iterator peak2);

    void reduceSmallAreas(const float areaLimit);

    void eliminateKinks();

    float estimateScale(vector<float>& data) const;

    void estimateScales();

    void setOrigPointers();

    void eliminatePositiveMinima();
    void eliminateNegativeMaxima();

    void reducePositiveMaxima();
    void reduceNegativeMinima();

    void reducePositiveFlats();

    void markZeroCrossings();

    list<Peak>::iterator advanceZC(
      const list<Peak>::iterator peak,
      const unsigned indexNext) const;

    void markQuiet();

    void markSharpPeaksPos(
      list<Sharp>& sharps,
      Sharp sharpScale,
      vector<SharpCluster>& clusters,
      const bool debug);

    void markSharpPeaksNeg(
      list<Sharp>& sharps,
      Sharp sharpScale,
      vector<SharpCluster>& clusters,
      const bool debug);

    void markSharpPeaks();

    void countPositiveRuns() const;

    void pickStartingClusters(
      vector<PeakCluster>& clusters,
      const unsigned n) const;

    void pickStartingClustersSharp(
      const list<Sharp>& sharps,
      vector<SharpCluster>& clusters,
      const unsigned n) const;

    void runKmeansOnce(vector<PeakCluster>& clusters);
    void runKmeansOnceSharp(
      list<Sharp>& sharps,
      const Sharp& sharpScale,
      vector<SharpCluster>& clusters);

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

    PeakType classifyPeak(
      const Peak& peak,
      const vector<PeakCluster>& clusters) const;

    void countClusters(vector<PeakCluster>& clusters);

    unsigned getConvincingClusters(vector<PeakCluster>& clusters);

    float getFirstPeakTime() const;

    void makeSynthPeaksSharp(vector<float>& synthPeaks) const;
    void makeSynthPeaksQuiet(vector<float>& synthPeaks) const;
    void makeSynthPeaksLines(vector<float>& synthPeaks) const;
    void makeSynthPeaksPosLines(vector<float>& synthPeaks) const;
    void makeSynthPeaksClassical(vector<float>& synthPeaks) const;

    void printPeaks(const vector<PeakTime>& timesTrue) const;

    void printClustersDetail(
      const vector<PeakCluster>& clusters) const;

    void printClusters(
      const vector<PeakCluster>& clusters,
      const bool debugDetails) const;

    void printClustersSharpDetail(
      const list<Sharp>& sharps,
      const Sharp& sharpScale,
      const vector<SharpCluster>& clusters) const;
    
    void printSharpClusters(
      const list<Sharp>& sharps,
      const Sharp& sharpScale,
      const vector<SharpCluster>& clusters,
      const bool debugDetails) const;

  public:

    PeakDetect();

    ~PeakDetect();

    void reset();

    void log(
      const vector<float>& samples,
      const unsigned offsetSamples);

    void reduce();

    void reduceNew();

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
