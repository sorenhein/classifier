#ifndef TRAIN_TRACE_H
#define TRAIN_TRACE_H

#include <vector>
#include <string>

#include "transient/Transient.h"
#include "transient/Quiet.h"

#include "SegActive.h"
#include "struct.h"

using namespace std;

class PeakStats;
class Control;


class Trace
{
  private:

    vector<double> samples;

    Transient transient;
    Quiet quietFront;
    Quiet quietBack;

    SegActive segActive;

    string strTransientCSV();

  public:

    Trace();

    ~Trace();

    void detect(
      const Control& control,
      const double sampleRate,
      const vector<float>& fsamples,
      const unsigned thid,
      Imperfections& imperf);

    void logPeakStats(
      const vector<double>& posTrue,
      const string& trainTrue,
      const double speedTrue,
      PeakStats& peakStats);

    bool getAlignment(
      vector<double>& times,
      vector<int>& actualToRef,
      unsigned& numFrontWheels);

    void getTrace(
      vector<double>& times,
      unsigned& numFrontWheels) const;

    void write(
      const Control& control,
      const string& filename,
      const unsigned thid) const;

};

#endif
