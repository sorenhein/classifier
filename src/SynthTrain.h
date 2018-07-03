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


  public:

    SynthTrain();

    ~SynthTrain();

    void readDisturbance(const string& fname);

    void disturb(
      const vector<Peak>& perfectPeaks,
      vector<Peak>& synthPeaks,
      const int origSpeed,
      int& newSpeed) const;
};

#endif
