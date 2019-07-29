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

    Interval writeInterval;


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
      const vector<double>& samples,
      const double sampleRate,
      const Interval& active,
      const unsigned thid);

    const vector<float>& getDeflection() const;

    void writeSpeed(const string& filename) const;

    void writePos(const string& filename) const;
};

#endif
