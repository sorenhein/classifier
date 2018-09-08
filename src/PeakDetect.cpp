#include <list>
#include <iostream>
#include <iomanip>
#include <functional>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakDetect.h"
#include "PeakCluster.h"
#include "Except.h"

#define SMALL_AREA_FACTOR 100.f

// Only a check limit, not algorithmic parameters.
#define ABS_RANGE_LIMIT 1.e-4
#define ABS_AREA_LIMIT 1.e-4

#define SAMPLE_RATE 2000.


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


bool PeakDetect::check(const vector<float>& samples) const
{
  bool flag = true;
  const unsigned lp = peaks.size();
  if (lp == 0)
    return true;

  unsigned sumLeft = 0, sumRight = 0;
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

  if (sumLeft + peaks.back().right.len + 1 != samples.size())
  {
    cout << 
      "sum left " << sumLeft + peaks.back().right.len <<
      " vs. " << samples.size() << endl;
    flag = false;
  }

  if (sumRight + peaks.front().left.len + 1 != samples.size())
  {
    cout << 
      "sum right " << sumRight + peaks.front().left.len <<
      " vs. " << samples.size() << endl;
    flag = false;
  }

  return flag;
}


void PeakDetect::log(
  const vector<float>& samples,
  const unsigned offsetSamples)
{
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

    p.left.len = i - pi;
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
    lastPeak.right.len = len - 1 - pi;
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
if (peak.index == 9623)
  flag = true;

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


void PeakDetect::reduceSmallRuns(const float areaLimit)
{
  vector<unsigned> survivors;

  // Skip small starting peaks.
  unsigned start = 0;
  const unsigned lp = peaks.size();
  while (start < lp && peaks[start].left.area <= areaLimit)
    start++;

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
        if (peaks[i].maxFlag == maxFlag &&
           ((maxFlag && peaks[i].value > vextr) ||
            (! maxFlag && peaks[i].value < vextr)))
        {
          iextr = i;
          vextr = peaks[i].value;
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
        survivors.push_back(iextr);
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

  PeakDetect::reduceSmallRuns(normalLargeArea / SMALL_AREA_FACTOR);
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


void PeakDetect::eliminateTinyAreas()
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


void PeakDetect::eliminateKinks()
{
  vector<unsigned> survivors;

  const unsigned lp = peaks.size();
  for (unsigned i = 0; i+1 < lp; i++)
  {
    const double l2r = peaks[i].left.area / peaks[i].right.area;
    const double nextl2r = peaks[i+1].left.area / peaks[i+1].right.area;

    if (l2r >= 100. && nextl2r <= 1.)
    {
      // Strike self and next peak.
      i++;
    }
    else if (nextl2r <= 0.01 && l2r >= 1.)
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


void PeakDetect::reduceNew()
{
cout << "Raw peaks: " << peaks.size() << "\n";
PeakDetect::print();

  // PeakDetect::eliminateTinyAreas();
  // TODO Maybe also something derived from the signal.
  // Call it SmallPeaks, not SmallRuns
  // TODO Doubly linked list for peak structure.
  PeakDetect::reduceSmallRuns(0.1f);

cout << "Non-tiny peaks: " << peaks.size() << "\n";
PeakDetect::print();

  PeakDetect::eliminateKinks();

cout << "Non-kinky peaks: " << peaks.size() << "\n";
PeakDetect::print();

  PeakDetect::eliminatePositiveMinima();
cout << "Negative peaks: " << peaks.size() << "\n";
PeakDetect::print();

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


void PeakDetect::printHeader() const
{
  cout << 
    setw(5) << "No." <<
    setw(6) << "Index" <<
    setw(9) << "Value" <<
    setw(5) << "Type" <<
    setw(5) << "Act" <<
    setw(5) << "Llen" <<
    setw(5) << "Rlen" <<
    setw(7) << "Lrange" <<
    setw(7) << "Rrange" << 
    setw(8) << "Larea" <<
    setw(8) << "Rarea" << 
    "\n";
}


void PeakDetect::printPeak(
  const PeakData& peak,
  const unsigned index) const
{
  cout << 
    setw(5) << right << index <<
    setw(6) << right << peak.index + offset <<
    setw(9) << fixed << setprecision(2) << peak.value <<
    setw(5) << (peak.maxFlag ? "max" : "min") <<
    setw(5) << (peak.activeFlag ? "yes" : "-") <<
    setw(5) << peak.left.len <<
    setw(5) << peak.right.len <<
    setw(7) << fixed << setprecision(2) << peak.left.range <<
    setw(7) << fixed << setprecision(2) << peak.right.range << 
    setw(8) << fixed << setprecision(2) << peak.left.area <<
    setw(8) << fixed << setprecision(2) << peak.right.area << 
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

