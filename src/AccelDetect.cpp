#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>

#include "AccelDetect.h"
#include "database/Control.h"
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
  for (unsigned i = 1; i < len; i++)
  {
    if (samples[i] > samples[0])
    {
      maxFlag = false;
      break;
    }
    else if (samples[i] < samples[0])
    {
      maxFlag = true;
      break;
    }
  }
  peaks.back().logSentinel(samples[0], maxFlag);
}


void AccelDetect::logLast(const vector<float>& samples)
{
  const auto& peakPrev = peaks.back();
  const float areaFull = 
    AccelDetect::integrate(samples, peakPrev.getIndex(), len-1);
  const float areaCumPrev = peakPrev.getAreaCum();

  peaks.extend();
  peaks.back().log(len-1,
    samples[len-1], 
    areaCumPrev + areaFull, 
    ! peakPrev.getMaxFlag());
}


void AccelDetect::log(
  const vector<float>& samples,
  const unsigned offsetIn,
  const unsigned lenIn)
{
  if (samples.size() < 2)
    THROW(ERR_SHORT_ACCEL_TRACE, "Accel trace length: " + 
      to_string(samples.size()));

  len = lenIn;
  offset = offsetIn;

  peaks.clear();

  // The first peak is a dummy extremum at the first sample.
  AccelDetect::logFirst(samples);

  for (unsigned i = 1; i < len-1; i++)
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


void AccelDetect::extract(const string& text)
{
  if (peaks.empty())
    return;

  const bool debug = true;
  const bool debugDetails = true;

  if (debugDetails)
  {
    const string s = "Original " + text + " peaks";
    cout << peaks.str(s, offset);
  }

  AccelDetect::reduceSmallPeaks(PEAK_PARAM_AREA, 0.1f, false);

  if (debugDetails)
  {
    const string s = "Non-tiny " + text + " peaks";
    cout << peaks.str(s, offset);
  }

  AccelDetect::eliminateKinks();

  if (debugDetails)
  {
    const string s = "Non-kinky " + text + " list peaks (first)";
    cout << peaks.str(s, offset);
  }

  AccelDetect::estimateScale();
  if (debug)
  {
    const string s = text + " scale";
    AccelDetect::printPeak(scale, s);
  }

  AccelDetect::reduceSmallPeaks(PEAK_PARAM_RANGE, 
    scale.getRange() / 10.f, true);

  if (debugDetails)
  {
    const string s = "Range-reduced " + text + " peaks";
    cout << peaks.str(s, offset);
  }

  AccelDetect::eliminateKinks();

  if (debugDetails)
  {
    const string s = "Non-kinky list " + text + " peaks (second)";
    cout << peaks.str(s, offset);
  }

  AccelDetect::completePeaks();
}


bool AccelDetect::below(
  const bool minFlag,
  const float level,
  const float threshold) const
{
  // When minFlag == false, this method returns true if the level
  // is below the threshold.
  // When minFlag == true, this method returns true if the level
  // is above the threshold.
  // When reading code, think of minFlag == false and it will work.
  // This is consistent with the suggestive method name "below".
  
  if (minFlag)
    return (level > threshold);
  else
    return (level < threshold);
}


void AccelDetect::simplifySide(
  const bool minFlag,
  const float threshold)
{
  auto pit = peaks.begin();

  while (true)
  {

    // Find the first peak "above" the threshold.
    while (pit != peaks.end() && 
        AccelDetect::below(minFlag, pit->getValue(), threshold))
      pit++;

    if (pit == peaks.end())
      return;

    auto pstart = (pit == peaks.begin() ? pit : prev(pit));

cout << "pstart: " << pstart->getIndex() << ": first above is " <<
  pit->getIndex() << endl;

    // Find the next peak "below" the threshold.
    auto pfirst = pit;
    auto pext = pit;
    while (pit != peaks.end() &&
        ! AccelDetect::below(minFlag, pit->getValue(), threshold))
    {
      // Keep track of the highest "maximum".
      if (! AccelDetect::below(minFlag, pit->getValue(), pext->getValue()))
        pext = pit;

      pit++;
    }

cout << "  extreme: " << pext->getIndex() << ", first below is " <<
  pit->getIndex() << endl;

    if (pext != pstart)
    {
      if (pstart->isMinimum() == pext->isMinimum())
      {
        // Delete [pstart, pext).
cout << "  deleting [" << pstart->getIndex() << ", " <<
  pext->getIndex() << ")" << endl;
        peaks.collapse(pstart, pext);
      }
      else
      {
        // Delete (pstart, pext).
cout << "  deleting (" << pstart->getIndex() << ", " <<
  pext->getIndex() << ")" << endl;
        peaks.collapse(next(pstart), pext);
      }
    }

    if (pext != pit && next(pext) != pit)
    {
      // Delete (pext, pit).  As pext is a "minimum" (if ! minFlag)
      // and pit must be a maximum in order to go above the threshold
      // for the first time, the polarities are always different.
cout << " deleting (" << pext->getIndex() << ", " <<
  pit->getIndex() << ")" << endl;
      peaks.collapse(next(pext), pit);
    }
  }
}


void AccelDetect::simplifySides(const float ampl)
{
  // For each stretch where the level is above -0.1 * ampl,
  // simplify to a single maximum.  These stretches will always
  // run from maximum to maximum.
  // Similarly for each stretch where there level is below +0.1 * ampl,
  // running from minimum to minimum.
  
  // Maxima.
cout << "Starting to simplify maxima" << endl;
  AccelDetect::simplifySide(false, -0.1f * ampl);

    const string s = "MAX-Simplified peaks";
    cout << peaks.str(s, offset);
    cout << endl;

  // Minima.
cout << "Starting to simplify minima" << endl;
  AccelDetect::simplifySide(true, 0.1f * ampl);
}


void AccelDetect::extractCorr(
  const float ampl,
  const string& text)
{
  // ampl is a hint at the correlation peak.
  // spacing is a hint at the spacing between two wheel.

  if (peaks.empty())
    return;

  const bool debug = true;
  const bool debugDetails = true;

  if (debugDetails)
  {
    const string s = "Original " + text + " peaks";
    cout << peaks.str(s, offset);
  }

  AccelDetect::simplifySides(ampl);

  if (debugDetails)
  {
    const string s = "Simplified " + text + " peaks";
    cout << peaks.str(s, offset);
  }

  AccelDetect::completePeaks();
}


void AccelDetect::setExtrema(
  list<Extremum>& minima,
  list<Extremum>& maxima) const
{
  unsigned i = 0;
  for (Pciterator pit = peaks.cbegin(); pit != peaks.cend(); pit++)
  {
    if (pit->isMinimum())
    {
      minima.push_back(Extremum());
      Extremum& ext = minima.back();
      ext.index = pit->getIndex();
      ext.ampl = pit->getValue();
      ext.minFlag = true;
      ext.origin = i;
    }
    else
    {
      maxima.push_back(Extremum());
      Extremum& ext = maxima.back();
      ext.index = pit->getIndex();
      ext.ampl = pit->getValue();
      ext.minFlag = false;
      ext.origin = i;
    }
    i++;
  }
}


bool AccelDetect::getLimits(
  unsigned& lower,
  unsigned& upper) const
{
  list<Extremum> minima, maxima;
  AccelDetect::setExtrema(minima, maxima);

  if (minima.size() < 2 || maxima.size() < 2)
  {
    cout << "Too few speed peaks.\n";
    return false;
  }

  minima.sort();
  maxima.sort();

  cout << "Minima\n";
  for (auto& m: minima)
    cout << m.str(offset);
  cout << "\n";

  cout << "Maxima\n";
  for (auto& m: maxima)
    cout << m.str(offset);
  cout << "\n";

  // In a well-formed bogie, the largest minimum speed is at the downward 
  // slope leading up to the first negative peak.  Then comes a pretty
  // large maximum speed on the upward slope towards the middle, 
  // followed by a pretty large minimum speed downward after the middle.
  // Finally we should get the largest positive speed on the upward
  // slope after the second peak.
  
  const Extremum& min1 = minima.front();
  const Extremum& max1 = * next(maxima.begin());
  const Extremum& min2 = * next(minima.begin());
  const Extremum& max2 = maxima.front();

  if (min1.origin + 3 != max2.origin)
  {
    cout << "Largest minima and maxima not three apart.\n";
    return false;
  }

  if (min1.origin + 2 != min2.origin)
  {
    cout << "Two largest minima not two apart.\n";
    return false;
  }

  if (max1.origin + 2 != max2.origin)
  {
    cout << "Two largest maxima not two apart.\n";
    return false;
  }

  lower = min1.index;
  upper = max2.index;
  return true;
}


void AccelDetect::printPeak(
  const Peak& peak,
  const string& text) const
{
  cout << text << "\n";
  cout << peak.strHeader();
  cout << peak.str(0) << endl;
}


string AccelDetect::printPeaks() const
{
  return peaks.str("peaks", offset);
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
