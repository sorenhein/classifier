#ifndef TRAIN_SYNTH_H
#define TRAIN_SYNTH_H

#include "Classifier.h"
#include "const.h"

class Disturb;


using namespace std;


class SynthTrain
{
  private:

    double sampleRate;

    bool makeAccel(
      const vector<PeakPos>& perfectPositions, // In mm
      vector<PeakSample>& synthPeaks, // In samples
      const double offset, // In m
      const double speed, // In m/s
      const double accel) const; // In m/s^2

    void makeNormalNoise(
      vector<PeakSample>& synthPeaks,
      const int noiseSdev) const;

    void makeRandomInsertions(
      vector<PeakSample>& synthPeaks,
      const int lo,
      const int hi) const;

    void makeRandomDeletions(
      vector<PeakSample>& synthPeaks,
      const int lo,
      const int hi) const;

    void makeRandomFrontDeletions(
      vector<PeakSample>& synthPeaks,
      const int lo,
      const int hi) const;

    void makeRandomBackDeletions(
      vector<PeakSample>& synthPeaks,
      const int lo,
      const int hi) const;

    void scaleTrace(
      const vector<PeakPos>& perfectPositions, // In mm
      vector<PeakSample>& synthPeaks, // In samples
      const double origSpeed,
      const double newSpeed) const;
      

  public:

    SynthTrain();

    ~SynthTrain();

    void setSampleRate(const double sampleRateIn);

    void disturb(
      const vector<PeakPos>& perfectPositions, // In mm
      const Disturb& disturb,
      vector<PeakSample>& synthPeaks, // In samples
      const double offset, // In m
      const double speed, // In m/s
      const double accel) const; // In m/s^2
};

#endif
