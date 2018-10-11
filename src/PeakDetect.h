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

    struct Period
    {
      unsigned start;
      unsigned len;
      float lenScaled;
      float depth;
      float minLevel;
      unsigned clusterNo;

      unsigned scoreLeft;
      unsigned scoreRight;
      unsigned scoreLowest;
      bool fitsPeriodFlag;

      Period()
      {
        start = 0;
        len = 0;
        lenScaled = 0.f;
        depth = 0.f;
        minLevel = 0.f;
        clusterNo = 0;
      };

      void operator += (const Period& p2)
      {
        start += p2.start;
        len += p2.len;
        lenScaled += p2.lenScaled;
        depth += p2.depth;
      };

      void operator /= (const unsigned n)
      {
        start /= n;
        len /= n;
        lenScaled /= n;
        depth /= n;
      };

      float distance(const Period& p2) const
      {
        const float ldiff = lenScaled - p2.lenScaled;
        const float ldepth = depth - p2.depth;

        return ldiff * ldiff + ldepth * ldepth;
      };

      float distance2(const Period& p2) const
      {
        const float ldiff = 
          static_cast<float>(len) - static_cast<float>(p2.len);
        return ldiff * ldiff;
      };

      string strHeader() const
      {
        stringstream ss;
        ss << setw(6) << right << "Start" <<
          setw(8) << right << "Length" <<
          setw(8) << right << "Lnorm" <<
          setw(8) << right << "Dnorm" << endl;
        return ss.str();
      };

      string str(const unsigned offset = 0) const
      {
        stringstream ss;
        ss << 
          setw(6) << right << start + offset <<
          setw(8) << len <<
          setw(8) << fixed << setprecision(2) << lenScaled <<
          setw(8) << fixed << setprecision(2) << depth << endl;
        return ss.str();
      };

    };

    struct PeriodCluster
    {
      Period centroid;
      unsigned len;
      unsigned numConvincing;
      bool good;
      unsigned count;
      unsigned periodDominant;
      unsigned periodMedianDev;
      unsigned periodLargestDev;
      unsigned numPeriodics;
      unsigned lenMedian;
      unsigned lenCloseCount;
      unsigned lenMedianDev;
      unsigned lenLargestDev;
      unsigned duplicates;
      float depthAvg;
    };

    list<Period> quietCandidates;
    unsigned quietFavorite;


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

    const list<Peak>::iterator collapsePeaks(
      const list<Peak>::iterator peak1,
      const list<Peak>::iterator peak2);

    const list<Peak>::iterator collapsePeaksNew(
      const list<Peak>::iterator peak1,
      const list<Peak>::iterator peak2);

    void reduceSmallRanges(const float rangeLimit);
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

    void markPossibleQuiet();

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

    void pickStartingClustersQuiet(
      list<Period>& quiets,
      vector<PeriodCluster>& clusters,
      const unsigned n) const;

    void runKmeansOnce(vector<PeakCluster>& clusters);
    void runKmeansOnceSharp(
      list<Sharp>& sharps,
      const Sharp& sharpScale,
      vector<SharpCluster>& clusters);
    void runKmeansOnceQuiet(
      list<Period>& quiets,
      vector<PeriodCluster>& clusters,
      const unsigned csize,
      const bool reclusterFlag);

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

    unsigned countDuplicateUses(
      const list<Period>& quiets,
      const unsigned cno) const;

    void estimatePeriodicity(
      vector<unsigned>& spacings,
      unsigned& period,
      unsigned& numMatches) const;

    bool clustersCompatible(
      const vector<Period *>& intervals1,
      const vector<Period *>& intervals2) const;

    void findCompatibles(
      const vector<vector<Period *>>& intervals,
      vector<vector<unsigned>>& compatibles) const;

    unsigned promisingCluster(
      vector<vector<Period *>>& intervals,
      vector<PeriodCluster>& clusters,
      const unsigned clen);

    void removeOverlongIntervals(
      list<Period>& quiets,
      vector<vector<Period *>>& intervals,
      const unsigned period);

    void moveIntervalsWithLength(
      vector<vector<Period *>>& intervals,
      vector<PeriodCluster>& clusters,
      const unsigned clenOrig,
      const unsigned lenTarget);

    void hypothesizeIntervals(
      const vector<Period *>& cintervals,
      const unsigned period,
      const unsigned lenMedian,
      const unsigned lastSampleNo,
      vector<Period>& hypints) const;

    void findFillers(
      vector<vector<Period *>>& intervals,
      const unsigned period,
      const unsigned clusterSampleLen,
      const unsigned lastSampleNo,
      const unsigned clen,
      const unsigned cfill);

    unsigned largestValue(const list<Period>& quiets) const;

    unsigned intervalDist(
      const Period * p1,
      const Period * p2) const;

    void recalcClusters(
      const list<Period>& quiets,
      vector<PeriodCluster>& clusters);

    void recalcCluster(
      const list<Period>& quiets,
      vector<PeriodCluster>& clusters,
      const unsigned cno);

    unsigned intervalDistance(
      const Period * int1,
      const Period * int2,
      const unsigned period) const;

    void labelIntervalLists(
      vector<vector<Period *>>& intervals,
      const unsigned period);

    bool matchesIntervalList(
      const vector<Period *>& cint,
      const Period * interval,
      const unsigned period,
      unsigned& matchingPos,
      unsigned& matchingDist) const;

    void purifyIntervalLists(
      vector<PeriodCluster>& clusters,
      vector<vector<Period *>>& intervals,
      const unsigned period);

    void setQuietMedians(
      list<Period>& quiets,
      vector<PeriodCluster>& clusters,
      vector<vector<Period *>>& intervals);

    void makeSynthPeaksSharp(vector<float>& synthPeaks) const;
    void makeSynthPeaksQuietNew(vector<float>& synthPeaks) const;
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


    void printClustersQuietDetail(
      const list<Period>& quiets,
      const vector<PeriodCluster>& clusters) const;
    
    void printQuietClusters(
      const list<Period>& quiets,
      const vector<PeriodCluster>& clusters,
      const bool debugDetails) const;

    void printSelectClusters(
      const vector<PeriodCluster>& clusters,
      const unsigned period) const;

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
