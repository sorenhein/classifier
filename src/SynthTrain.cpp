/* 
   Makes a synthethic trace of peaks corresponding to a given train
   type.  Only the peak times and not the peak amplitudes are modeled
   realistically.  The peak times are subject to several disturbances
   controlled by five parameters:

   - noicePercent set the standard deviation of the (normally
     distributed) noise on the interval length between two ideal
     peaks.

   - injectUpTo sets the maximum number (uniformly distributed between
     0 and this number) of peaks that are randomly inserted anywhere
     in the trace.  "Anywhere includes up to +/- 2% beyond the
     actual length of the train.
   
   - deleteUpTo is analogous and sets the maximum number of peaks
     that are randomly deleted among the actual peaks.

   - pruneFrontUpTo sets the maximum number of actual leading 
     (consecutive) peaks that are randomly deleted at the front of
     a train.

   - pruneBackUpTo is similar for the back.
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <random>
// #include <cstdlib>
// #include <ctime>

#include "SynthTrain.h"
#include "Disturb.h"
// #include "read.h"


SynthTrain::SynthTrain()
{
}


SynthTrain::~SynthTrain()
{
}


void SynthTrain::setSampleRate(const int sampleRateIn)
{
  sampleRate = sampleRateIn;
}


void SynthTrain::makeNormalNoise(
  vector<Peak>& synthPeaks,
  const int noiseSdev) const
{
  // Noise standard deviation is in absolute milli-seconds.
  if (noiseSdev == 0)
    return;

  random_device rd;
  mt19937 var(rd());
  normal_distribution<> dist(0, 1);
  
  const float factor = noiseSdev * 1000.f / sampleRate;

  for (unsigned i = 0; i < synthPeaks.size(); i++)
  {
    const int delta = static_cast<int>(round(factor * dist(var)));
    synthPeaks[i-1].sampleNo += delta;
  }
}


void SynthTrain::makeRandomInsertions(
  vector<Peak>& synthPeaks,
  const int lo,
  const int hi) const
{
  if (hi == 0)
    return;

  const unsigned n = lo + static_cast<unsigned>(rand() % (hi+1-lo));

  for (unsigned i = 0; i < n; i++)
  {
    const unsigned hit = 
      static_cast<unsigned>(rand() % (synthPeaks.size()-1));
    
    const int tLeft = synthPeaks[hit].sampleNo;
    const float vLeft = synthPeaks[hit].value;

    const int tRight = synthPeaks[hit+1].sampleNo;
    const float vRight = synthPeaks[hit+1].value;

    Peak newPeak;
    newPeak.sampleNo = tLeft + 1 + (rand() % (tRight - tLeft - 2));
    newPeak.value = (vLeft + vRight) / 2.f;

    synthPeaks.insert(synthPeaks.begin()+hit+1, newPeak);
  }
}


void SynthTrain::makeRandomDeletions(
  vector<Peak>& synthPeaks,
  const int lo,
  const int hi) const
{
  if (hi == 0)
    return;

  const unsigned n = static_cast<unsigned>(rand() % (hi+1-lo));

  for (unsigned i = 0; i < n; i++)
  {
    const unsigned hit = 
      static_cast<unsigned>(rand() % synthPeaks.size());
    
    synthPeaks.erase(synthPeaks.begin() + hit);
  }
}


void SynthTrain::makeRandomFrontDeletions(
  vector<Peak>& synthPeaks,
  const int lo,
  const int hi) const
{
  if (hi == 0)
    return;

  const unsigned n = static_cast<unsigned>(rand() % (hi+1-lo));
  
  if (n == 0)
    return;

  synthPeaks.erase(synthPeaks.begin(), synthPeaks.begin() + n);
}


void SynthTrain::makeRandomBackDeletions(
  vector<Peak>& synthPeaks,
  const int lo,
  const int hi) const
{
  if (hi == 0)
    return;

  const unsigned n = static_cast<unsigned>(rand() % (hi+1-lo));
  
  if (n == 0)
    return;

  synthPeaks.erase(synthPeaks.end() - n, synthPeaks.end());
}


void SynthTrain::scaleTrace(
   vector<Peak>& synthPeaks,
   const int origSpeed,
   const int newSpeed) const
{
  if (synthPeaks.size() == 0)
    return;

  const int offset = synthPeaks[0].sampleNo;
  const float factor = static_cast<float>(newSpeed) /
    static_cast<float>(origSpeed);

  for (unsigned i = 0; i < synthPeaks.size(); i++)
  {
    synthPeaks[i].sampleNo = offset +
      static_cast<int>((synthPeaks[i].sampleNo - offset) * factor);
  }
}

void printPeaks(const vector<Peak>& synthPeaks, const int level);
#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

void SynthTrain::disturb(
  const vector<Peak>& perfectPeaks,
  const Disturb& disturb,
  vector<Peak>& synthPeaks,
  const int origSpeed,
  const int minSpeed,
  const int maxSpeed,
  int& newSpeed) const
{
  synthPeaks = perfectPeaks;

  SynthTrain::makeNormalNoise(synthPeaks, disturb.getNoiseSdev());

  // int lo, hi;
  // disturb.getInjectRange(lo, hi);
  // SynthTrain::makeRandomInsertions(synthPeaks, lo, hi);

  // disturb.getDeleteRange(lo, hi);
  // SynthTrain::makeRandomDeletions(synthPeaks);

  // disturb.getFrontRange(lo, hi);
  // SynthTrain::makeRandomFrontDeletions(synthPeaks);

  // disturb.getBackRange(lo, hi);
  // SynthTrain::makeRandomBackDeletions(synthPeaks);

  newSpeed = minSpeed + (rand() % (maxSpeed-minSpeed));
  SynthTrain::scaleTrace(synthPeaks, origSpeed, newSpeed);

// cout << endl << "scale " << newSpeed << endl;
}

