#ifndef TRAIN_TRANSIENT_H
#define TRAIN_TRANSIENT_H

#include <vector>
#include <string>

#include "trans.h"

using namespace std;


class Transient
{
  private:

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

    string strHeaderCSV() const;

  public:

    Transient();

    ~Transient();

    void reset();

    bool detect(
      const vector<float>& samples,
      const double sampleRate);

    unsigned lastSampleNo() const;

    void writeFile(const string& filename) const;

    string str() const;

    string strCSV() const;
};

#endif
