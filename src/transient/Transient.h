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

    // Within a run, a transient might have a quasi-linear piece
    // that builds up from firstBuildupSample (value buildupStart)
    // for a length of buildupLength, leaving only transientLength
    // samples for the actual transient.

    unsigned firstBuildupSample;
    unsigned buildupLength;
    float buildupStart;
    float buildupPeak;
    unsigned transientLength;

    float transientAmpl; // In g
    float timeConstant; // In ms

    vector<float> synth;
    float fitError;


    bool findEarlyPeak(
      const vector<float>& samples,
      const double sampleRate,
      const Run& run);

    bool largeBump(
      const vector<float>& samples,
      const Run& run,
      unsigned& bumpPosition) const;

    bool checkDecline(
      const vector<float>& samples,
      const Run& run);

    void estimateTransientParams(
      const vector<float>& samples,
      const double sampleRate,
      const Run& run);

    void synthesize();

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
