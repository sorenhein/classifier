#ifndef TRAIN_ACCELDETECT_H
#define TRAIN_ACCELDETECT_H

#include <vector>
#include <list>
#include <string>
#include <sstream>

#include "Peaks.h"

using namespace std;

class Control;


class AccelDetect
{
  private:

    unsigned len;
    unsigned offset;

    Peaks peaks;

    Peak scale;


    float integrate(
      const vector<float>& samples,
      const unsigned i0,
      const unsigned i1) const;

    void annotate();

    void logFirst(const vector<float>& samples);
    void logLast(const vector<float>& samples);

    const list<Peak>::iterator collapsePeaks(
      const list<Peak>::iterator peak1,
      const list<Peak>::iterator peak2);

    void reduceSmallPeaks(
      const PeakParam param,
      const float paramLimit,
      const bool preserveKinksFlag);

    void eliminateKinks();

    void estimateScale();

    void completePeaks();

    void printPeak(
      const Peak& peak,
      const string& text) const;


  public:

    AccelDetect();

    ~AccelDetect();

    void reset();

    void log(
      const vector<float>& samples,
      const unsigned offsetIn,
      const unsigned lenIn);

    void extract();

    void printPeaksCSV(const vector<double>& timesTrue) const;

    void writePeak(
      const Control& control,
      const unsigned first,
      const string& filename) const;

    Peaks& getPeaks();

};

#endif
