#ifndef TRAIN_SYNTH_H
#define TRAIN_SYNTH_H

#include "Classifier.h"

class Disturb;


using namespace std;


class SynthTrain
{
  private:

    int sampleRate;

    bool makeAccel(
      vector<Peak>& synthPeaks,
      const float offset,
      const float speed,
      const float accel) const;

    void makeNormalNoise(
      vector<Peak>& synthPeaks,
      const int noiseSdev) const;

    void makeRandomInsertions(
      vector<Peak>& synthPeaks,
      const int lo,
      const int hi) const;

    void makeRandomDeletions(
      vector<Peak>& synthPeaks,
      const int lo,
      const int hi) const;

    void makeRandomFrontDeletions(
      vector<Peak>& synthPeaks,
      const int lo,
      const int hi) const;

    void makeRandomBackDeletions(
      vector<Peak>& synthPeaks,
      const int lo,
      const int hi) const;

    void scaleTrace(
      vector<Peak>& synthPeaks,
      const float origSpeed,
      const float newSpeed) const;
      

  public:

    SynthTrain();

    ~SynthTrain();

    void setSampleRate(const int sampleRateIn);

    void disturb(
      const vector<Peak>& perfectPeaks,
      const Disturb& disturb,
      vector<Peak>& synthPeaks,
      const float origSpeed,
      const float minSpeed,
      const float maxSpeed,
      float& newSpeed) const;
};

#endif
