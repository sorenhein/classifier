#ifndef TRAIN_SEGACTIVE_H
#define TRAIN_SEGACTIVE_H

#include <vector>
#include <string>

// #include "Median/median.h"
#include "PeakDetect.h"
#include "struct.h"

using namespace std;

class PeakStats;


class SegActive
{
  private:

    vector<float> accelFloat;
    vector<float> synthSpeed;
    vector<float> synthPos;
    vector<float> synthPeaks;

    Interval writeInterval;

    PeakDetect peakDetect;

    void doubleToFloat(
      const vector<double>& samples,
      vector<float>& samplesFloat);

    void integrateFloat(
      const vector<float>& integrand,
      const bool a2vflag, // true if accel->speed, false if speed->pos
      const unsigned startIndex,
      const unsigned len,
      vector<float>& result) const;

    void filterFloat(
      const vector<float>& num,
      const vector<float>& denom,
      vector<float>& integrand);

    void highpass(
      const vector<double>& num,
      const vector<double>& denom,
      vector<float>& integrand);

  public:

    SegActive();

    ~SegActive();

    void reset();

    bool detect(
      const vector<double>& samples, // TODO: Should use times[]
      const Interval& active,
      const Control& control,
      Imperfections& imperf);

    void logPeakStats(
      const vector<PeakPos>& posTrue,
      const string& trainTrue,
      const double speedTrue,
      PeakStats& peakStats);

    void getPeakTimes(
      vector<PeakTime>& times,
      unsigned& numFrontWheels) const; 

    void writeSpeed(
      const string& origname,
      const string& dirname) const;

    void writePos(
      const string& origname,
      const string& dirname) const;

    void writePeak(
      const string& origname,
      const string& dirname) const;
};

#endif
