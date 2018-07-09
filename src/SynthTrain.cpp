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


void SynthTrain::setSampleRate(const double sampleRateIn)
{
  sampleRate = sampleRateIn; // In Hz
}


bool SynthTrain::makeAccel(
  const vector<PeakPos>& peakPositions, // In mm
  vector<PeakSample>& synthPeaks, // In samples
  const double offset, // In m
  const double speed, // In m/s
  const double accel) const // In m/s^2
{
  PeakSample peak;
  peak.value = 1.;
  synthPeaks.clear();

  const double cfactor = sampleRate / (speed * 1000.);

  const double offsetPos = cfactor * offset;

  if (accel == 0.)
  {
    /* 
       s[m] = v[m/s] * t[s]
       t[s] = s[m] / v[m/s]
       t[samples]/fs[Hz] = (s[mm]/1000) / v[m/s]
       t[samples] = fs[Hz] / (1000 * v[m/s])
       offset is added on at the end.
    */


    const unsigned l = peakPositions.size();
    for (unsigned i = 0; i < l; i++)
    {
      peak.no = 
        static_cast<int>(offsetPos + cfactor * peakPositions[i].pos);
      synthPeaks.push_back(peak);
    }
  }
  else
  {
    /* 
       si is the ith position in peakPositions[m].
       s[m] = s0[m] + v[m/s] * t[s] + (1/2) * a[m/s^2] * t^2 [s^2]
       t[s] = (-v +/- sqrt(v^2 - 2 * a * (si - s0) / a
       t[s] = (v/a) * (+/- sqrt(1 + 2 * a * (si - s0) / v^2) - 1)
       If a > 0, we're always on the positive branch of the parabola,
       so we take the plus sign.
       If a < 0, there's an upper limit on the si's that we can
       ask for.  This is given by the sqrt argument below.
       We take the minus sign in order to be on the growing, left
       part of the parabola.  As the si's are assumed monotonically
       increasing, we don't go past the maximum.

       {si} should be monotonically increasing.
       Corresponds to v + 2 * a * t_last > 0.
       This is not checked, as t_last is calculated and not given.

       We need the sqrt argument to be non-negative, so
       1 + 2 * a * (si - s0) / v^2 >= 0
       v^2 + 2 * a * (si - s0) >= 0
       This is given if a > 0.
       Otherwise a >= -v^2 / (s_last - s0).

       In non-SI units:
       t[samples] = fs[Hz] * (v/a) * 
         (+/- sqrt(1 + 2 * a * (si - s0 [mm]) / (1000 * v^2) - 1)
    */

    const double n0 = sampleRate * speed / accel;
    const double factor = 2. * accel / (1000. * speed * speed);

    const double sfirst = peakPositions.front().pos;
    const double slast = peakPositions.back().pos;
    if (speed * speed + 2. * accel * (slast - sfirst) / 1000. < 0.)
    {
      cout << "Bad acceleration" << endl;
      return false;
    }

    const unsigned l = peakPositions.size();
    for (unsigned i = 0; i < l; i++)
    {
      const double root = 
        sqrt(1. + factor * (peakPositions[i].pos - sfirst));

      if (accel > 0.)
        peak.no = static_cast<int>(offsetPos + n0 * (root-1.));
      else
        peak.no = static_cast<int>(offsetPos + n0 * (-root-1.));
      synthPeaks.push_back(peak);
    }
  }
  return true;
}


void SynthTrain::makeNormalNoise(
  vector<PeakSample>& synthPeaks,
  const int noiseSdev) const
{
  // Noise standard deviation is in absolute milli-seconds.
  if (noiseSdev == 0)
    return;

  random_device rd;
  mt19937 var(rd());
  normal_distribution<> dist(0, 1);
  
  const double factor = noiseSdev * 1000. / sampleRate;

  for (unsigned i = 0; i < synthPeaks.size(); i++)
  {
    const int delta = static_cast<int>(round(factor * dist(var)));
    synthPeaks[i-1].no += delta;
  }
}


void SynthTrain::makeRandomInsertions(
  vector<PeakSample>& synthPeaks,
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
    
    const int tLeft = synthPeaks[hit].no;
    const double vLeft = synthPeaks[hit].value;

    const int tRight = synthPeaks[hit+1].no;
    const double vRight = synthPeaks[hit+1].value;

    PeakSample newPeak;
    newPeak.no = tLeft + 1 + (rand() % (tRight - tLeft - 2));
    newPeak.value = (vLeft + vRight) / 2.;

    synthPeaks.insert(synthPeaks.begin()+hit+1, newPeak);
  }
}


void SynthTrain::makeRandomDeletions(
  vector<PeakSample>& synthPeaks,
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
  vector<PeakSample>& synthPeaks,
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
  vector<PeakSample>& synthPeaks,
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
   const vector<PeakPos>& perfectPositions,
   vector<PeakSample>& synthPeaks,
   const double origSpeed,
   const double newSpeed) const
{
  if (synthPeaks.size() == 0)
    return;

  const int offset = synthPeaks[0].no;
  const double factor = newSpeed / origSpeed;

  for (unsigned i = 0; i < synthPeaks.size(); i++)
  {
    synthPeaks[i].no = offset +
      static_cast<int>((perfectPositions[i].pos - offset) * factor);
  }
}

void printPeaks(const vector<PeakSample>& synthPeaks, const int level);
#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

void SynthTrain::disturb(
  const vector<PeakPos>& perfectPositions, // In mm
  const Disturb& disturb,
  vector<PeakSample>& synthPeaks, // In samples
  const double offset, // In m
  const double speed, // In m/s
  const double accel) const // In m/s^2
{
  if (! SynthTrain::makeAccel(perfectPositions, synthPeaks, 
    offset, speed, accel))
  {
    cout << "makeAccel failed" << endl;
    return;
  }

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

  // SynthTrain::scaleTrace(synthPeaks, origSpeed, newSpeed);

// cout << endl << "scale " << newSpeed << endl;
}

