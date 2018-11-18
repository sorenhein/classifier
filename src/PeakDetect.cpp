#include <list>
#include <iostream>
#include <iomanip>
#include <functional>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakDetect.h"
#include "PeakSeeds.h"
#include "PeakMinima.h"
#include "PeakMatch.h"
#include "PeakStructure.h"
#include "PeakStats.h"
#include "Except.h"


#define SAMPLE_RATE 2000.

#define KINK_RATIO 100.f

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))



PeakDetect::PeakDetect()
{
  PeakDetect::reset();
}


PeakDetect::~PeakDetect()
{
}


void PeakDetect::reset()
{
  len = 0;
  offset = 0;
  peaks.clear();
  models.reset();
  candidates.clear();
}


float PeakDetect::integrate(
  const vector<float>& samples,
  const unsigned i0,
  const unsigned i1) const
{
  float sum = 0.f;
  for (unsigned i = i0; i < i1; i++)
    sum += samples[i];

  return sum;
}


void PeakDetect::annotate()
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


bool PeakDetect::check(const vector<float>& samples) const
{
  if (samples.size() == 0)
    THROW(ERR_SHORT_ACCEL_TRACE, "Accel trace length: " + to_string(len));

  if (peaks.size() < 2)
    THROW(ERR_NO_PEAKS, "Too few peaks: " + to_string(peaks.size()));

  const auto peakFirst = peaks.begin();
  const auto peakLast = prev(peaks.end());
  bool flag = true;

  for (auto it = peakFirst; it != peaks.end(); it++)
  {
    Peak peakSynth;
    Peak const * peakPrev = nullptr;
    float areaFull = 0.f;
    const unsigned index = it->getIndex();

    if (it != peakFirst)
    {
      peakPrev = &*prev(it);
      areaFull = peakPrev->getAreaCum() + 
        PeakDetect::integrate(samples, peakPrev->getIndex(), index);
    }

    peakSynth.log(index, it->getValue(), areaFull, it->getMaxFlag());
    peakSynth.annotate(peakPrev);

    if (! it->check(peakSynth, offset))
      flag = false;
  }
  return flag;
}


void PeakDetect::logFirst(const vector<float>& samples)
{
  peaks.emplace_back(Peak());

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


void PeakDetect::logLast(const vector<float>& samples)
{
  const auto& peakPrev = peaks.back();
  const float areaFull = 
    PeakDetect::integrate(samples, peakPrev.getIndex(), len-1);
  const float areaCumPrev = peakPrev.getAreaCum();

  peaks.emplace_back(Peak());
  peaks.back().log(
    samples.size()-1, 
    samples[samples.size()-1], 
    areaCumPrev + areaFull, 
    ! peakPrev.getMaxFlag());
}


void PeakDetect::log(
  const vector<float>& samples,
  const unsigned offsetSamples)
{
  len = samples.size();
  if (len < 2)
    THROW(ERR_SHORT_ACCEL_TRACE, "Accel trace length: " + to_string(len));

  offset = offsetSamples;
  peaks.clear();

  // The first peak is a dummy extremum at the first sample.
  PeakDetect::logFirst(samples);

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
    const float areaFull = PeakDetect::integrate(samples, 
      peakPrev.getIndex(), i);
    const float areaCumPrev = peakPrev.getAreaCum();

    // The peak contains data for the interval preceding it.
    peaks.emplace_back(Peak());
    peaks.back().log(i, samples[i], areaCumPrev + areaFull, maxFlag);
  }

  // The last peak is a dummy extremum at the last sample.
  PeakDetect::logLast(samples);

  PeakDetect::annotate();

  PeakDetect::check(samples);
}


const list<Peak>::iterator PeakDetect::collapsePeaks(
  const list<Peak>::iterator peak1,
  const list<Peak>::iterator peak2)
{
  // Analogous to list.erase(), peak1 does not survive, while peak2 does.
  if (peak1 == peak2)
    return peak1;

  Peak * peak0 = 
    (peak1 == peaks.begin() ? nullptr : &*prev(peak1));

  peak2->update(peak0);

  return peaks.erase(peak1, peak2);
}


void PeakDetect::reduceSmallPeaks(
  const PeakParam param,
  const float paramLimit,
  const bool preserveFlag)
{
  // We use this method for two reductions:
  // 1. Small areas (first, as a rough reduction).
  // 2. Small ranges (second, as a more precise reduction).
  // preserveFlag regulates whether we may reduce away certain
  // positive peaks.  As this changes the perceived slope of 
  // negative minimum peaks, we only do it for the first reduction.

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

    auto peakCurrent = peak, peakMax = peak;
    const bool maxFlag = peak->getMaxFlag();
    float sumParam = 0.f, lastParam = 0.f;
    float valueMax = numeric_limits<float>::lowest();

    do
    {
      peak++;
      if (peak == peaks.end())
        break;

      sumParam = peak->getParameter(* peakCurrent, param);
      lastParam = peak->getParameter(param);
      const float value = peak->getValue();
      if (! maxFlag && value > valueMax)
      {
        valueMax = value;
        peakMax = peak;
      }
      else if (maxFlag && -value > valueMax)
      {
        valueMax = -value;
        peakMax = peak;
      }
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
    else if (preserveFlag &&
        peakCurrent->getValue() > 0.f &&
        ! peakCurrent->getMaxFlag())
    {
      // Don't connect two positive maxima.  This can mess up
      // the gradient calculation which influences peak perception.
      peak++;
    }
    else if (peak->getMaxFlag() != maxFlag)
    {
      // Keep from peakCurrent to peak which is also often peakMax.
      peak = PeakDetect::collapsePeaks(--peakCurrent, peak);
      peak++;
    }
    else
    {
      // Keep the start, the most extreme peak of opposite polarity,
      // and the end.
      peakMax = PeakDetect::collapsePeaks(--peakCurrent, peakMax);
      peak = PeakDetect::collapsePeaks(++peakMax, peak);
      peak++;
    }
  }
}


void PeakDetect::eliminateKinks()
{
  if (peaks.size() < 4)
    THROW(ERR_NO_PEAKS, "Peak list is short");

  for (auto peak = next(peaks.begin(), 2); 
      peak != prev(peaks.end()); )
  {
// cout << "peak " << peak->getIndex() << " count " << peaks.size() <<
  // endl;
    const auto peakPrev = prev(peak);
    const auto peakPrevPrev = prev(peakPrev);
    const auto peakNext = next(peak);

    const float areaPrev = peak->getArea(* peakPrev);
    if (areaPrev == 0.f)
      THROW(ERR_ALGO_PEAK_COLLAPSE, "Zero area");

    const float ratioPrev = 
      peakPrev->getArea(* peakPrevPrev) / areaPrev;
    const float ratioNext = peakNext->getArea(* peak) / areaPrev;

    if (ratioPrev > 1.f && 
        ratioNext > 1.f &&
        ratioPrev * ratioNext > KINK_RATIO)
    {
      // Candidate for removal.  But if it changes the gradient
      // too much, don't do it.

      if (peakPrev->similarGradient(* peak, * peakNext))
        peak = PeakDetect::collapsePeaks(peakPrev, peakNext);
      else
        peak++;
    }
    else
      peak++;
  }
}


void PeakDetect::estimateScale(Peak& scale)
{
  scale.reset();
  unsigned no = 0;
  for (auto peak = next(peaks.begin()); peak != peaks.end(); peak++)
  {
    if (! peak->getMaxFlag() && peak->getValue() < 0.f)
    {
      scale += * peak;
      no++;
    }
  }

  if (no == 0)
    THROW(ERR_NO_PEAKS, "No negative minima");

  scale/= no;
}


void PeakDetect::reduce()
{
  if (peaks.empty())
    return;

  const bool debug = true;
  const bool debugDetails = false;

  if (debugDetails)
    PeakDetect::printAllPeaks("Original peaks");

  PeakDetect::reduceSmallPeaks(PEAK_PARAM_AREA, 0.1f, false);

  if (debugDetails)
    PeakDetect::printAllPeaks("Non-tiny peaks");

  PeakDetect::eliminateKinks();
  if (debugDetails)
    PeakDetect::printAllPeaks("Non-kinky list peaks");

  Peak scale;
  PeakDetect::estimateScale(scale);
  if (debug)
    PeakDetect::printPeak(scale, "Scale");

  PeakDetect::reduceSmallPeaks(PEAK_PARAM_RANGE, 
    scale.getRange() / 10.f, true);

  // Mark some tall peaks as seeds.
  PeakSeeds seeds;
  seeds.mark(peaks, offset, scale.getRange());
  cout << seeds.str(offset, "after pruning");

  // Label the negative minima as wheels, bogeys etc.
  PeakMinima minima;
  minima.mark(peaks, candidates, offset);

  // Use the labels to extract the car structure from the peaks.
  PeakStructure pstruct;
  vector<CarDetect> cars;
  pstruct.markCars(models, cars, candidates, offset);
}


void PeakDetect::logPeakStats(
  const vector<PeakPos>& posTrue,
  const string& trainTrue,
  const double speedTrue,
  PeakStats& peakStats)
{
  PeakMatch matches;
  matches.logPeakStats(peaks, posTrue, trainTrue, speedTrue, peakStats);
}


void PeakDetect::makeSynthPeaks(vector<float>& synthPeaks) const
{
  for (unsigned i = 0; i < synthPeaks.size(); i++)
    synthPeaks[i] = 0;

  for (auto& peak: peaks)
  {
    if (peak.isSelected())
      synthPeaks[peak.getIndex()] = peak.getValue();
  }
}


float PeakDetect::getFirstPeakTime() const
{
  for (auto& peak: peaks)
  {
    if (peak.isSelected())
      return peak.getIndex() / static_cast<float>(SAMPLE_RATE);
  }

  THROW(ERR_NO_PEAKS, "No peaks selected");
}


void PeakDetect::getPeakTimes(vector<PeakTime>& times) const
{
  times.clear();
  const float t0 = PeakDetect::getFirstPeakTime();

  for (auto& peak: peaks)
  {
    if (peak.isSelected())
    {
      times.emplace_back(PeakTime());
      PeakTime& p = times.back();
      p.time = peak.getIndex() / SAMPLE_RATE - t0;
      p.value = peak.getValue();
    }
  }
}


void PeakDetect::printPeak(
  const Peak& peak,
  const string& text) const
{
  cout << text << "\n";
  cout << peak.strHeader();
  cout << peak.str(0) << endl;
}


void PeakDetect::printRange(
  const unsigned start,
  const unsigned end,
  const string& text) const
{
  cout << text << " " << start + offset << "-" << end + offset << endl;
}


void PeakDetect::printWheelCount(const string& text) const
{
  unsigned count = 0;
  for (auto cand: candidates)
  {
    if (cand->isWheel())
      count++;
  }
  cout << text << " " << count << " peaks" << endl;
}


void PeakDetect::printCars(
  const vector<CarDetect>& cars,
  const string& text) const
{
  if (cars.empty())
    return;

  cout << "Cars " << text << "\n";
  cout << cars.front().strHeaderFull();

  for (unsigned cno = 0; cno < cars.size(); cno++)
  {
    cout << cars[cno].strFull(cno, offset);
    if (cno+1 < cars.size() &&
        cars[cno].endValue() != cars[cno+1].startValue())
      cout << string(56, '.') << "\n";
  }
  cout << endl;
}


void PeakDetect::printCarStats(const string& text) const
{
  cout << "Car stats " << text << "\n";
  cout << models.str();
}


void PeakDetect::printPeaksCSV(const vector<PeakTime>& timesTrue) const
{
  cout << "true\n";
  unsigned i = 0;
  for (auto tt: timesTrue)
    cout << i++ << ";" << fixed << setprecision(6) << tt.time << "\n";

  cout << "\nseen\n";
  i = 0;
  for (auto& peak: peaks)
  {
    if (peak.isSelected())
      cout << i++ << ";" << 
        fixed << setprecision(6) << peak.getTime() << "\n";
  }
  cout << endl;
}


void PeakDetect::printAllPeaks(const string& text) const
{
  if (peaks.empty())
    return;

  if (text != "")
    cout << text << ": " << peaks.size() << "\n";
  cout << peaks.front().strHeader();

  for (auto& peak: peaks)
    cout << peak.str(offset);
  cout << endl;
}

