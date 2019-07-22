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
class Control;


class PeakDetect
{
  private:

    unsigned len;
    unsigned offset;

    PeakPool peaks;

    CarModels models;
    list<CarDetect> cars;
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

    void eliminateKinks();

    void estimateScale(Peak& scale) const;

    void completePeaks();

    void pushPeak(
      Peak const * pptr,
      const float tOffset,
      int& pnoNext,
      vector<PeakTime>& times,
      vector<int>& actualToRef) const;

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
      const vector<double>& posTrue,
      const string& trainTrue,
      const double speedTrue,
      PeakStats& peakStats);

    void makeSynthPeaks(vector<float>& synthPeaks) const;

    bool getAlignment(
      vector<PeakTime>& times,
      vector<int>& actualToRef,
      unsigned& numFrontWheels);

    void getPeakTimes(
      vector<PeakTime>& times,
      unsigned& numFrontWheels) const;

    void printPeaksCSV(const vector<PeakTime>& timesTrue) const;

    // TMP
    string deleteStr(Peak const * p1, Peak const * p2) const;

};

#endif
