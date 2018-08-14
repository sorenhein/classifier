#ifndef TRAIN_TRACE_H
#define TRAIN_TRACE_H

#include <vector>
#include <string>

#include "struct.h"

using namespace std;


class Trace
{
  private:

    struct Run
    {
      unsigned first;
      unsigned len;
      bool posFlag;
      double cum;
    };

    vector<double> samples;
    vector<Run> runs;
    vector<PeakTime> times;

    string filename;
    unsigned firstActiveSample;
    unsigned firstActiveRun;
    unsigned lastTransientSample;
    double timeConstant; // In ms
    double transientAmpl; // In g

    bool readText();

    bool readBinary();

    bool processTransient();

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

    void printStats() const;

};

#endif
