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
    vector<PeakTime> times;

    double threshold;

    string filename;
    unsigned firstActiveSample;
    unsigned lastTransientSample;
    double midLevel;
    double timeConstant;
    double sdev;
    double average;

    bool readText();

    bool readBinary();

    unsigned findCrossing(const double level) const;

    bool processTransient();

    bool skipTransient();

    bool calcAverage();

    void calcRuns(vector<Run>& runs) const;

    bool runsToBumps(
      const vector<Run>& runs,
      vector<Run>& bumps) const;

    void tallyBumps(
      const vector<Run>& bumps,
      unsigned& longRun,
      double& tallRun) const;

    bool thresholdPeaks();


  public:

    Trace();

    ~Trace();

    bool read(const string& fname);

    void getTrace(vector<PeakTime>& timesOut) const;

    void printStats() const;

};

#endif
