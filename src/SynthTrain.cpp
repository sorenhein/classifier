/* 
   Makes a synthethic trace of peaks corresponding to a given train
   type.  Only the peak times and not the peak amplitudes are modeled
   realistically.  The peak times are subject to several disturbances.
   controlled by five parameters:
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <random>

#include "SynthTrain.h"
#include "Disturb.h"


SynthTrain::SynthTrain()
{
}


SynthTrain::~SynthTrain()
{
}


bool SynthTrain::makeAccel(
  const vector<PeakPos>& peakPositions, // In m
  vector<PeakTime>& synthPeaks, // In s
  const double offset, // In m
  const double speed, // In m/s
  const double accel) const // In m/s^2
{
  PeakTime peak;
  peak.value = 1.;
  synthPeaks.clear();

  const double offsetPos = offset / speed;

  if (abs(accel) < 1.e-3)
  {
    /* 
       s[m] = v[m/s] * t[s]
       t[s] = s[m] / v[m/s]
       t[samples]/fs[Hz] = s[m] / v[m/s]
       t[samples] = fs[Hz] / v[m/s]
       offset is added on at the end.
    */


    const unsigned l = peakPositions.size();
    for (unsigned i = 0; i < l; i++)
    {
      peak.time = offsetPos + peakPositions[i].pos / speed;
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
       We take the plus sign in order to be on the growing
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
    */

    const double t0 = speed / accel;
    const double factor = 2. * accel / (speed * speed);

    const double sfirst = peakPositions.front().pos;
    const double slast = peakPositions.back().pos;
    if (speed * speed + 2. * accel * (slast - sfirst) < 0.)
      return false;

    const unsigned l = peakPositions.size();
    for (unsigned i = 0; i < l; i++)
    {
      const double root = 
        sqrt(1. + factor * (peakPositions[i].pos - sfirst));

      peak.time = offsetPos + t0 * (root-1.);
      synthPeaks.push_back(peak);
    }
  }
  return true;
}


void SynthTrain::makeNormalNoise(
  vector<PeakTime>& synthPeaks,
  const int noiseSdev) const
{
  // Noise standard deviation is in absolute milli-seconds.
  if (noiseSdev == 0)
    return;

  random_device rd;
  mt19937 var(rd());
  normal_distribution<> dist(0, 1);
  
  const double factor = noiseSdev / 1000.;

  const unsigned l = synthPeaks.size();
  for (unsigned i = 0; i < l; i++)
  {
    double gapBefore;
    if (i == 0)
      gapBefore = -1.; // Something big and negative
    else
      gapBefore = synthPeaks[i-1].time - synthPeaks[i].time;

    double gapAfter;
    if (i == l-1)
      gapAfter = 1.; // Something big and positive
    else
      gapAfter = synthPeaks[i+1].time - synthPeaks[i].time;

    double noise;
    do
    {
      noise = factor * dist(var);
    }
    while (noise <= gapBefore || noise >= gapAfter);

    synthPeaks[i-1].time += noise;
  }
}


void SynthTrain::makeRandomInsertions(
  vector<PeakTime>& synthPeaks,
  const int lo,
  const int hi) const
{
  if (hi == 0)
    return;

  const unsigned n = lo + static_cast<unsigned>(rand() % (hi+1-lo));

  random_device rd;
  mt19937 var(rd());
  uniform_real_distribution<> dist(0, 1);

  for (unsigned i = 0; i < n; i++)
  {
    const unsigned hit = 
      static_cast<unsigned>(rand() % (synthPeaks.size()-1));
    
    const double tLeft = synthPeaks[hit].time;
    const double vLeft = synthPeaks[hit].value;

    const double tRight = synthPeaks[hit+1].time;
    const double vRight = synthPeaks[hit+1].value;

    PeakTime newPeak;
    newPeak.time = tLeft + (tRight - tLeft) * dist(var);
    newPeak.value = (vLeft + vRight) / 2.;

    synthPeaks.insert(synthPeaks.begin()+hit+1, newPeak);
  }
}


void SynthTrain::makeRandomDeletions(
  vector<PeakTime>& synthPeaks,
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
  vector<PeakTime>& synthPeaks,
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
  vector<PeakTime>& synthPeaks,
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
   vector<PeakTime>& synthPeaks,
   const double origSpeed,
   const double newSpeed) const
{
  if (synthPeaks.size() == 0)
    return;

  const double offset = synthPeaks[0].time;
  const double factor = newSpeed / origSpeed;

  for (unsigned i = 0; i < synthPeaks.size(); i++)
  {
    synthPeaks[i].time = offset +
      static_cast<int>((perfectPositions[i].pos - offset) * factor);
  }
}


bool SynthTrain::disturb(
  const vector<PeakPos>& perfectPositions, // In m
  const Disturb& disturb,
  vector<PeakTime>& synthPeaks, // In samples
  const double offset, // In m
  const double speed, // In m/s
  const double accel) const // In m/s^2
{
  if (! SynthTrain::makeAccel(perfectPositions, synthPeaks, 
    offset, speed, accel))
  {
    return false;
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
  return true;
}


