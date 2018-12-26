#ifndef TRAIN_PEAKDETECT_H
#define TRAIN_PEAKDETECT_H

#include <vector>
#include <list>
#include <string>
#include <sstream>

#include "Peak.h"
#include "PeakPool.h"
#include "PeakStructure.h"
#include "CarModels.h"
#include "struct.h"

using namespace std;

class PeakStats;


class PeakDetect
{
  private:

    unsigned len;
    unsigned offset;

    list<Peak> peaks;
    PeakPool peakPool;

    list<Peak *> candidates;
    CarModels models;
    PeakStructure pstruct;


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

    void reduceSmallPeaks(
      const PeakParam param,
      const float paramLimit,
      const bool preserveKinksFlag,
      const Control& control);
    void reduceSmallPeaksNew(
      const PeakParam param,
      const float paramLimit,
      const bool preserveKinksFlag,
      const Control& control);

    void eliminateKinks();
    void eliminateKinksNew();

    void estimateScale(Peak& scale);

    float getFirstPeakTime() const;

    void printPeak(
      const Peak& peak,
      const string& text) const;


  public:

    PeakDetect();

    ~PeakDetect();

    void reset();

    void log(
      const vector<float>& samples,
      const unsigned offsetSamples);

    void reduce(
      const Control& control,
      Imperfections& imperf);

    void logPeakStats(
      const vector<PeakPos>& posTrue,
      const string& trainTrue,
      const double speedTrue,
      PeakStats& peakStats);

    void makeSynthPeaks(vector<float>& synthPeaks) const;
    void getPeakTimes(vector<PeakTime>& times) const;

    void printPeaksCSV(const vector<PeakTime>& timesTrue) const;

    void printAllPeaks(const string& text = "") const;

    // TMP
    string deleteStr(Peak const * p1, Peak const * p2) const;

};

#endif
