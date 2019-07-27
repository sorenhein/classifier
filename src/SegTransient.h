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

struct Run
{
  unsigned first;
  unsigned len;
  bool posFlag;
  double cum;
};




class SegTransient
{
  private:

    enum TransientType
    {
      TRANSIENT_NONE = 0,
      TRANSIENT_SMALL = 1,
      TRANSIENT_MEDIUM = 2,
      TRANSIENT_LARGE_POS = 3,
      TRANSIENT_LARGE_NEG = 4,
      TRANSIENT_SIZE = 5
    };


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

    string strHeader() const;

  public:

    SegTransient();

    ~SegTransient();

    void reset();

    bool detect(
      const vector<double>& samples, // TODO: Should use times[]
      const vector<Run>& runs);

    unsigned lastSampleNo() const;

    void writeFile(const string& filename) const;

    string str() const;

    string headerCSV() const;

    string strCSV() const;
};

#endif
