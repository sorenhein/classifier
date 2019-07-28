#ifndef TRAIN_TRANSIENT_H
#define TRAIN_TRANSIENT_H

#include <vector>
#include <string>

// #include "struct.h"

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
  float cum;
};



class Transient
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


    vector<Run> runs;

    TransientStatus status;
    TransientType transientType;

    unsigned firstBuildupSample;
    unsigned buildupLength;
    float buildupStart;

    unsigned transientLength;
    float transientAmpl; // In g
    float timeConstant; // In ms

    vector<float> synth;
    float fitError;


    void calcRuns(const vector<float>& samples);

    bool detectPossibleRun(unsigned& rno);

    bool findEarlyPeak(
      const vector<float>& samples,
      const Run& run);

    bool checkDecline(
      const vector<float>& samples,
      const Run& run);

    void estimateTransientParams(
      const vector<float>& samples,
      const Run& run);

    void synthesize();

    bool largeActualDeviation(
      const vector<float>& samples,
      const Run& run,
      unsigned& devpos) const;

    bool largeSynthDeviation(
      const vector<float>& samples,
      unsigned& devpos) const;

    bool errorIsSmall(const vector<float>& samples);

    string strHeader() const;

  public:

    Transient();

    ~Transient();

    void reset();

    bool detect(const vector<float>& samples);

    unsigned lastSampleNo() const;

    void writeFile(const string& filename) const;

    string str() const;

    string headerCSV() const;

    string strCSV() const;
};

#endif
