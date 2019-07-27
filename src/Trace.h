#ifndef TRAIN_TRACE_H
#define TRAIN_TRACE_H

#include <vector>
#include <string>

#include "SegTransient.h"
#include "SegQuiet.h"
#include "SegActive.h"
#include "struct.h"

using namespace std;

class PeakStats;
class Control;


class Trace
{
  private:

    vector<double> samples;
    vector<Run> runs;

    SegTransient transient;
    SegQuiet quietFront;
    SegQuiet quietBack;
    SegActive segActive;

    bool transientFlag;
    bool quietFlag;

    void calcRuns();

    bool readBinary(const string& filenameFull);

    void printSamples(const string& title) const;
    string strTransientHeaderCSV();
    string strTransientCSV();

  public:

    Trace();

    ~Trace();

    void read(const string& fname);

    void detect(
      const Control& control,
      const double sampleRate,
      Imperfections& imperf);

    void logPeakStats(
      const vector<double>& posTrue,
      const string& trainTrue,
      const double speedTrue,
      PeakStats& peakStats);

    bool getAlignment(
      vector<PeakTime>& times,
      vector<int>& actualToRef,
      unsigned& numFrontWheels);

    void getTrace(
      vector<PeakTime>& times,
      unsigned& numFrontWheels) const;

    void write(
      const Control& control,
      const string& filenameFull,
      const string& filename) const;

};

#endif
