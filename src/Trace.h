#ifndef TRAIN_TRACE_H
#define TRAIN_TRACE_H

#include <vector>
#include <string>

#include "SegTransient.h"
#include "SegQuiet.h"
#include "struct.h"

using namespace std;


class Trace
{
  private:

    vector<double> samples;
    vector<Run> runs;
    vector<PeakTime> times;

    SegTransient transient;
    SegQuiet quietFront;
    SegQuiet quietBack;
    SegQuiet quietIntra;

    string filename;
    unsigned firstActiveSample;
    unsigned firstActiveRun;
    bool transientFlag;
    bool quietFlag;

    bool readText();

    bool readBinary();

    void calcRuns();

    bool runsToBumps(vector<Run>& bumps) const;

    void tallyBumps(
      const vector<Run>& bumps,
      unsigned& longRun,
      double& tallRun) const;

    void combineRuns(
      vector<Run>& runvec,
      const double threshold) const;

    double calcThreshold(
      const vector<Run>& runvec,
      const unsigned num) const;

    bool thresholdPeaks();

    void printSamples(const string& title) const;

    void printRunsAsVector(
      const string& title,
      const vector<Run>& runvec) const;

    void printFirstRuns(
      const string& title,
      const vector<Run>& runvec,
      const unsigned num) const;

  public:

    Trace();

    ~Trace();

    bool read(const string& fname);

    void getTrace(vector<PeakTime>& timesOut) const;

    void writeTransient() const;
    void writeQuietBack() const;
    void writeQuietFront() const;

    string strTransientHeaderCSV();
    string strTransientCSV();

    void printStats() const;

};

#endif
