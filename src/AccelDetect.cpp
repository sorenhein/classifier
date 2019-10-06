#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>

#include "AccelDetect.h"
#include "Except.h"

#include "database/Control.h"

#include "util/io.h"


#define KINK_RATIO 500.f
#define KINK_RATIO_CONSIDER 20.f
#define KINK_RATIO_ONE_DIM 8.f
#define SAMPLE_RATE 2000.f


AccelDetect::AccelDetect()
{
  AccelDetect::reset();
}


AccelDetect::~AccelDetect()
{
}


void AccelDetect::reset()
{
  len = 0;
  offset = 0;

  peaks.clear();
  scale.reset();
}


float AccelDetect::integrate(
  const vector<float>& samples,
  const unsigned i0,
  const unsigned i1) const
{
  float sum = 0.f;
  for (unsigned i = i0; i < i1; i++)
    sum += samples[i];

  sum += 0.5f * (samples[i1] - samples[i0]);

  return sum;
}


void AccelDetect::annotate()
{
  if (peaks.size() < 2)
    THROW(ERR_NO_PEAKS, "Too few peaks: " + to_string(peaks.size()));

  const auto peakFirst = peaks.begin();

  for (auto it = peakFirst; it != peaks.end(); it++)
  {
    const Peak * peakPrev = (it == peakFirst ? nullptr : &*prev(it));

    it->annotate(peakPrev);
  }
}


void AccelDetect::logFirst(const vector<float>& samples)
{
  peaks.extend();

  // Find the initial peak polarity.
  bool maxFlag = false;
  for (unsigned i = offset+1; i < offset+len; i++)
  {
    if (samples[i] > samples[offset])
    {
      maxFlag = false;
      break;
    }
    else if (samples[i] < samples[offset])
    {
      maxFlag = true;
      break;
    }
  }
  peaks.back().logSentinel(samples[offset], maxFlag);
}


void AccelDetect::logLast(const vector<float>& samples)
{
  const auto& peakPrev = peaks.back();
  const float areaFull = 
    AccelDetect::integrate(samples, peakPrev.getIndex(), offset+len-1);
  const float areaCumPrev = peakPrev.getAreaCum();

  peaks.extend();
  peaks.back().log(
    offset+len-1,
    samples[offset+len-1], 
    areaCumPrev + areaFull, 
    ! peakPrev.getMaxFlag());
}


void AccelDetect::log(
  const vector<float>& samples,
  const unsigned offsetIn,
  const unsigned lenIn)
{
  if (samples.size() < 2)
    THROW(ERR_SHORT_ACCEL_TRACE, "Accel trace length: " + to_string(len));

  len = lenIn;
  offset = offsetIn;

  peaks.clear();

  // The first peak is a dummy extremum at the first sample.
  AccelDetect::logFirst(samples);

  for (unsigned i = offset+1; i < offset+len-1; i++)
  {
    bool maxFlag;
    while (i < len-1 && samples[i] == samples[i-1])
      i++;

    if (i == len-1)
      break;

    if (samples[i] > samples[i-1])
    {
      // Use the last of equals as the starting point.
      while (i < len-1 && samples[i] == samples[i+1])
        i++;

      if (i == len-1 || samples[i] <= samples[i+1])
        continue;
      else
        maxFlag = true;
    }
    else
    {
      while (i < len-1 && samples[i] == samples[i+1])
        i++;

      if (i == len-1 || samples[i] >= samples[i+1])
        continue;
      else
        maxFlag = false;
    }

    const auto& peakPrev = peaks.back();
    const float areaFull = AccelDetect::integrate(samples, 
      peakPrev.getIndex(), i);
    const float areaCumPrev = peakPrev.getAreaCum();

    // The peak contains data for the interval preceding it.
    peaks.extend();
    peaks.back().log(i, samples[i], areaCumPrev + areaFull, maxFlag);
  }

  // The last peak is a dummy extremum at the last sample.
  AccelDetect::logLast(samples);

  AccelDetect::annotate();
}


void AccelDetect::reduceSmallPeaks(
  const PeakParam param,
  const float paramLimit,
  const bool preserveKinksFlag)
{
  // We use this method for two reductions:
  // 1. Small areas (first, as a rough reduction).
  // 2. Small ranges (second, as a more precise reduction).
  // In either case, we only allow reductions that don't drastically
  // change the slope of remaining peaks.

  if (peaks.empty())
    THROW(ERR_NO_PEAKS, "Peak list is empty");

  auto peak = next(peaks.begin());

  while (peak != peaks.end())
  {
    const float paramCurrent = peak->getParameter(param);
    if (paramCurrent >= paramLimit)
    {
      peak++;
      continue;
    }

    auto peakCurrent = peak;
    float sumParam = 0.f, lastParam = 0.f;

    do
    {
      peak++;
      if (peak == peaks.end())
        break;

      sumParam = peak->getParameter(* peakCurrent, param);
      lastParam = peak->getParameter(param);
    }
    while (abs(sumParam) < paramLimit || abs(lastParam) < paramLimit);


    if (abs(sumParam) < paramLimit || abs(lastParam) < paramLimit)
    {
      // It's the last set of peaks.  We could keep the largest peak
      // of the same polarity as peakCurrent (instead of peakCurrent).
      // It's a bit random whether or not this would be a "real" peak,
      // and we also don't keep track of this above.  So we just stop.
      if (peakCurrent != peaks.end())
        peaks.erase(peakCurrent, peaks.end());
      break;
    }

    // Depending on gradients, we might delete everything in
    // [peakPrev, peak) or keep support peaks along the way.
    // Of the possibilities from left and from right (separately),
    // note the lowest one.  If they are the same, keep that.

    auto peakFirst = prev(peakCurrent);
    const bool flagExtreme = peakFirst->getMaxFlag();


    if (flagExtreme != peak->getMaxFlag())
    {
      // We have a broad extremum.  If peakFirst is a minimum, then
      // the overall extremum is a minimum.  Find the most extreme
      // peak of the same polarity.
      
      auto peakExtreme = peakFirst;
      for (auto p = peakCurrent; p != peak; p++)
      {
        if (p->getMaxFlag() == flagExtreme)
        {
          if (flagExtreme && p->getValue() > peakExtreme->getValue())
            peakExtreme = p;
          else if (! flagExtreme && p->getValue() < peakExtreme->getValue())
            peakExtreme = p;
        }
      }

      // Determine whether the peak has an extent.
      Peak const * peakL;
      if (peakFirst->similarGradientForward(* peakExtreme))
        peakL = &*peakExtreme;
      else
      {
        // peakExtreme and peakFirst have the same polarity.
        peakL = &*peakFirst;
      }

      list<Peak>::iterator peakR;
      if (peakExtreme->similarGradientBackward(* peak))
        peakR = peakExtreme;
      else
      {
        // peakExtreme and peak have different polarities.
        // Find the last previous one before peak to have the same
        // polarity as peakExtreme.
        peakR = peak;
        while (peakR != peakExtreme)
        {
          if (peakR->getMaxFlag() == flagExtreme)
            break;
          else
            peakR = prev(peakR);
        }
      }
      
      peakExtreme->logExtent(* peakL, * peakR);

      // Do the deletions.
      if (peakFirst != peakExtreme)
        peaks.collapse(peakFirst, peakExtreme);

      peakExtreme++;
      if (peakExtreme != peak)
        peaks.collapse(peakExtreme, peak);

      peak++;
      continue;
    }
    else if (peakFirst->getValue() < 0.f && peak->getValue() < 0.f)
    {
      // Any kink on the negative side of the axis.
      peaks.collapse(peakFirst, peak);
      peak++;
      continue;
    }
    else if (preserveKinksFlag)
    {
      peak++;
      continue;
    }
    else
    {
      // We have a kink.  It's debatable if this should lead to
      // an extent.  Here we just delete everything.
      peaks.collapse(peakFirst, peak);
      peak++;
      continue;
    }
  }
}


void AccelDetect::eliminateKinks()
{
  if (peaks.size() < 4)
    THROW(ERR_NO_PEAKS, "Peak list is short");

  for (auto peak = next(peaks.begin(), 2); peak != prev(peaks.end(), 2); )
  {
    const auto peakPrev = prev(peak);
    const auto peakPrevPrev = prev(peakPrev);
    const auto peakNext = next(peak);

    if (peakPrev->getValue() >= 0.f)
    {
      // No kinks fixed on the positive side of the line.
      peak++;
      continue;
    }

    if (peak->isMinimum() != peakPrevPrev->isMinimum() ||
        peakNext->isMinimum() != peakPrev->isMinimum())
    {
      // We may have destroyed the alternation while reducing.
      peak++;
      continue;
    }

    bool violFlag = false;
    if (peak->getMaxFlag())
    {
      if (peak->getValue() > peakPrevPrev->getValue() ||
         peakPrev->getValue() < peakNext->getValue())
        violFlag = true;
    }
    else
    {
      if (peak->getValue() < peakPrevPrev->getValue() ||
         peakPrev->getValue() > peakNext->getValue())
        violFlag = true;
    }

    if (violFlag)
    {
      // A kink should satisfy basic monotonicity.
      peak++;
      continue;
    }

    const float areaPrev = peak->getArea(* peakPrev);
    if (areaPrev == 0.f)
      THROW(ERR_ALGO_PEAK_COLLAPSE, "Zero area");

    const float ratioPrev = peakPrev->getArea(* peakPrevPrev) / areaPrev;
    const float ratioNext = peakNext->getArea(* peak) / areaPrev;

    if (ratioPrev <= 1.f || ratioNext <= 1.f)
    {
      // Should look like a kink.
      peak++;
      continue;
    }

    const unsigned lenWhole = peakNext->getIndex() - peakPrevPrev->getIndex();
    const unsigned lenMid = peak->getIndex() - peakPrev->getIndex();

    const float vWhole = abs(peakNext->getValue() - peakPrevPrev->getValue());
    const float vMid = abs(peak->getValue() - peakPrev->getValue());

    if (KINK_RATIO_ONE_DIM * lenMid < lenWhole &&
        KINK_RATIO_ONE_DIM * vMid < vWhole)
    {
      peak = peaks.collapse(peakPrev, peakNext);
    }
    else
      peak++;
  }
}


void AccelDetect::estimateScale()
{
  scale.reset();
  unsigned no = 0;
  for (auto peak = next(peaks.begin()); peak != peaks.end(); peak++)
  {
    if (peak->isCandidate())
    {
      scale += * peak;
      no++;
    }
  }

  if (no == 0)
    THROW(ERR_NO_PEAKS, "No negative minima");

  scale/= no;
}


void AccelDetect::completePeaks()
{
  for (auto pit = peaks.begin(); pit != prev(peaks.end()); pit++)
  {
    auto npit = next(pit);
    pit->logNextPeak(&*npit);
  }
}


void AccelDetect::extract()
{
  if (peaks.empty())
    return;

  const bool debug = true;
  const bool debugDetails = true;

  if (debugDetails)
    cout << peaks.str("Original accel peaks", offset);

  AccelDetect::reduceSmallPeaks(PEAK_PARAM_AREA, 0.1f, false);

  if (debugDetails)
    cout << peaks.str("Non-tiny accel peaks", offset);

  AccelDetect::eliminateKinks();

  if (debugDetails)
    cout << peaks.str("Non-kinky accel list peaks (first)", offset);

  AccelDetect::estimateScale();
  if (debug)
    AccelDetect::printPeak(scale, "Accel scale");

  AccelDetect::reduceSmallPeaks(PEAK_PARAM_RANGE, 
    scale.getRange() / 10.f, true);

  if (debugDetails)
    cout << peaks.str("Range-reduced accel peaks", offset);

  AccelDetect::eliminateKinks();

  if (debugDetails)
    cout << peaks.str("Non-kinky list accel peaks (second)", offset);

  AccelDetect::completePeaks();
}


void AccelDetect::printPeak(
  const Peak& peak,
  const string& text) const
{
  cout << text << "\n";
  cout << peak.strHeader();
  cout << peak.str(0) << endl;
}


void AccelDetect::printPeaksCSV(const vector<double>& timesTrue) const
{
  cout << "true\n";
  unsigned i = 0;
  for (auto tt: timesTrue)
    cout << i++ << ";" << fixed << setprecision(6) << tt << "\n";
  cout << "\n";

  cout << peaks.strTimesCSV(&Peak::isSelected, "seen");
}


void AccelDetect::writePeak(
  const Control& control,
  const unsigned first,
  const string& filename) const
{
  vector<unsigned> times;
  vector<float> values;
  peaks.getSamples(&Peak::isSelected, first, times, values);

  writeBinaryUnsigned(
    control.peakDir() + "/" + control.timesName() + "/" + filename, times);
  writeBinaryFloat(
    control.peakDir() + "/" + control.valuesName() + "/" + filename, values);
}


Peaks& AccelDetect::getPeaks()
{
  return peaks;
}
