#ifndef TRAIN_SEGACTIVE_H
#define TRAIN_SEGACTIVE_H

#include <vector>
#include <string>

#include "Median/median.h"
#include "PeakDetect.h"
#include "struct.h"

using namespace std;


class SegActive
{
  private:

    vector<float> synthSpeed;
    vector<float> synthPos;
    vector<float> synthPeaks;

    Interval writeInterval;

    PeakDetect peakDetect;

    void integrate(
      const vector<double>& samples,
      const Interval& active);

    void compensateSpeed();

    void integrateFloat();

    void highpass();

  public:

    SegActive();

    ~SegActive();

    void reset();

    bool detect(
      const vector<double>& samples, // TODO: Should use times[]
      const Interval& active);

    void getPeakTimes(vector<PeakTime>& times) const; 

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
