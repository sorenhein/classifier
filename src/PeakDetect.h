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


  // For putting together complete cars.

  struct Car
  {
    unsigned start; // Excluding offset
    unsigned end;

    bool leftGapSet;
    bool rightGapSet;

    unsigned leftGap;
    unsigned leftBogeyGap; // Zero if single wheel
    unsigned midGap;
    unsigned rightBogeyGap; // Zero if single wheel
    unsigned rightGap;

    unsigned noLeftGap;
    unsigned noRightGap;
    unsigned no;

    Peak * firstBogeyLeft;
    Peak * firstBogeyRight;
    Peak * secondBogeyLeft;
    Peak * secondBogeyRight;

    unsigned catStatIndex;

    void operator += (const Car& c2)
    {
      if (c2.leftGapSet)
      {
        leftGap += c2.leftGap;
        noLeftGap++;
      }

      if (c2.rightGapSet)
      {
        rightGap += c2.rightGap;
        noRightGap++;
      }

      midGap += c2.midGap;
      leftBogeyGap += c2.leftBogeyGap;
      rightBogeyGap += c2.rightBogeyGap;
      no++;
    };

    void avg()
    {
      if (noLeftGap)
        leftGap /= noLeftGap;
      if (noRightGap)
        rightGap /= noRightGap;
      if (no)
      {
        midGap /= no;
        leftBogeyGap /= no;
        rightBogeyGap /= no;
      }
    };

    string str()
    {
      stringstream ss;
      ss << setw(12) << left << "Left gap" << 
        setw(6) << right << leftGap << " (" << noLeftGap << ")\n";
      ss << setw(12) << left << "Left bogey" << 
        setw(6) << right << leftBogeyGap << " (" << no << ")\n";
      ss << setw(12) << left << "Mid gap" << 
        setw(6) << right << midGap << " (" << no << ")\n";
      ss << setw(12) << left << "Right bogey" << 
        setw(6) << right << rightBogeyGap << " (" << no << ")\n";
      ss << setw(12) << left << "Right gap" << 
        setw(6) << right << rightGap << " (" << noRightGap << ")\n";
      return ss.str();
    };
  };

  struct CarStat
  {
    Car carSum;
    Peak firstBogeyLeftSum;
    Peak firstBogeyRightSum;
    Peak secondBogeyLeftSum;
    Peak secondBogeyRightSum;

    Car carAvg;
    Peak firstBogeyLeftAvg;
    Peak firstBogeyRightAvg;
    Peak secondBogeyLeftAvg;
    Peak secondBogeyRightAvg;

    unsigned noFirstLeft;
    unsigned noFirstRight;
    unsigned noSecondLeft;
    unsigned noSecondRight;

    bool symmetryFlag;
    unsigned numWheels;
    unsigned count;

    void operator += (const Car& c)
    {
      carSum += c;
      count++;

      if (c.firstBogeyLeft)
      {
        firstBogeyLeftSum += * c.firstBogeyLeft;
        noFirstLeft++;
      }

      if (c.firstBogeyRight)
      {
        firstBogeyRightSum += * c.firstBogeyRight;
        noFirstRight++;
      }

      if (c.secondBogeyLeft)
      {
        secondBogeyLeftSum += * c.secondBogeyLeft;
        noSecondLeft++;
      }

      if (c.secondBogeyRight)
      {
        secondBogeyRightSum += * c.secondBogeyRight;
        noSecondRight++;
      }
    };

    void avg()
    {
      carAvg = carSum;
      carAvg.avg();

      if (noFirstLeft)
      {
        firstBogeyLeftAvg = firstBogeyLeftSum;
        firstBogeyLeftAvg /= noFirstLeft;
      }
      if (noFirstRight)
      {
        firstBogeyRightAvg = firstBogeyRightSum;
        firstBogeyRightAvg /= noFirstRight;
      }
      if (noSecondLeft)
      {
        secondBogeyLeftAvg = secondBogeyLeftSum;
        secondBogeyLeftAvg /= noSecondLeft;
      }
      if (noSecondRight)
      {
        secondBogeyRightAvg = secondBogeyRightSum;
        secondBogeyRightAvg /= noSecondRight;
      }
    };

    float distComponent(const unsigned a, const unsigned b) const
    {
      const float ratio = (a > b ? 
        static_cast<float>(a) / static_cast<float>(b) :
        static_cast<float>(b) / static_cast<float>(a));

      if (ratio <= 1.1f)
        return 0.f;
      else
        return 100.f * (ratio - 1.f) * (ratio - 1.f);

    };

    float distance(const Car& car) const
    {
      // Could also involve peak quality.

      float dist = 0.f;

      if (carAvg.leftGapSet && car.leftGapSet)
        dist += distComponent(carAvg.leftGap, car.leftGap);

      if (carAvg.rightGapSet && car.rightGapSet)
        dist += distComponent(carAvg.rightGap, car.rightGap);

      dist += distComponent(carAvg.leftBogeyGap, car.leftBogeyGap);
      dist += distComponent(carAvg.midGap, car.midGap);
      dist += distComponent(carAvg.rightBogeyGap, car.rightBogeyGap);

      return dist;
    }

    string str()
    {
      return carAvg.str();
    };
  };


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
    bool similarFlag;
    float quality; // Always >= 0, low is good
    float qualityShape; // Always >= 0, low is good

    bool wheelFlag;
    WheelType wheelSide;

    BogeyType bogeySide;

    bool spuriousFlag;
  };




    unsigned len;
    unsigned offset;
    list<Peak> peaks;
    PeakData scales;
    Peak scalesList;

    list<Peak> peaksNew;
    list<Peak> peaksNewer;

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

    void findFirstSize(
      const vector<unsigned>& dists,
      unsigned& lower,
      unsigned& upper) const;

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

    unsigned estimateQuietCluster(
      const vector<PeriodCluster>& clusters,
      const unsigned period) const;

    bool checkQuantity(
      const unsigned actual,
      const unsigned reference,
      const float factor,
      const string& text) const;

    bool matchesCarStat(
      const Car& car,
      const vector<CarStat>& carStats,
      unsigned& index) const;

    bool findFourWheeler(
      const unsigned start,
      const unsigned end,
      const bool leftGapPresent,
      const bool rightGapPresent,
      const vector<PeakEntry>& peaksAnnot,
      const vector<unsigned>& peakNos,
      const vector<unsigned>& peakIndices,
      const Car& carAvg,
      Car& car) const;

    void fixFourWheels(
      vector<PeakEntry>& peaksAnnot,
      const vector<unsigned>& peakIndices,
      const unsigned p0,
      const unsigned p1,
      const unsigned p2,
      const unsigned p3) const;

    void updateCars(
      vector<Car>& cars,
      vector<CarStat>& carStats,
      Car& car,
      const unsigned numWheels) const;

    bool findCars(
      const unsigned start,
      const unsigned end,
      const bool leftGapPresent,
      const bool rightGapPresent,
      vector<PeakEntry>& peaksAnnot,
      vector<Car>& cars,
      vector<CarStat>& carStats) const;

    void makeSynthPeaksSharp(vector<float>& synthPeaks) const;
    void makeSynthPeaksQuietNew(vector<float>& synthPeaks) const;
    void makeSynthPeaksQuiet(vector<float>& synthPeaks) const;
    void makeSynthPeaksLines(vector<float>& synthPeaks) const;
    void makeSynthPeaksPosLines(vector<float>& synthPeaks) const;
    void makeSynthPeaksClassical(vector<float>& synthPeaks) const;
    void makeSynthPeaksClassicalNewer(vector<float>& synthPeaks) const;

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
