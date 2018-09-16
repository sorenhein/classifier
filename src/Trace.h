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


class Trace
{
  private:

    vector<double> samples;
    vector<Run> runs;

    SegTransient transient;
    SegQuiet quietFront;
    SegQuiet quietBack;
    SegActive segActive;

    string filename;
    bool transientFlag;
    bool quietFlag;

    void calcRuns();

    bool readText();
    bool readBinary();

    void printSamples(const string& title) const;
    string strTransientHeaderCSV();
    string strTransientCSV();

  public:

    Trace();

    ~Trace();

    void read(
      const string& fname,
      const bool binaryFlag);

    void detect(const Control& control);

    void logPeakStats(
      const vector<PeakPos>& posTrue,
      const string& trainTrue,
      const double speedTrue,
      PeakStats& peakStats);

    void getTrace(vector<PeakTime>& times) const;

    void write(const Control& control) const;

};

#endif
