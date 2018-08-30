#ifndef TRAIN_PEAKCLUSTER_H
#define TRAIN_PEAKCLUSTER_H

#include <vector>
#include <string>

#include "struct.h"

using namespace std;


class PeakCluster
{
  private:

    struct ClusterStat
    {
      unsigned num;
      float sum;
      float sumsq;
      float mean;
      float sdev;

      ClusterStat() 
      { num = 0; sum = 0.f; sumsq = 0.f; mean = 0.f; sdev = 0.f; }

      void operator +=(const float val) 
      { num++; sum += val; sumsq += val*val; }

      void calc() { 
        if (num <= 1) return;
        mean = sum / num; 
        sdev = sqrt((num*sumsq - sum*sum) / (num * (num-1))); }
    };

    struct PeakStat
    {
      ClusterStat left;
      ClusterStat right;
      ClusterStat ratio;

      void operator +=(const PeakData& peak)
      {
        left += peak.left.area; 
        right += peak.right.area;
        ratio += peak.left.area / peak.right.area;
      }

      void calc() { left.calc(); right.calc(); ratio.calc(); }
    };


    PeakStat similar;
    PeakStat leftHeavy;
    PeakStat rightHeavy;

    bool newFlag;

    void calc();

    void print(
      const ClusterStat& cstat,
      const string& title) const;

    void print(
      const PeakStat& pstat,
      const string& title) const;


  public:

    PeakCluster();

    ~PeakCluster();

    void reset();

    void operator +=(const PeakData& peak);

    bool isOutlier(const PeakData& peak);
};

#endif
