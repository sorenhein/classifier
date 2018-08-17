#ifndef TRAIN_SEGTRANSIENT_H
#define TRAIN_SEGTRANSIENT_H

#include <vector>
#include <string>

#include "struct.h"

using namespace std;


class SegTransient
{
  private:

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
      unsigned& rno) const;

    bool findEarlyPeak(
      const vector<double>& samples,
      const Run& run);

    bool checkDecline(
      const vector<double>& samples,
      const Run& run) const;

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

    bool detect(
      const vector<double>& samples, // TODO: Should use times[]
      const vector<Run>& runs);

    void writeBinary(const string& origname) const;

    string strCSV() const;

};

#endif
