#include <list>
#include <iostream>
#include <iomanip>
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
#define KINK_RATIO_CONSIDER 20.f
#define KINK_RATIO_ONE_DIM 5.f


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
  pstruct.reset();
}


float PeakDetect::integrate(
  const vector<float>& samples,
  const unsigned i0,
  const unsigned i1) const
{
  float sum = 0.f;
  for (unsigned i = i0; i < i1; i++)
  // for (unsigned i = i0; i <= i1; i++)
  // for (unsigned i = i0+1; i <= i1; i++)
    sum += samples[i];

  sum += 0.5f * (samples[i1] - samples[i0]);

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

cout << "UPDATING peak2\n";
cout << peak2->strQuality(offset) << endl;
cout << "with" << endl;
if (peak0 != nullptr)
{
cout << peak0->strQuality(offset) << endl;
  peak2->update(peak0);
}
cout << peak2->strQuality(offset) << endl;


  return peaks.erase(peak1, peak2);
}


string PeakDetect::deleteStr(
  Peak const * p1,
  Peak const * p2) const
{
  stringstream ss;
  if (p1 == p2)
    ss << "DELETE " << p1->getIndex() + offset << endl;
  else
    ss << "DELETE [" << p1->getIndex() + offset << ", " <<
      p2->getIndex() + offset << "]\n";
  
  return ss.str();
}


void PeakDetect::reduceSmallPeaks(
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


cout << "REDUCE " << (flagExtreme ? "MAX" : "MIN") << "\n";
cout << peak->strHeader();
for (auto p = peakFirst; p != peak; p++)
  cout << p->str(offset);
cout << peak->str(offset);
cout << "EXTREME " << peakExtreme->getIndex() + offset << endl << endl;


      // Determine whether the peak has an extent.
      Peak const * peakL;
      if (peakFirst->similarGradientForward(* peakExtreme))
        peakL = &*peakExtreme;
      else
      {
        // peakExtreme and peakFirst have the same polarity.
        peakL = &*peakFirst;
        cout << "Left extent " << 
          peakExtreme->getIndex() - peakL->getIndex() << endl;
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

        cout << "Right extent " << 
          peakR->getIndex() - peakExtreme->getIndex() << endl;
      }
      
      peakExtreme->logExtent(* peakL, * peakR);

      // Do the deletions.
      if (peakFirst != peakExtreme)
      {
        cout << PeakDetect::deleteStr(&*peakFirst, &*prev(peakExtreme));
        PeakDetect::collapsePeaks(peakFirst, peakExtreme);
      }

      peakExtreme++;
      if (peakExtreme != peak)
      {
        cout << PeakDetect::deleteStr(&*peakExtreme, &*prev(peak));
        PeakDetect::collapsePeaks(peakExtreme, peak);
      }

      peak++;
      continue;
    }
    else if (peakFirst->getValue() < 0.f && peak->getValue() < 0.f)
    {
      // Any kink on the negative side of the axis.

cout << "EXP " << (flagExtreme ? "MAXKINK" : "MINKINK") << "\n";
cout << peak->strHeader();
for (auto p = peakFirst; p != peak; p++)
  cout << p->str(offset);
cout << peak->str(offset);

        cout << PeakDetect::deleteStr(&*peakFirst, &*prev(peak));
        PeakDetect::collapsePeaks(peakFirst, peak);

      peak++;
      continue;
    }
    else if (preserveKinksFlag)
    {
/*
cout << "DONTTOUCH " << (flagExtreme ? "MAXKINK" : "MINKINK") << "\n";
cout << peak->strHeader();
for (auto p = peakFirst; p != peak; p++)
  cout << p->str(offset);
cout << peak->str(offset);
*/

      peak++;
      continue;
    }
    else
    {
      // We have a kink.  It's debatable if this should lead to
      // an extent.  Here we just delete everything.
      
cout << "REDUCE " << (flagExtreme ? "MAXKINK" : "MINKINK") << "\n";
cout << peak->strHeader();
for (auto p = peakFirst; p != peak; p++)
  cout << p->str(offset);
cout << peak->str(offset);

      // peak->logExtent(* peakFirst, * peak);

        cout << PeakDetect::deleteStr(&*peakFirst, &*prev(peak));
        PeakDetect::collapsePeaks(peakFirst, peak);

      peak++;
      continue;
    }
  }
}


void PeakDetect::eliminateKinks()
{
  if (peaks.size() < 4)
    THROW(ERR_NO_PEAKS, "Peak list is short");

  for (auto peak = next(peaks.begin(), 2); peak != prev(peaks.end()); )
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

    if (ratioPrev * ratioNext > KINK_RATIO)
    {
      peak = PeakDetect::collapsePeaks(peakPrev, peakNext);
      continue;
    }

    const unsigned lenWhole = peakNext->getIndex() - peakPrevPrev->getIndex();
    const unsigned lenMid = peak->getIndex() - peakPrev->getIndex();

    const float vWhole = abs(peakNext->getValue() - peakPrevPrev->getValue());
    const float vMid = abs(peak->getValue() - peakPrev->getValue());

    if (KINK_RATIO_ONE_DIM * lenMid < lenWhole &&
        KINK_RATIO_ONE_DIM * vMid < vWhole)
    {
      // Another way of looking like a kink.
      peak = PeakDetect::collapsePeaks(peakPrev, peakNext);
    }
    else
      peak++;
  }
}


void PeakDetect::eliminateKinksNew()
{
  if (peaks.size() < 4)
    THROW(ERR_NO_PEAKS, "Peak list is short");

  for (auto peak = next(peaks.begin(), 2); peak != prev(peaks.end()); )
  {
    const auto peakPrev = prev(peak);
    const auto peakPrevPrev = prev(peakPrev);
    const auto peakNext = next(peak);

    if (peak->isMinimum() != peakPrevPrev->isMinimum() ||
        peakNext->isMinimum() != peakPrev->isMinimum())
    {
      // We may have destroyed the alternation while reducing.
      peak++;
      continue;
    }

    const float areaPrev = peak->getArea(* peakPrev);
    if (areaPrev == 0.f)
      THROW(ERR_ALGO_PEAK_COLLAPSE, "Zero area");

    const float ratioPrev = 
      peakPrev->getArea(* peakPrevPrev) / areaPrev;
    const float ratioNext = peakNext->getArea(* peak) / areaPrev;

    if (ratioPrev > 1.f && 
        ratioNext > 1.f &&
        ratioPrev * ratioNext > KINK_RATIO_CONSIDER)
    {
      /*
      if (peakPrev->getValue() < 0.f &&
          ratioPrev * ratioNext > KINK_RATIO)
      {
  cout << "Kink match on ratios" << endl;
  cout << peakPrevPrev->strQuality(offset);
  cout << peakPrev->strQuality(offset);
  cout << peak->strQuality(offset);
  cout << peakNext->strQuality(offset) << endl;

        // Comparably small-area kink, first kink below the line.
        peak = PeakDetect::collapsePeaks(peakPrev, peakNext);
      }
      else 
      */
      if (peakPrev->similarGradientTwo(* peak, * peakNext))
      {
        // Candidate for removal.  But if it changes the gradient
        // too much, don't do it.
  cout << "Kink match on gradient" << endl;
  cout << peakPrevPrev->strQuality(offset);
  cout << peakPrev->strQuality(offset);
  cout << peak->strQuality(offset);
  cout << peakNext->strQuality(offset) << endl;

        // Gradients match roughly.
        peak = PeakDetect::collapsePeaks(peakPrev, peakNext);
      }
      /*
      else if (peakPrev->getValue() < 0.f &&
          5 * (peak->getIndex() - peakPrev->getIndex()) <
          peakNext->getIndex() - peakPrevPrev->getIndex())
      {
  cout << "Kink match on look" << endl;
  cout << peakPrevPrev->strQuality(offset);
  cout << peakPrev->strQuality(offset);
  cout << peak->strQuality(offset);
  cout << peakNext->strQuality(offset) << endl;

  const unsigned lenWhole = peakNext->getIndex() - peakPrevPrev->getIndex();
  const unsigned lenMid = peak->getIndex() - peakPrev->getIndex();

  const float vWhole = abs(peakNext->getValue() - peakPrevPrev->getValue());
  const float vMid = abs(peak->getValue() - peakPrev->getValue());

  bool viol = false;
  if (peak->getMaxFlag())
  {
    if (peak->getValue() > peakPrevPrev->getValue() ||
       peakPrev->getValue() < peakNext->getValue())
      viol = true;
  }
  else
  {
    if (peak->getValue() < peakPrevPrev->getValue() ||
       peakPrev->getValue() > peakNext->getValue())
      viol = true;
  }

  cout << "KINK " << setw(8) << setprecision(2) << ratioPrev << " * " << 
    setw(8) << ratioNext << " = " <<
    setw(8) << ratioPrev * ratioNext << ", " <<
    setw(8) << lenWhole / static_cast<float>(lenMid) << " and " <<
    setw(8) << vWhole / vMid << (viol ? " ***" : "") << endl;

        // Kink, short in time, below the zero line.
        peak = PeakDetect::collapsePeaks(peakPrev, peakNext);
      }
      */
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


void PeakDetect::reduce(Imperfections& imperf)
{
  if (peaks.empty())
    return;

  const bool debug = true;
  const bool debugDetails = true;

  if (debugDetails)
    PeakDetect::printAllPeaks("Original peaks");

  PeakDetect::reduceSmallPeaks(PEAK_PARAM_AREA, 0.1f, false);

  if (debugDetails)
    PeakDetect::printAllPeaks("Non-tiny peaks");

  /* */
  PeakDetect::eliminateKinks();
  if (debugDetails)
    PeakDetect::printAllPeaks("Non-kinky list peaks (first)");
  /* */

  Peak scale;
  PeakDetect::estimateScale(scale);
  if (debug)
    PeakDetect::printPeak(scale, "Scale");

  PeakDetect::reduceSmallPeaks(PEAK_PARAM_RANGE, 
    scale.getRange() / 10.f, true);

  if (debugDetails)
    PeakDetect::printAllPeaks("Range-reduced peaks");

  /* */
  // PeakDetect::eliminateKinksNew();
  PeakDetect::eliminateKinks();
  if (debugDetails)
    PeakDetect::printAllPeaks("Non-kinky list peaks (second)");
  /* */

  // Mark some tall peaks as seeds.
  PeakSeeds seeds;
  seeds.mark(peaks, offset, scale.getRange());
  cout << seeds.str(offset, "after pruning");

  // Label the negative minima as wheels, bogeys etc.
  PeakMinima minima;
  minima.mark(peaks, candidates, offset);

  // Use the labels to extract the car structure from the peaks.
  vector<CarDetect> cars;
  pstruct.markCars(models, cars, candidates, offset);

  pstruct.markImperfections(cars, candidates, imperf);
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

