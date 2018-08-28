#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakDetect.h"

#define KEEP_AREA_RATIO_LOWER 2.0f
#define KEEP_AREA_RATIO_UPPER 20.0f

#define ABS_RANGE_LIMIT 1.e-4
#define ABS_AREA_LIMIT 1.e-4


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


void PeakDetect::makeSorted()
{
  SortData sd;
  for (unsigned i = 0; i < peaks.size(); i++)
  {
    sd.pindex = i;
    sd.valLeft = peaks[i].left.area;
    sd.valRight = peaks[i].right.area;
    sortedPeaks.push_back(sd);
  }

  sort(sortedPeaks.begin(), sortedPeaks.end());

  for (unsigned i = 0; i < sortedPeaks.size(); i++)
    toSorted[sortedPeaks[i].pindex] = i;
}


void PeakDetect::log(
  const vector<float>& samples,
  const unsigned offsetSamples)
{
  len = samples.size();
  offset = offsetSamples;
  sampleFirst = samples[0];
  sampleLast = samples.back();

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

  // PeakDetect::makeSorted();
  PeakDetect::check(samples);
  PeakDetect::print(false);
}


void PeakDetect::reduceData(const vector<unsigned>& survivors)
{
  unsigned sno = survivors.size();
  FlankData left, right;
  left.reset();
  right.reset();
  unsigned np = 0;

  for (unsigned pno = peaks.size(); pno > 0; pno--)
  {
    PeakData& peak = peaks[pno-1];
    const bool maxFlag = peaks[survivors[sno-1]].maxFlag;

    if (pno-1 > survivors[sno-1])
    {
      if (peak.maxFlag == maxFlag)
      {
        left += peak.left;
        right += peak.right;
      }
      else
      {
        left -= peak.left;
        right -= peak.right;
      }
      np++;
      continue;
    }


    if (np > 0)
    {
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


void PeakDetect::reduceUnleveled()
{
  // TODO
  // Go from the beginning in sortedPeaks
  // Want a left ratio and a right ratio in bounds
  // Poke out this element
  // - Mark as inactive in peaks
  // - Maybe mark a neighbor in peaks as well
  // - Remove these 1-2 from sortedPeaks
  // - The toSorted numbers shift (down)
  // - 1-2 peaks are modified in peaks
  // - These also come out of sortedPeaks
  // - This too changes toSorted
  // - They go back in at new places
  // - This too changes toSorted
  // Validate that all three vectors are still consistent.
  // FlankData should also have a ratio

  // OR: Effectively look for long runs

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

cout << "Have " << peaks.size() << " peaks, " <<
  survivors.size() << " survivors\n";
  PeakDetect::reduceData(survivors);

  PeakDetect::print(false);
}


void PeakDetect::getLevel(vector<float>& level) const
{
  const unsigned lp = peaks.size();
  if (lp == 0)
    return;

  PeakData const * prevPeak = nullptr;
  for (const PeakData& peak: peaks)
  {
    // We levelize such that the minima are all zero.
    // This requires good minima ahead of time.
    if (peak.maxFlag)
      continue;

    unsigned prevIndex;
    float prevValue;
    if (prevPeak == nullptr)
    {
      prevIndex = 0;
      prevValue = peak.value;
    }
    else
    {
      prevIndex = prevPeak->index;
      prevValue = prevPeak->value;
    }

    const float step =
      (peak.value - prevValue) / (peak.index - prevIndex);


    for (unsigned i = prevIndex; i < peak.index; i++)
      level[i] = prevValue + step * (i - prevIndex);

    prevPeak = &peak;
  }

  // Do the last bit.  Just repeat the last value.
  const PeakData& lastPeak = peaks.back();
  for (unsigned i = lastPeak.index; i < len; i++)
    level[i] = lastPeak.value;
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
  cout << "COUNT " << count << endl;
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

