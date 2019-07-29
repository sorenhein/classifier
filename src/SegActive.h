#ifndef TRAIN_SEGACTIVE_H
#define TRAIN_SEGACTIVE_H

#include <vector>
#include <string>

#include "PeakDetect.h"

#include "transient/trans.h"

#include "struct.h"

using namespace std;

class PeakStats;
class Control;
struct FilterDouble;
struct FilterFloat;


class SegActive
{
  private:

    vector<float> accelFloat;
    vector<float> synthSpeed;
    vector<float> synthPos;
    // vector<float> synthPeaks;

    Interval writeInterval;

    // PeakDetect peakDetect;

    void doubleToFloat(
      const vector<double>& samples,
      vector<float>& samplesFloat);

    void integrateFloat(
      const vector<float>& integrand,
      const float sampleRate,
      const bool a2vflag, // true if accel->speed, false if speed->pos
      const unsigned startIndex,
      const unsigned len,
      vector<float>& result) const;

    void filterFloat(
      const FilterFloat& filter,
      vector<float>& integrand);

    void highpass(
      const FilterDouble& filter,
      vector<float>& integrand);

  public:

    SegActive();

    ~SegActive();

    void reset();

    bool detect(
      const vector<double>& samples, // TODO: Should use times[]
      const double sampleRate,
      const Interval& active,
      // const Control& control,
      const unsigned thid);
      // Imperfections& imperf);

    const vector<float>& getDeflection() const;

    /*
    void logPeakStats(
      const vector<double>& posTrue,
      const string& trainTrue,
      const double speedTrue,
      PeakStats& peakStats);

    bool getAlignment(
      vector<double>& times,
      vector<int>& actualToRef,
      unsigned& numFrontWheels);

    void getPeakTimes(
      vector<double>& times,
      unsigned& numFrontWheels) const; 
      */

    void writeSpeed(const string& filename) const;

    void writePos(const string& filename) const;

    void writePeak(const string& filename) const;
};

#endif
