#ifndef TRAIN_TRACE_H
#define TRAIN_TRACE_H

#include <vector>
#include <string>

#include "SegTransient.h"
#include "SegQuiet.h"
#include "SegActive.h"
#include "struct.h"

using namespace std;


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

  public:

    Trace();

    ~Trace();

    bool read(
      const string& fname,
      const bool binaryFlag);

    void getTrace(vector<PeakTime>& times) const;

    string strTransientHeaderCSV();
    string strTransientCSV();

    void write(const Control& control) const;

};

#endif
