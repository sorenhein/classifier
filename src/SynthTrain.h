#ifndef TRAIN_SYNTH_H
#define TRAIN_SYNTH_H

#include "Classifier.h"

using namespace std;


class SynthTrain
{
  private:

    struct Disturbance
    {
      float noisePercent; // Standard deviation of interval noise
      int injectUpTo; // Inject random peaks up to this number
      int deleteUpTo; // Delete correct peaks up to this number
      int pruneFrontUpTo; // Delete leading peaks up to this number
      int pruneBackUpTo; // Delete trailing peaks up to this number
    };

    Disturbance disturbance;

    void makeNormalNoise(vector<Peak>& synthPeaks) const;

    void makeRandomInsertions(vector<Peak>& synthPeaks) const;

    void makeRandomDeletions(vector<Peak>& synthPeaks) const;

    void makeRandomFrontDeletions(vector<Peak>& synthPeaks) const;

    void makeRandomBackDeletions(vector<Peak>& synthPeaks) const;

    void scaleTrace(
      vector<Peak>& synthPeaks,
      const int origSpeed,
      const int newSpeed) const;
      

  public:

    SynthTrain();

    ~SynthTrain();

    void readDisturbance(const string& fname);

    void disturb(
      const vector<Peak>& perfectPeaks,
      vector<Peak>& synthPeaks,
      const float origSpeed,
      float& newSpeed) const;
};

#endif
