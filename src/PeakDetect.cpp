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
cout << peak2->strQuality() << endl;
cout << "with" << endl;
if (peak0 != nullptr)
{
cout << peak0->strQuality() << endl;
  peak2->update(peak0);
}
cout << peak2->strQuality() << endl;

  // TODO Do it here, or in Minima?
// if (peak0 != nullptr)
  // peak0->logNextPeak(&*peak2);

  return peaks.erase(peak1, peak2);
}


void PeakDetect::printTMP(
  list<Peak>::iterator peakCurrent,
  list<Peak>::iterator peakMax,
  list<Peak>::iterator peak,
  const string& place) const
{
  // setw(7) << fixed << setprecision(4) << 
  // peakCurrent->getValue() << " " << 
  // peakCurrent->getIndex() + offset <<

  cout << "PEAK " << place << ", " <<
    (peakCurrent->getMaxFlag() ? "max" : "min") <<
    (peakCurrent->getValue() >= 0. ? " + " : " - ") <<
    (peakMax->getMaxFlag() ? "max" : "min") <<
    (peakMax->getValue() >= 0. ? " + " : " - ") <<
    (peak->getMaxFlag() ? "max" : "min") <<
    (peak->getValue() >= 0. ? " + " : " - ") <<
    (peak == peakMax ? "  =" : " !=") <<
    endl;
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


void PeakDetect::reduceSmallPeaksNew(
  const PeakParam param,
  const float paramLimit)
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
PeakDetect::printTMP(peakCurrent, peakMax, peak, "P1");
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



    auto peakLeft = peakFirst;
    auto peakRight = prev(peak);

    for (auto p = peakFirst; p != peak; p++)
    {
      if (p->getValue() < peakLeft->getValue() &&
          peakFirst->similarGradientForward(* p))
        peakLeft = p;

      if (p == peaks.begin())
        continue;

      auto prevp = prev(p);
      if (prevp->getValue() < peakRight->getValue() &&
          p->similarGradientBackward(* peak))
        peakRight = prevp;
    }
    

cout << "REDUCE\n";
cout << peakLeft->strHeader();
for (auto p = peakFirst; p != peak; p++)
  cout << p->str(offset);
cout << peak->str(offset);
cout << "LEFT/RIGHT " << peakLeft->getIndex() + offset << ", " <<
  peakRight->getIndex() + offset << ", MAX " <<
  peakMax->getIndex() + offset << endl << endl;



    // It can happen that we can delete everything, seen from the
    // right.
    // if (peakRight->getIndex() < peakFirst->getIndex())
    // {
      // auto peakEarly = prev(peakFirst);
      // cout << PeakDetect::deleteStr(&*peakEarly, &*prev(peak));
      // PeakDetect::collapsePeaks(peakEarly, peak);
    // }
    // else 
    if (peakLeft->getIndex() >= peakRight->getIndex())
    // else if (peakLeft == peakRight)
    {
      // Several solutions unless the two are the same, but we use peakLeft.
      if (peakFirst != peakLeft)
      {
        cout << PeakDetect::deleteStr(&*peakFirst, &*prev(peakLeft));
        PeakDetect::collapsePeaks(peakFirst, peakLeft);
      }

      // peakRight++;
      peakLeft++;
      // if (peakRight != peak)
      if (peakLeft != peak)
      {
        cout << PeakDetect::deleteStr(&*peakLeft, &*prev(peak));
        PeakDetect::collapsePeaks(peakLeft, peak);
        // cout << PeakDetect::deleteStr(&*peakRight, &*prev(peak));
        // PeakDetect::collapsePeaks(peakRight, peak);
      }
    }
    else
    {
      peakMax = (peakLeft->getValue() < peakRight->getValue() ?
        peakLeft : peakRight);
      
      cout << "MAX " << peakMax->getIndex() + offset << ": " <<
        peakLeft->getIndex() + offset << "-" <<
        peakRight->getIndex() + offset << "\n";

      peakMax->logExtent(* peakFirst, * peakRight);

      if (peakFirst != peakMax)
      {
        cout << PeakDetect::deleteStr(&*peakFirst, &*prev(peakMax));
        PeakDetect::collapsePeaks(peakFirst, peakMax);
      }

      peakMax++;
      if (peakMax != peak)
      {
        cout << PeakDetect::deleteStr(&*peakMax, &*prev(peak));
        PeakDetect::collapsePeaks(peakMax, peak);
      }

      /*
      if (peakFirst != peakLeft)
      {
        cout << PeakDetect::deleteStr(&*peakFirst, &*prev(peakLeft));
        PeakDetect::collapsePeaks(peakFirst, peakLeft);
      }

      peakLeft++;
      if (peakLeft != peakRight)
      {
        cout << PeakDetect::deleteStr(&*peakLeft, &*prev(peakRight));
        PeakDetect::collapsePeaks(peakLeft, peakRight);
      }

      peakRight++;
      if (peakRight != peak)
      {
        cout << PeakDetect::deleteStr(&*peakRight, &*peak);
        PeakDetect::collapsePeaks(peakRight, peak);
      }
      */
    }
    cout << endl;


    // TODO: similarGradient for both peakPrev-1 to peak forwards,
    // and from peak to peakPrev-1 backwards.


/*
if (peak->getIndex()+offset > 4500)
  cout << "HERE " << peak->getIndex() + offset << endl;
PeakDetect::printTMP(peakCurrent, peakMax, peak, "P2");
cout << peakCurrent->getIndex() + offset<< " " <<
  peakMax->getIndex() + offset << " " << 
  peak->getIndex() + offset << endl;

    // Preserve negative minima, otherwise erase all the way to peak.
    list<Peak>::iterator
      peakAfterCurr = (peakMax->isCandidate() ? peakMax : peak);

    // Leave peakCurrent if it would change the gradient too much.
    if (! peakCurrent->similarGradientOne(* peakAfterCurr))
      peakCurrent++;

cout << 
  "  curr " << peakCurrent->getIndex() + offset << 
  " afterCurr " << peakAfterCurr->getIndex() + offset << endl;

    peakAfterCurr = PeakDetect::collapsePeaks(peakCurrent, peakAfterCurr);

    if (peakMax != peak && peakMax->isCandidate())
    {
cout << "  max " << peakMax->getIndex() + offset << " " << 
  peak->getIndex() + offset << endl;
      PeakDetect::collapsePeaks(++peakMax, peak);
    }
    */
    
cout << "peak was " << peak->getIndex() + offset << ", is ";
    peak++;
cout << peak->getIndex() + offset << endl;
  }
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

if (preserveFlag && peak->getIndex() > 1500)
  cout << peakCurrent->getIndex() + offset << " " <<
    peakMax->getIndex() + offset << " " <<
    peak->getIndex() + offset << endl;
/*
if (preserveFlag && peak->getIndex() > 11500)
{
  cout << 
    "PEAK curr " << 
    setw(7) << fixed << setprecision(4) << peakCurrent->getValue() <<
    " " << peakCurrent->getIndex() + offset <<
    (peakCurrent->getMaxFlag() ? " max" : " min") <<
    ", max " << peakMax->getIndex() + offset <<
    (peakMax->getMaxFlag() ? " max" : " min") <<
    (peakMax->getValue() >= 0.f ? " pos" : " neg") <<
    ", peak " << peak->getIndex() + offset <<
    (peak->getMaxFlag() ? " max" : " min") <<
    (peak->getValue() >= 0.f ? " pos" : " neg") <<
    " mf " << (maxFlag ? "true" : "false") <<
    "\n";
}
*/
    if (abs(sumParam) < paramLimit || abs(lastParam) < paramLimit)
    {
//if (preserveFlag && peak->getIndex() > 11500)
  //cout << "PATH1\n";
      // It's the last set of peaks.  We could keep the largest peak
      // of the same polarity as peakCurrent (instead of peakCurrent).
      // It's a bit random whether or not this would be a "real" peak,
      // and we also don't keep track of this above.  So we just stop.
if (preserveFlag)
  PeakDetect::printTMP(peakCurrent, peakMax, peak, "P1");
      if (peakCurrent != peaks.end())
        peaks.erase(peakCurrent, peaks.end());
      break;
    }
    else if (preserveFlag &&
        peakCurrent->getValue() > 0.f && 
        ! peakCurrent->getMaxFlag())
        // THOUGHT: maxFlag true, pcurr true, next false,
        // values pos pos any, don't do
    {
//if (preserveFlag && peak->getIndex() > 11500)
  //cout << "PATH2\n";
if (preserveFlag)
  PeakDetect::printTMP(peakCurrent, peakMax, peak, "P2");
      // Don't connect two positive maxima.  This can mess up
      // the gradient calculation which influences peak perception.
      peak++;
    }
    // else if (peak->getMaxFlag() != maxFlag)
    else if (peak->getMaxFlag() != maxFlag ||
        peak == peakMax)
    {
//if (preserveFlag && peak->getIndex() > 11500)
  //cout << "PATH3\n";
if (preserveFlag)
  PeakDetect::printTMP(peakCurrent, peakMax, peak, "P3");
      // Keep from peakCurrent to peak which is also often peakMax.
      peak = PeakDetect::collapsePeaks(--peakCurrent, peak);
      peak++;
    }
    else
    {
//if (preserveFlag && peak->getIndex() > 11500)
  //cout << "PATH4\n";
      // Keep the start, the most extreme peak of opposite polarity,
      // and the end.
if (preserveFlag)
  PeakDetect::printTMP(peakCurrent, peakMax, peak, "P4");
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

  for (auto peak = next(peaks.begin(), 2); peak != prev(peaks.end()); )
  {
    const auto peakPrev = prev(peak);
    const auto peakPrevPrev = prev(peakPrev);
    const auto peakNext = next(peak);


    if (peakNext->isMinimum() != peakPrev->isMinimum() &&
        ((peakNext->getValue() > peak->getValue() &&
         peak->getValue() > peakPrev->getValue()) ||
         (peakNext->getValue() < peak->getValue() &&
         peak->getValue() < peakPrev->getValue())))
    {
if (peak->getIndex() + offset == 2025)
  cout << "HERE" << endl;
      // Three in order, with first and last having different polarity.
      if (peak->similarGradientBest(* peakNext))
      {
cout << "Best\n";
        peak = PeakDetect::collapsePeaks(peak, peakNext);
      }
      else
      {
cout << "Not best\n";
        peak++;
      }
      continue;
    }

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
        ratioPrev * ratioNext > KINK_RATIO)
    {
      // Candidate for removal.  But if it changes the gradient
      // too much, don't do it.

      if (peakPrev->similarGradientTwo(* peak, * peakNext))
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
  const bool debugDetails = true;

  if (debugDetails)
    PeakDetect::printAllPeaks("Original peaks");

  // PeakDetect::reduceSmallPeaks(PEAK_PARAM_AREA, 0.1f, false);
  PeakDetect::reduceSmallPeaksNew(PEAK_PARAM_AREA, 0.1f);

  if (debugDetails)
    PeakDetect::printAllPeaks("Non-tiny peaks");

  PeakDetect::eliminateKinks();
  if (debugDetails)
    PeakDetect::printAllPeaks("Non-kinky list peaks");

  Peak scale;
  PeakDetect::estimateScale(scale);
  if (debug)
    PeakDetect::printPeak(scale, "Scale");

  // PeakDetect::reduceSmallPeaks(PEAK_PARAM_RANGE, 
    // scale.getRange() / 10.f, true);
  PeakDetect::reduceSmallPeaksNew(PEAK_PARAM_RANGE, scale.getRange() / 10.f);

  if (debugDetails)
    PeakDetect::printAllPeaks("Range-reduced peaks");

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

