#ifndef TRAIN_SEGTRANSIENT_H
#define TRAIN_SEGTRANSIENT_H

#include <vector>
#include <string>

#include "struct.h"

using namespace std;

enum TransientStatus
{
  TSTATUS_TOO_FEW_RUNS = 0,
  TSTATUS_NO_CANDIDATE_RUN = 1,
  TSTATUS_NO_FULL_DECLINE = 2,
  TSTATUS_NO_MID_DECLINE = 3,
  TSTATUS_BACK_ACTUAL_CORR = 4,
  TSTATUS_FRONT_ACTUAL_CORR = 5,
  TSTATUS_BACK_SYNTH_CORR = 6,
  TSTATUS_BAD_FIT = 7,
  TSTATUS_SIZE = 8
};


class SegTransient
{
  private:

    TransientStatus status;
    TransientType transientType;

    unsigned firstBuildupSample;
    unsigned buildupLength;
    double buildupStart;

    unsigned transientLength;
    double transientAmpl; // In g
    double timeConstant; // In ms

    vector<float> synth;
    double fitError;

    bool detectPossibleRun(
      const vector<Run>& runs,
      unsigned& rno);

    bool findEarlyPeak(
      const vector<double>& samples,
      const Run& run);

    bool checkDecline(
      const vector<double>& samples,
      const Run& run);

    void estimateTransientParams(
      const vector<double>& samples,
      const Run& run);

    void synthesize();

    bool largeActualDeviation(
      const vector<double>& samples,
      const Run& run,
      unsigned& devpos) const;

    bool largeSynthDeviation(
      const vector<double>& samples,
      unsigned& devpos) const;

    bool errorIsSmall(const vector<double>& samples);

  public:

    SegTransient();

    ~SegTransient();

    void reset();

    bool detect(
      const vector<double>& samples, // TODO: Should use times[]
      const vector<Run>& runs);

    unsigned lastSampleNo() const;

    void writeBinary(const string& origname) const;

    string headerCSV() const;

    string strCSV() const;
};

#endif
