#include <list>
#include <iostream>
#include <iomanip>
#include <functional>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <random>

#include "PeakDetect.h"
#include "PeakCluster.h"
#include "Except.h"

#define SMALL_AREA_FACTOR 100.f

// Only a check limit, not algorithmic parameters.
#define ABS_RANGE_LIMIT 1.e-4
#define ABS_AREA_LIMIT 1.e-4

#define SAMPLE_RATE 2000.

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
  peaks.clear();
}


float PeakDetect::integral(
  const vector<float>& samples,
  const unsigned i0,
  const unsigned i1,
  const float refval) const
{
  float sum = 0.f;
  for (unsigned i = i0; i < i1; i++)
    sum += samples[i] - refval;

  return abs(sum);
}


float PeakDetect::integralList(
  const vector<float>& samples,
  const unsigned i0,
  const unsigned i1) const
{
  float sum = 0.f;
  for (unsigned i = i0; i < i1; i++)
    sum += samples[i];

  return sum;
}


void PeakDetect::annotateList()
{
  if (peakList.size() < 2)
    THROW(ERR_NO_PEAKS, "Too few peaks: " + to_string(peakList.size()));

  const auto peakFirst = peakList.begin();
  const auto peakLast = prev(peakList.end());

  for (auto it = peakFirst; it != peakList.end(); it++)
  {
    const Peak * peakPrev = (it == peakFirst ? nullptr : &*prev(it));
    const Peak * peakNext = (it == peakLast ? nullptr : &*next(it));

    it->annotate(peakPrev, peakNext);
  }
}


bool PeakDetect::checkList(const vector<float>& samples) const
{
  if (samples.size() == 0)
    THROW(ERR_SHORT_ACCEL_TRACE, "Accel trace length: " + to_string(len));

  if (peakList.size() < 2)
    THROW(ERR_NO_PEAKS, "Too few peaks: " + to_string(peakList.size()));

  const auto peakFirst = peakList.begin();
  const auto peakLast = prev(peakList.end());
  bool flag = true;

  for (auto it = peakFirst; it != peakList.end(); it++)
  {
    Peak peakSynth;
    Peak const * peakPrev = nullptr, * peakNext = nullptr;
    float areaFull = 0.f;
    const unsigned index = it->getIndex();

    if (it != peakFirst)
    {
      peakPrev = &*prev(it);
      areaFull = peakPrev->getAreaCum() + 
        PeakDetect::integralList(samples, peakPrev->getIndex(), index);
    }

    if (it != peakLast)
      peakNext = &*next(it);

    peakSynth.log(index, it->getValue(), areaFull, it->getMaxFlag());
    peakSynth.annotate(peakPrev, peakNext);

    if (! it->check(peakSynth, offset))
      flag = false;
  }
  return flag;
}


bool PeakDetect::check(const vector<float>& samples) const
{
  bool flag = true;
  const unsigned lp = peaks.size();
  if (lp == 0)
    return true;

  float sumLeft = 0.f, sumRight = 0.f;
  for (unsigned i = 0; i < lp; i++)
  {
    const PeakData& peak = peaks[i];
    if (samples[peak.index] != peak.value)
    {
      cout << 
        "i " << i << 
        ": value " << samples[peak.index] <<
        " vs. " << peak.value << endl;
      flag = false;
    }

    if (i == 0)
    {
      if (peak.left.len != peak.index)
      {
        cout << 
          "i " << i << 
          ": left flank " << peak.left.len <<
          " vs. " << peak.index << endl;
        flag = false;
      }

      if (abs(peak.left.range - abs(peak.value - samples[0])) > 
          ABS_RANGE_LIMIT)
      {
        cout << 
          "i " << i << 
          ": left range " << peak.left.range <<
          " vs. value " << peak.value << 
          ", sp[0] " << samples[0] << endl;
        flag = false;
      }

      const unsigned pi = peak.index;
      const float a = 
        PeakDetect::integral(samples, 0, pi, samples[pi]);
      if (abs(peak.left.area - a) > ABS_AREA_LIMIT)
      {
        cout << 
          "i " << i << 
          ": left area " << peak.left.area <<
          " vs. value " << a << endl;
      }

    }
    else if (i == lp-1)
    {
      if (peak.right.len + peak.index + 1 != samples.size())
      {
        cout << 
          "i " << i << 
          ": right flank " << peak.right.len <<
          " vs. " << peak.index << endl;
        flag = false;
      }

      if (abs(peak.right.range - 
          abs(peak.value - samples.back())) > ABS_RANGE_LIMIT)
      {
        cout << 
          "i " << i << 
          ": right range " << peak.right.range <<
          " vs. value " << peak.value << 
          ", sp[last] " << samples.back() << endl;
        flag = false;
      }

      const unsigned pi = peak.index;
      const float a =
        PeakDetect::integral(samples, pi, len, samples[pi]);
      if (abs(peak.right.area - a) > ABS_AREA_LIMIT)
      {
        cout << 
          "i " << i << 
          ": right area " << peak.right.area <<
          " vs. value " << a << endl;
      }
    }
    else
    {
      if (peak.left.len != peaks[i-1].right.len)
      {
        cout << 
          "i " << i << 
          ": left flank LR " << peak.left.len <<
          " vs. " << peaks[i-1].right.len << endl;
        flag = false;
      }

      if (peak.left.range != peaks[i-1].right.range)
      {
        cout << 
         "i " << i << 
         ": left range LR " << peak.left.range <<
          " vs. " << peaks[i-1].right.range << endl;
        flag = false;
      }

      float ev = abs(peak.value - peaks[i-1].value);
      if (abs(peak.left.range - ev) > ABS_RANGE_LIMIT)
      {
        cout << 
          "i " << i << 
          ": left range LV " << peak.left.range <<
          " vs. value " << ev << 
          ", diff " << abs(peak.left.range - ev) << endl;
        flag = false;
      }

      if (peak.left.area != peaks[i-1].right.area)
      {
        cout << 
         "i " << i << 
         ": left area LR " << peak.left.area <<
          " vs. " << peaks[i-1].right.area << endl;
        flag = false;
      }

      ev = PeakDetect::integral(samples, peaks[i-1].index,
        peak.index, samples[peak.index]);
      if (abs(peak.left.area - ev) > ABS_AREA_LIMIT)
      {
        cout << 
          "i " << i << 
          ": left area LV " << peak.left.area <<
          " vs. value " << ev <<
          ", diff " << abs(peak.left.range - ev) << endl;
        flag = false;
      }
    }

    if (i > 0)
    {
      if (peak.maxFlag == peaks[i-1].maxFlag)
      {
        cout << 
          "i " << i << 
          " maxFlag " << peak.maxFlag <<
          " is same as predecessor" << endl;
        flag = false;
      }
    }

    sumLeft += peak.left.len;
    sumRight += peak.right.len;
  }

  float ev = samples.size() - peaks.back().right.len - 1.f;
  if (abs(sumLeft - ev) > ABS_RANGE_LIMIT)
  {
    cout << 
      "sum left " << sumLeft + peaks.back().right.len <<
      " vs. " << samples.size() << endl;
    flag = false;
  }

  ev = samples.size() - peaks.front().left.len - 1.f;
  if (abs(sumRight - ev) > ABS_RANGE_LIMIT)
  {
    cout << 
      "sum right " << sumRight + peaks.front().left.len <<
      " vs. " << samples.size() << endl;
    flag = false;
  }

  return flag;
}


void PeakDetect::logList(
  const vector<float>& samples,
  const unsigned offsetSamples)
{
  len = samples.size();
  if (len < 2)
    THROW(ERR_SHORT_ACCEL_TRACE, "Accel trace length: " + to_string(len));

  offset = offsetSamples;
  peakList.clear();

  // We put a sentinel peak at the beginning.
  peakList.emplace_back(Peak());
  peakList.back().logSentinel(samples[0]);
  bool maxFlag = false;

  for (unsigned i = 1; i < len-1; i++)
  {
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

    const auto& peakPrev = peakList.back();
    const float areaFull = PeakDetect::integralList(samples, 
      peakPrev.getIndex(), i);
    const float areaCumPrev = peakPrev.getAreaCum();

    // The peak contains data for the interval preceding it.
    peakList.emplace_back(Peak());
    peakList.back().log(i, samples[i], areaCumPrev + areaFull, maxFlag);
  }

  // We put another peak at the end.
  const auto& peakPrev = peakList.back();
  const float areaFull = 
    PeakDetect::integralList(samples, peakPrev.getIndex(), len-1);
  const float areaCumPrev = peakPrev.getAreaCum();

  peakList.emplace_back(Peak());
  peakList.back().log(
    len-1, 
    samples[len-1], 
    areaCumPrev + areaFull, 
    ! peakPrev.getMaxFlag());

  PeakDetect::annotateList();

  PeakDetect::checkList(samples);
}


void PeakDetect::log(
  const vector<float>& samples,
  const unsigned offsetSamples)
{
  PeakDetect::logList(samples, offsetSamples);

  len = samples.size();
  offset = offsetSamples;

  peaks.clear();
  PeakData p;

  for (unsigned i = 1; i < len-1; i++)
  {
    if (samples[i] > samples[i-1])
    {
      // Use the last of equals as the starting point.
      while (i < len-1 && samples[i] == samples[i+1])
        i++;

      if (i < len-1 && samples[i] > samples[i+1])
        p.maxFlag = true;
      else
        continue;
    }
    else if (samples[i] < samples[i-1])
    {
      while (i < len-1 && samples[i] == samples[i+1])
        i++;

      if (i < len-1 && samples[i] < samples[i+1])
        p.maxFlag = false;
      else
        continue;
    }

    p.index = i;
    p.value = samples[i];
    p.activeFlag = true;

    const unsigned pi = (peaks.size() > 0 ? 
      peaks.back().index : 0);

    p.left.len = static_cast<float>(i - pi);
    p.left.range = abs(samples[i] - samples[pi]);
    p.left.area = 
      PeakDetect::integral(samples, pi, i, samples[i]);

    if (peaks.size() > 0)
      peaks.back().right = p.left;

    peaks.push_back(p);
  }

  if (peaks.size() > 0)
  {
    PeakData& lastPeak = peaks.back();
    const unsigned pi = lastPeak.index;
    lastPeak.right.len = static_cast<float>(len - 1 - pi);
    lastPeak.right.range = abs(samples[pi] - samples.back());
    lastPeak.right.area =
      PeakDetect::integral(samples, pi, len, samples[pi]);
  }

  PeakDetect::check(samples);
}


void PeakDetect::remakeFlanks(const vector<unsigned>& survivors)
{
  // Once some peaks have been eliminated, the left and right
  // flanks have to be recalculated.

  unsigned sno = survivors.size();

  FlankData left, right;
  left.reset();
  right.reset();
  unsigned np = 0;

bool flag = false;
  for (unsigned pno = peaks.size(); pno > 0; pno--)
  {
if (sno == 0)
  THROW(ERR_SURVIVORS, "survivor number is zero");

    PeakData& peak = peaks[pno-1];
// if (peak.index == 9623)
  // flag = true;

if (flag)
  cout << "\nNext survivor: " << survivors[sno-1] << 
    ", " << peaks[survivors[sno-1]].index << "\n";

    const bool maxFlag = peaks[survivors[sno-1]].maxFlag;

    if (pno-1 > survivors[sno-1])
    {
      if (peak.maxFlag == maxFlag)
      {
        left += peak.left;
        right += peak.right;

if (flag)
{
  cout << "Incrementing left by " << peak.left.range << 
    ", " << peak.left.area << "\n";
  cout << "Incrementing right by " << peak.right.range << 
    ", " << peak.right.area << "\n";
  cout << "Left now " << left.range <<
    ", " << left.area << "\n";
  cout << "Right now " << right.range <<
    ", " << right.area << "\n";
}
      }
      else
      {
        left -= peak.left;
        right -= peak.right;
if (flag)
{
  cout << "Decrementing left by " << peak.left.range << 
    ", " << peak.left.area << "\n";
  cout << "Decrementing right by " << peak.right.range << 
    ", " << peak.right.area << "\n";
  cout << "Left now " << left.range <<
    ", " << left.area << "\n";
  cout << "Right now " << right.range <<
    ", " << right.area << "\n";
}
      }
      np++;
      continue;
    }


    if (np > 0)
    {
if (flag)
{
  cout << "pno now " << pno << "\n";
}
      peak.right += right;

      if (pno+np < peaks.size())
      {
        peaks[pno+np].left -= left;
        peaks.erase(peaks.begin() + pno, 
          peaks.begin() + pno + np);
      }
      else
        peaks.erase(peaks.begin() + pno, peaks.end());

      left.reset();
      right.reset();
      np = 0;
    }

    sno--;
    if (sno == 0)
    {
      for (unsigned i = 0; i < pno-1; i++)
      {
        if (peaks[i].maxFlag == maxFlag)
          left += peaks[i].left;
        else
          left -= peaks[i].left;
      }
      peaks[pno-1].left += left;

      if (pno > 1)
        peaks.erase(peaks.begin(), peaks.begin() + pno - 1);
      break;
    }
  }
}


void PeakDetect::reduceToRuns()
{
  // A run is a sequence of peaks (from min to max or the
  // other way round).  The running range is on such a
  // sequence is the difference between the starting peak
  // and the peak we have gotten to.  In a run, the trace
  // only crosses the mid-point between the starting point
  // and the running range once (so there are multiple
  // checks per run).  Also, this property applies both to 
  // the run itself forwards and to the "backward" run.
  // Finally, a run does not contain two maxima (or minima)
  // that shorten the run range.

  vector<unsigned> survivors;
  survivors.push_back(0);
  for (unsigned i = 0; i < peaks.size()-1; i++)
  {
    if (i < survivors.back())
      continue;

    bool maxFlag = peaks[i].maxFlag;
    float level = peaks[i].value;
    float watermark = (maxFlag ? 
      numeric_limits<float>::max() :
      numeric_limits<float>::lowest());

    // Generate a list of forward candidates.
    list<unsigned> candidates;
    float runningRange = 0.f;
    for (unsigned j = i+1; j < peaks.size(); j++)
    {
      const float d = peaks[j].value - level;

      if ((maxFlag && d >= 0) || (! maxFlag && d <= 0))
        break;

      if (abs(d) < runningRange / 2.f)
        break;
      else if (peaks[j].maxFlag == maxFlag)
        continue;
      else if ((maxFlag && peaks[j].value >= watermark) ||
          (! maxFlag && peaks[j].value <= watermark))
        break;
      else
      {
        runningRange = abs(d);
        watermark = peaks[j].value;
        candidates.push_back(j);
      }
    }

    // Go backward from each candidate.  They are all of
    // opposity polarity to the starting peak.
    for (auto rit = candidates.rbegin(); 
        rit != candidates.rend(); rit++)
    {
      level = peaks[*rit].value;
      runningRange = 0.f;
      bool found = true;
      for (unsigned j = (*rit) - 1; j > i; j--)
      {
        const float d = peaks[j].value - level;

        if ((maxFlag && d <= 0) || (! maxFlag && d >= 0))
        {
          found = false;
          break;
        }

        if (abs(d) < runningRange / 2.f)
        {
          found = false;
          break;
        }
        else if (peaks[j].maxFlag != maxFlag)
          continue;
        else
          runningRange = abs(d);
      }
      
      if (found)
      {
        survivors.push_back(*rit);
        break;
      }
    }
  }

  PeakDetect::remakeFlanks(survivors);
}


void PeakDetect::estimateAreaRanges(
  float& veryLargeArea,
  float& normalLargeArea) const
{
  // The algorithm is not particularly sensitive to the 
  // parameters here.  It's just to get an idea of scale.

  const unsigned lp = peaks.size();
  vector<float> areas(lp+1);
  for (unsigned i = 0; i < lp; i++)
    areas[i] = peaks[i].left.area;
  areas[lp] = peaks.back().right.area;
  sort(areas.begin(), areas.end(), greater<float>());

  // If there is a very large area, it's probably at the
  // beginning of a trace and it's a residual of the transient.

  unsigned first = 0;
  while (first < lp && areas[first] > 2. * areas[first+1])
    first++;

  if (first > 0)
    veryLargeArea = (areas[first] + areas[first+1]) / 2.f;
  else
    veryLargeArea = 0.f;

  if (first > 2)
  {
    cout << "WARNING estimateRange: More than two large areas\n";
    for (unsigned i = 0; i < first; i++)
      cout << "i " << i << ": " << areas[i] << endl;
    cout << endl;
    normalLargeArea = 0.;
    return;
  }
  
  // Next come the large, but fairly normal areas.

  const float firstVal = areas[first];
  unsigned last = first;
  while (last < lp && areas[last] > 0.5 * areas[first])
    last++;

  normalLargeArea = areas[(first + last) / 2];
}


void PeakDetect::estimatePeakSize(float& negativePeakSize) const
{
  // The algorithm is not particularly sensitive to the 
  // parameters here.  It's just to get an idea of scale.

  const unsigned lp = peaks.size();
  vector<float> ampl(lp+1);
  for (unsigned i = 0; i < lp; i++)
  {
    if (! peaks[i].maxFlag && peaks[i].value < 0.f)
      ampl[i] = -peaks[i].value;
  }
  sort(ampl.begin(), ampl.end(), greater<float>());

  // If there is a very large peak, it's probably at the
  // beginning of a trace and it's a residual of the transient.

  unsigned first = 0;
  while (first < lp && 
      (ampl[first] > 2. * ampl[first+1] ||
       ampl[first] > 2. * ampl[first+2]))
    first++;

  if (first > 2)
  {
    cout << "WARNING estimatePeak: More than two large peaks\n";
    for (unsigned i = 0; i < first; i++)
      cout << "i " << i << ": " << ampl[i] << endl;
    cout << endl;
    return;
  }
  
  // Next come the large, but fairly normal areas.

  const float firstVal = ampl[first];
  unsigned last = first;
  while (last < lp && ampl[last] > 0.5 * ampl[first])
    last++;

  negativePeakSize = ampl[(first + last) / 2];
}


void PeakDetect::collapsePeaks(
  list<Peak>::iterator peak1,
  list<Peak>::iterator peak2)
{
  // Analogous to list.erase(), peak1 does not survive, while peak2 does.
  if (peak1 == peak2)
    return;

  // Need same polarity.
  if (peak1->getMaxFlag() != peak2->getMaxFlag())
  {
    cout << "ERROR C1" << endl;
  }

  Peak * peak0 = 
    (peak1 == peakList.begin() ? &*peak1 : &*prev(peak1));
  Peak * peakN = (next(peak2) == peakList.end() ? nullptr : &*next(peak2));
/*
cout << "peak2 before collapse\n";
cout << peak2->str(offset);
cout << "peak1 before collapse\n";
cout << peak1->str(offset);
cout << "peak0 before collapse\n";
cout << peak0->str(offset);
cout << "peakN before collapse\n";
cout << peakN->str(offset);
*/
  peak2->update(peak0, peakN);
/*
cout << "peak2 after collapse\n";
cout << peak2->str(offset) << "\n";
*/

  peakList.erase(peak1, peak2);
}


void PeakDetect::reduceSmallAreasList(const float areaLimit)
{
  if (peakList.empty())
    THROW(ERR_NO_PEAKS, "Peak list is empty");

  const auto peakLast = prev(peakList.end());
  auto peak = next(peakList.begin());

  while (peak != peakList.end())
  {
    auto peakPrev = prev(peak);
    const float area = peak->getArea();
    if (area >= areaLimit)
    {
      peak++;
      continue;
    }

    auto peakCurrent = peak, peakMax = peak;
    const bool maxFlag = peak->getMaxFlag();
    float sumArea, lastArea;
    float valueMax = numeric_limits<float>::lowest();

    do
    {
      peakPrev = peak;
      peak++;

      sumArea = peak->getArea(* peakCurrent);
      lastArea = peak->getArea();
// cout << "index " << peak->getIndex() << ": " << sumArea <<
  // ", " << lastArea << endl;
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
    while ((abs(sumArea) < areaLimit || abs(lastArea) < areaLimit) && 
        peak != peakLast);
// cout << "broke out: " << sumArea << ", " << lastArea << endl;

    if (abs(sumArea) < areaLimit || abs(lastArea) < areaLimit)
    {
      // It's the last set of peaks.  We could keep the largest peak
      // of the same polarity as peakCurrent (instead of peakCurrent).
      // It's a bit random whether or not this would be a "real" peak,
      // and we also don't keep track of this above.  So we we just stop.
      if (peakCurrent != peakLast)
        peakList.erase(++peakCurrent, peakList.end());
      break;
    }
    else if (peak->getMaxFlag() != maxFlag)
    {
// cout << "Different polarity\n";
      // Keep from peakCurrent to peak which is also often peakMax.
      PeakDetect::collapsePeaks(--peakCurrent, peak);
      peak++;
    }
    else
    {
// cout << "Same polarity\n";
      // Keep the start, the most extreme peak of opposite polarity,
      // and the end.
      PeakDetect::collapsePeaks(--peakCurrent, peakMax);
      PeakDetect::collapsePeaks(++peakMax, peak);
      peak++;
    }
  }
}


void PeakDetect::reduceSmallAreas(const float areaLimit)
{
  vector<unsigned> survivors;

  // Skip small starting peaks.
  unsigned start = 0;
  const unsigned lp = peaks.size();
  while (start < lp && peaks[start].left.area <= areaLimit)
    start++;

  // TODO This isn't perfect and may let some peaks through that turn
  // out to be small later on.  Probably the solution is only to
  // end on a maxFlag of the same orientation, i.e. making the while
  // loop step two at a time.  And keep track of runningArea.

  for (unsigned i = start; i < lp; i++)
  {
    if (peaks[i].right.area > areaLimit)
      survivors.push_back(i);
    else
    {
      const bool maxFlag = peaks[i].maxFlag;
      unsigned iextr = i;
      float vextr = peaks[iextr].value;

      while (i < lp && peaks[i].right.area <= areaLimit)
      {
        if (peaks[i].maxFlag == maxFlag)
        {
          if ((maxFlag && peaks[i].value > vextr) ||
              (! maxFlag && peaks[i].value < vextr))
          {
            iextr = i;
            vextr = peaks[i].value;
          }
        }
        i++;
      }

      if (i+1 < lp &&
          peaks[i].maxFlag != maxFlag &&
          ((maxFlag && peaks[i+1].value > vextr) ||
          (! maxFlag && peaks[i+1].value < vextr)))
      {
        // Let nature take its course.
      }
      else
      {
        survivors.push_back(iextr);

        if (i < lp && peaks[i].maxFlag != maxFlag)
          survivors.push_back(i);
      }
    }
  }

  PeakDetect::remakeFlanks(survivors);
}


void PeakDetect::reduceNegativeDips(const float peakLimit)
{
  vector<unsigned> survivors;

  const unsigned lp = peaks.size();

  for (unsigned i = 0; i < lp; i++)
  {
    const PeakData& peak = peaks[i];
   
    if (peak.maxFlag || peak.value < -peakLimit)
      survivors.push_back(i);
    else
    {
      // Collapse the peaks in pairs.
      i++;
    }
  }

  PeakDetect::remakeFlanks(survivors);
}


void PeakDetect::reduceTransientLeftovers()
{
  // Look for the first zero-crossing (upwards) following
  // the first peak.

  const unsigned lp = peaks.size();
  unsigned i = 0;
  while (i < lp && (peaks[i].maxFlag || peaks[i].value > 0.f))
    i++;
  while (i < lp && peaks[i].value < 0.f)
      i++;
  const unsigned firstCrossed = i;

  // Need enough of the other peaks for a histogram.

  if (lp-firstCrossed < 10)
    return;

  // Gather statistics on minimum peaks.

  PeakCluster peakCluster;
  for (i = firstCrossed; i < lp; i++)
  {
    if (! peaks[i].maxFlag)
      peakCluster += peaks[i];
  }

  // Check the early peaks against these templates.

  bool outlierFlag = false;
  unsigned lastOutlier = 0;

  for (i = 0; i < firstCrossed; i++)
  {
    if (! peaks[i].maxFlag && peakCluster.isOutlier(peaks[i]))
    {
      outlierFlag = true;
      lastOutlier = i;
    }
  }

  if (! outlierFlag)
    return;

  vector<unsigned> survivors;
  for (i = lastOutlier+1; i < lp; i++)
    survivors.push_back(i);

  PeakDetect::remakeFlanks(survivors);
}


void PeakDetect::reduce()
{
  PeakDetect::reduceToRuns();
// cout << "runs\n";
// PeakDetect::print();

  float veryLargeArea, normalLargeArea;
  PeakDetect::estimateAreaRanges(veryLargeArea, normalLargeArea);

  PeakDetect::reduceSmallAreas(normalLargeArea / SMALL_AREA_FACTOR);
// cout << "no small runs, " << normalLargeArea << ", " << 
  // normalLargeArea / SMALL_AREA_FACTOR << "\n";
// PeakDetect::print();

  // Currently not doing anything about very large runs.

  float negativePeakSize;
  PeakDetect::estimatePeakSize(negativePeakSize);
  
  PeakDetect::reduceNegativeDips(0.5f * negativePeakSize);
// cout << "no dips, " << negativePeakSize << "\n";
// PeakDetect::print();

  PeakDetect::reduceTransientLeftovers();
// cout << "no transient\n";
// PeakDetect::print();
}


void PeakDetect::eliminateSmallAreas()
{
  vector<unsigned> survivors;

  const unsigned lp = peaks.size();
  for (unsigned i = 0; i+1 < lp; i++)
  {
    // 0 .. lp
    // Start always from a tiny peak
    // Flag if can be subsumed by predecessor (may not exist)
    // Flag if successor (ditto)
    // If both exist:
    //   Aren't we always the (local) extremum?

    if (peaks[i].left.area <= 0.1 && 
        peaks[i].right.area <= 0.1 &&
        peaks[i+1].left.range <= peaks[i+1].right.range)
    {
      // This peak is tiny and can be subsumed by next one.
      //
      i++;
    }
    else if ( peaks[i].left.range >= peaks[i].right.range &&
        peaks[i+1].left.area <= 0.1 && 
        peaks[i+1].right.area <= 0.1)
    {
      // The next peak is tiny and can be subsumed by this one.
      i++;
    }
    else
      survivors.push_back(i);
  }

  PeakDetect::remakeFlanks(survivors);
}


void PeakDetect::eliminateKinksList()
{
  if (peakList.empty())
    THROW(ERR_NO_PEAKS, "Peak list is empty");

  for (auto peak = next(peakList.begin(), 2); 
      peak != prev(peakList.end()); peak++)
  {
    const auto peakPrev = prev(peak);
    const auto peakPrevPrev = prev(peakPrev);
    const auto peakNext = next(peak);

    const float ratioPrev = 
      peakPrev->getArea(* peakPrevPrev) / peak->getArea(* peakPrev);
    const float ratioNext = 
      peakNext->getArea(* peak) / peak->getArea(* peakPrev);

    if (ratioPrev > 1.f && 
        ratioNext > 1.f &&
        ratioPrev * ratioNext > 100.f)
    {
      PeakDetect::collapsePeaks(peakPrev, peakNext);
    }
  }
}


void PeakDetect::eliminateKinks()
{
  vector<unsigned> survivors;

  const unsigned lp = peaks.size();
  for (unsigned i = 0; i+1 < lp; i++)
  {
    const double l2r = peaks[i].left.area / peaks[i].right.area;
    const double nextl2r = peaks[i+1].left.area / peaks[i+1].right.area;

    if ((l2r >= 100. && nextl2r <= 1.) ||
        (l2r >= 10. && nextl2r <= 0.1))
    {
      // Strike self and next peak.
      i++;
    }
    else if ((nextl2r <= 0.01 && l2r >= 1.) ||
        (nextl2r <= 0.1 && l2r >= 10.))
    {
      // Strike self and next peak.
      i++;
    }
    else
      survivors.push_back(i);
  }

  PeakDetect::remakeFlanks(survivors);
}


void PeakDetect::eliminatePositiveMinima()
{
  vector<unsigned> survivors;

  const unsigned lp = peaks.size();

  // Strike a starting positive minimum.
  unsigned start = 0;
  if (peaks[start].value >= 0. && ! peaks[start].maxFlag)
    start++;

  for (unsigned i = start; i < lp-1; i++)
  {
    if (peaks[i+1].value < 0. || peaks[i+1].maxFlag)
    {
      // The next peak is not a positive mininum.
      survivors.push_back(i);

      if (i+1 == lp-1)
        survivors.push_back(i+1);
    }
    else
    {
      unsigned imax = i;
      float vmax = peaks[imax].value;

      while (i+2 < lp && peaks[i+1].value >= 0.)
      {
        if (vmax < peaks[i+2].value)
        {
          imax = i;
          vmax = peaks[i+2].value;
        }
        i += 2;
      }

      // Keep the largest maximum.
      survivors.push_back(imax);
    }
  }

  PeakDetect::remakeFlanks(survivors);
}


float PeakDetect::estimateScale(vector<float>& data) const
{
  // The algorithm is not particularly sensitive to the 
  // parameters here.  It's just to get an idea of scale.

  const unsigned ld = data.size();
  sort(data.begin(), data.end(), greater<float>());

  // If there are large values, they are often at the beginning
  // of a trace as a residual of the transient.

  unsigned first = 0;
  while (first < ld && 
      (data[first] > 2. * data[first+1] ||
       data[first] > 2. * data[first+2]))
    first++;

  if (first > 2)
  {
    cout << "WARNING estimateScale: More than two large peaks\n";
    for (unsigned i = 0; i < first; i++)
      cout << "i " << i << ": " << data[i] << endl;
    cout << endl;
  }
  
  // We take a number of the rest.

  const float firstVal = data[first];
  unsigned last = first;
  while (last < ld && data[last] > 0.5 * data[first])
    last++;

  return abs(data[(first + last) / 2]);
}


void PeakDetect::estimateScales()
{
  vector<float> valueV,
    leftlenV, leftrangeV, leftareaV,
    rightlenV, rightrangeV, rightareaV;

  for (const auto& peak: peaks)
  {
    if (peak.maxFlag)
      continue;

    valueV.push_back(abs(peak.value));
    leftlenV.push_back(peak.left.len);
    leftrangeV.push_back(peak.left.range);
    leftareaV.push_back(peak.left.area);
    rightlenV.push_back(peak.right.len);
    rightrangeV.push_back(peak.right.range);
    rightareaV.push_back(peak.right.area);
  }

  scales.index = 0;
  scales.maxFlag = false;

  scales.value = PeakDetect::estimateScale(valueV);
  scales.left.len = PeakDetect::estimateScale(leftlenV);
  scales.left.range = PeakDetect::estimateScale(leftrangeV);
  scales.left.area = PeakDetect::estimateScale(leftareaV);
  scales.right.len = PeakDetect::estimateScale(rightlenV);
  scales.right.range = PeakDetect::estimateScale(rightrangeV);
  scales.right.area = PeakDetect::estimateScale(rightareaV);
}


void PeakDetect::estimateScalesList()
{
  vector<float> valueV, lenV, rangeV, areaV;

  const Peak peakSentinel;

  for (auto peak = next(peakList.begin());
      peak != peakList.end(); peak++)
  {
    valueV.push_back(abs(peak->getValue()));
    lenV.push_back(peak->getLength());
    rangeV.push_back(peak->getRange());
    areaV.push_back(peak->getArea(* prev(peak)));
  }

  scalesList.log(
    static_cast<unsigned>(PeakDetect::estimateScale(lenV)),
    PeakDetect::estimateScale(valueV),
    PeakDetect::estimateScale(areaV),
    false);
}


void PeakDetect::normalizePeaks(vector<PeakData>& normalPeaks)
{
  normalPeaks.clear();

  for (const auto& peak: peaks)
  {
    if (peak.maxFlag)
      continue;

    PeakData npeak;
    npeak.index = peak.index;
    npeak.maxFlag = peak.maxFlag;
    npeak.value = peak.value / scales.value;
    npeak.left.len = peak.left.len / scales.left.len;
    npeak.left.range = peak.left.range / scales.left.range;
    npeak.left.area = peak.left.area / scales.left.area;
    npeak.right.len = peak.right.len / scales.right.len;
    npeak.right.range = peak.right.range / scales.right.range;
    npeak.right.area = peak.right.area / scales.right.area;
    normalPeaks.push_back(npeak);
  }
}


#define SQUARE(a) ((a) * (a))

float PeakDetect::metric(
  const PeakData& np1,
  const PeakData& np2) const
{
  return SQUARE(np1.value - np2.value) +
    4.f * SQUARE(np1.left.len - np2.left.len) +
    SQUARE(np1.left.range - np2.left.range) +
    SQUARE(np1.left.area - np2.left.area) +
    4.f * SQUARE(np1.right.len - np2.right.len) +
    SQUARE(np1.right.range - np2.right.range) +
    SQUARE(np1.right.area - np2.right.area);
}


void PeakDetect::runKmeansOnce(
  const Koptions& koptions,
  const vector<PeakData>& normalPeaks,
  vector<PeakData>& clusters,
  vector<unsigned>& assigns,
  float& distance) const
{
  const unsigned lp = normalPeaks.size();

  static random_device seed;
  static mt19937 rng(seed());
  uniform_int_distribution<unsigned> indices(0, lp-1);

  // Pick random centroids from the data points.
  clusters.resize(koptions.numClusters);
  for (auto& cluster: clusters)
    cluster = normalPeaks[indices(rng)];

  cout << "Normalpeaks" << endl;
  for (unsigned i = 0; i < normalPeaks.size(); i++)
    PeakDetect::printPeak(normalPeaks[i], i);
  cout << endl;
  
  cout << "Means" << endl;
  for (unsigned i = 0; i < clusters.size(); i++)
    PeakDetect::printPeak(clusters[i], i);
  cout << endl;
  
  clusters.resize(koptions.numClusters);
  assigns.resize(lp);
  distance = numeric_limits<float>::max();

  for (unsigned iter = 0; iter < koptions.numIterations; iter++)
  {
    vector<PeakData> clustersNew(koptions.numClusters);
    vector<unsigned> counts(koptions.numClusters, 0);
    float distGlobal = 0.f;

    for (unsigned pno = 0; pno < lp; pno++)
    {
      float distBest = numeric_limits<float>::max();
      unsigned bestCluster = 0;
      for (unsigned cno = 0; cno < koptions.numClusters; cno++)
      {
        const float dist = PeakDetect::metric(normalPeaks[pno], clusters[cno]);
        if (dist < distBest)
        {
          distBest = dist;
          bestCluster = cno;
        }
      }

      assigns[pno] = bestCluster;
      clustersNew[bestCluster] += normalPeaks[pno];
      counts[bestCluster]++;
      distGlobal += distBest;
    }

    for (unsigned cno = 0; cno < koptions.numClusters; cno++)
    {
      clusters[cno] = clustersNew[cno];
      clusters[cno] /= counts[cno];
    }

    if (distGlobal / distance >= koptions.convCriterion)
    {
      cout << "Finishing after " << iter << " iterations" << endl;
      break;
    }
    else if (distGlobal <= distance)
      distance = distGlobal;

    cout << "Iteration " << iter << ": dist " << distGlobal << 
      ", distGlobal " << distGlobal << endl;
  }
}


void PeakDetect::runKmeansOnceList(
  const Koptions& koptions,
  vector<Peak>& clusters,
  float& distance)
{
  static random_device seed;
  static mt19937 rng(seed());
  uniform_int_distribution<unsigned> indices(0, peakList.size()-1);

  // Pick random centroids from the data points.
  clusters.resize(koptions.numClusters);
  const auto peakFirst = peakList.begin();
  for (auto& cluster: clusters)
    cluster = * next(peakFirst, indices(rng));

  clusters.resize(koptions.numClusters);
  distance = numeric_limits<float>::max();

  for (unsigned iter = 0; iter < koptions.numIterations; iter++)
  {
    vector<Peak> clustersNew(koptions.numClusters);
    vector<unsigned> counts(koptions.numClusters, 0);
    float distGlobal = 0.f;

    for (auto& peak: peakList)
    {
      float distBest = numeric_limits<float>::max();
      unsigned bestCluster = 0;
      for (unsigned cno = 0; cno < koptions.numClusters; cno++)
      {
        const float dist = peak.distance(clusters[cno], scalesList);
        if (dist < distBest)
        {
          distBest = dist;
          bestCluster = cno;
        }
      }

      peak.logCluster(bestCluster);
      clustersNew[bestCluster] += peak;
      counts[bestCluster]++;
      distGlobal += distBest;
    }

    for (unsigned cno = 0; cno < koptions.numClusters; cno++)
    {
      clusters[cno] = clustersNew[cno];
      clusters[cno] /= counts[cno];
    }

    if (distGlobal / distance >= koptions.convCriterion)
    {
      cout << "Finishing after " << iter << " iterations" << endl;
      break;
    }
    else if (distGlobal <= distance)
      distance = distGlobal;

    cout << "Iteration " << iter << ": dist " << distGlobal << 
      ", distGlobal " << distGlobal << endl;
  }
}


void PeakDetect::reduceNew()
{
// cout << "Raw peaks: " << peaks.size() << "\n";
// PeakDetect::print();

cout << "List peaks: " << peakList.size() << "\n";
PeakDetect::printList();

  // PeakDetect::eliminateTinyAreas();
  // TODO Maybe also something derived from the signal.
  // TODO Doubly linked list for peak structure.
  PeakDetect::reduceSmallAreas(0.1f);

// cout << "Non-tiny peaks: " << peaks.size() << "\n";
// PeakDetect::print();

  PeakDetect::reduceSmallAreasList(0.1f);
cout << "Non-tiny list peaks: " << peakList.size() << "\n";
PeakDetect::printList();

  PeakDetect::eliminateKinks();
  PeakDetect::eliminateKinksList();

// cout << "Non-kinky peaks: " << peaks.size() << "\n";
// PeakDetect::print();

// cout << "Non-kinky list peaks: " << peakList.size() << "\n";
// PeakDetect::printList();

  PeakDetect::eliminatePositiveMinima();
// cout << "Negative peaks: " << peaks.size() << "\n";
// PeakDetect::print();

  PeakDetect::estimateScales();
cout << "Scale\n";
PeakDetect::printPeak(scales, 0);
cout << endl;

  PeakDetect::estimateScalesList();
cout << "Scale list\n";
cout << scalesList.str(0) << "\n";

  const float scaledCutoff = 
    0.05f * ((scales.left.area + scales.right.area) / 2.f);

cout << "Area cutoff " << scaledCutoff << endl;

  /* */
  PeakDetect::reduceSmallAreas(scaledCutoff);
// cout << "Reasonable peaks: " << peaks.size() << "\n";
// PeakDetect::print();
/* */

  const float scaledCutoffList = 0.05f * scalesList.getAreaCum();
cout << "Area list cutoff " << scaledCutoff << endl;

  PeakDetect::reduceSmallAreasList(scaledCutoffList);
// cout << "Reasonable list peaks: " << peakList.size() << "\n";
// PeakDetect::printList();


  /* */
  vector<PeakData> normalPeaks;
  PeakDetect::normalizePeaks(normalPeaks);

  Koptions koptions;
  koptions.numClusters = 8;
  koptions.numIterations = 20;
  koptions.convCriterion = 0.999f;

  vector<PeakData> clusters;
  vector<unsigned> assigns;
  float distance;

  PeakDetect::runKmeansOnce(koptions, normalPeaks, clusters, assigns,
    distance);

  cout << "clusters" << endl;
  PeakDetect::printHeader();
  for (unsigned i = 0; i < clusters.size(); i++)
    PeakDetect::printPeak(clusters[i], i);
  cout << endl;
  /* */

  for (unsigned i = 0; i < koptions.numClusters; i++)
  {
    cout << i << ":";
    for (unsigned pno = 0; pno < normalPeaks.size(); pno++)
    {
      if (assigns[pno] == i)
        cout << " " << pno;
    }
    cout << "\n";
  }
  cout << "\n";

  cout << "Cluster scores (small means skip)\n";
  vector<bool> skips(koptions.numClusters);
  for (unsigned cno = 0; cno < koptions.numClusters; cno++)
  {
    const auto& c = clusters[cno];

    // Is small when the cluster is tiny.
    float tinyScore = abs(c.value) + 
      c.left.range + c.right.range +
      c.left.area + c.right.area;

    // Is (relatively) lower when the cluster looks like a dip.
    float gradientScore = (c.left.range + c.right.range) /
      (c.left.len + c.right.len);

    cout << "i " << cno << ": " <<
      fixed << setprecision(2) << tinyScore << 
      ", " << gradientScore;

    if (tinyScore < 1.5f || gradientScore < 0.6f)
    {
      skips[cno] = true;
      cout << " SKIP";
    }
    else
      skips[cno] = false;

    cout << endl;
  }

  vector<PeakData> newPeaks;
  unsigned runningPeakNo = 0;
  for (unsigned i = 0; i < assigns.size(); i++)
  {
    if (skips[assigns[i]])
      continue;

    const unsigned index = normalPeaks[i].index;
    while (runningPeakNo < peaks.size() &&
        peaks[runningPeakNo].index < index)
      runningPeakNo++;

    if (runningPeakNo == peaks.size())
    {
      cout << "ERROR1" << endl;
      exit(0);
    }

    if (peaks[runningPeakNo].index != index)
    {
      cout << "ERROR2" << endl;
      exit(0);
    }

    newPeaks.push_back(peaks[runningPeakNo]);
  }

peaks = newPeaks;
// cout << "Final peaks: " << peaks.size() << "\n";
// PeakDetect::print();
}


void PeakDetect::makeSynthPeaks(vector<float>& synthPeaks) const
{
  for (unsigned i = 0; i < synthPeaks.size(); i++)
    synthPeaks[i] = 0;

  unsigned count = 0;
  for (auto& peak: peaks)
  {
    if (! peak.maxFlag)
    {
      synthPeaks[peak.index] = peak.value;
      count++;
    }
  }
}


void PeakDetect::makeSynthPeaksList(vector<float>& synthPeaks) const
{
  for (unsigned i = 0; i < synthPeaks.size(); i++)
    synthPeaks[i] = 0;

  // unsigned count = 0;
  for (auto& peak: peakList)
  {
    // if (! peak.maxFlag)
    // {
      synthPeaks[peak.getIndex()] = peak.getValue();
      // count++;
    // }
  }
}


void PeakDetect::getPeakTimes(vector<PeakTime>& times) const
{
  times.clear();
  const unsigned firstMin = (peaks[0].maxFlag ? 1 : 0);
  const float t0 = peaks[firstMin].index / 
    static_cast<float>(SAMPLE_RATE);

  for (unsigned i = 0; i < peaks.size(); i++)
  {
    const PeakData& peak = peaks[i];
    if (peak.maxFlag)
      continue;

    PeakTime p;
    p.time = peaks[i].index / SAMPLE_RATE - t0;
    p.value = peaks[i].value;
    times.push_back(p);
  }
}


void PeakDetect::getPeakTimesList(vector<PeakTime>& times) const
{
  times.clear();
  const auto pfirst = peakList.begin();
  unsigned findex;
  if (pfirst->getMaxFlag())
    findex = pfirst->getIndex();
  else
    findex = next(pfirst)->getIndex();

  const float t0 = findex / static_cast<float>(SAMPLE_RATE);

  for (const auto& peak: peakList)
  {
    if (peak.getMaxFlag())
      continue;

    times.emplace_back(PeakTime());
    PeakTime& p = times.back();
    p.time = peak.getIndex() / SAMPLE_RATE - t0;
    p.value = peak.getValue();
  }
}


void PeakDetect::printHeader() const
{
  cout << 
    setw(6) << "Index" <<
    setw(5) << "Type" <<
    setw(9) << "Value" <<
    setw(7) << "Len" <<
    setw(7) << "Range" << 
    setw(8) << "Area" <<
    setw(8) << "Grad" <<
    setw(8) << "Fill" <<
    setw(8) << "Cluster" <<
    "\n";
}



void PeakDetect::printPeak(
  const PeakData& peak,
  const unsigned index) const
{
  UNUSED(index);
  cout << 
    setw(6) << right << peak.index + offset <<
    setw(5) << (peak.maxFlag ? "max" : "min") <<
    setw(9) << fixed << setprecision(2) << peak.value <<
    setw(7) << fixed << setprecision(2) << peak.left.len <<
    setw(7) << fixed << setprecision(2) << peak.left.range <<
    setw(8) << fixed << setprecision(2) << peak.left.area <<
    setw(8) << fixed << setprecision(2) << 
      (peak.left.len == 0.f ? 0.f : peak.left.range / peak.left.len) <<
    setw(8) << fixed << setprecision(2) << 
      (peak.left.len == 0.f ? 0.f :
      peak.left.area / (0.5 * peak.left.len * peak.left.range)) <<
    setw(8) << right << "0" <<
    "\n";
}


void PeakDetect::print(const bool activeFlag) const
{
  PeakDetect::printHeader();

  unsigned i = 0;
  for (const auto& peak: peaks)
  {
    if (! activeFlag || peak.activeFlag)
      PeakDetect::printPeak(peak, i);

    i++;
  }
  cout << "\n";
}


void PeakDetect::printList() const
{
  cout << peakList.front().strHeader();

  for (auto peak = next(peakList.begin()); peak != peakList.end(); peak++)
    cout << peak->str(offset);
  cout << "\n";
}


void PeakDetect::printList(
  const vector<unsigned>& indices,
  const bool activeFlag) const
{
  PeakDetect::printHeader();
  for (unsigned i: indices)
  {
    const PeakData& peak = peaks[i];
    if (! activeFlag || peak.activeFlag)
      PeakDetect::printPeak(peak, i);

    i++;
  }
  cout << "\n";
}

