#ifndef TRAIN_SYNTH_H
#define TRAIN_SYNTH_H

#include "Classifier.h"

class Disturb;


using namespace std;


class SynthTrain
{
  private:

    int sampleRate;

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
      const int origSpeed,
      const int newSpeed) const;
      

  public:

    SynthTrain();

    ~SynthTrain();

    void setSampleRate(const int sampleRateIn);

    void disturb(
      const vector<Peak>& perfectPeaks,
      const Disturb& disturb,
      vector<Peak>& synthPeaks,
      const int origSpeed,
      const int minSpeed,
      const int maxSpeed,
      int& newSpeed) const;
};

#endif
