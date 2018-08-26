#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <math.h>

#include "SegActive.h"

#define G_FORCE 9.8f

#define SAMPLE_RATE 2000.

#define SEPARATOR ";"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

SegActive::SegActive()
{
}


SegActive::~SegActive()
{
}


void SegActive::reset()
{
}


void SegActive::integrate(
  const vector<double>& samples,
  const Interval& aint,
  const double mean)
{
  UNUSED(mean);
  /*
  double sum = 0.;
  for (unsigned i = aint.first; i < aint.first + aint.len; i++)
    sum += samples[i];
  double avg = sum / aint.len;
  */
  double avg = 0.;

  // synthSpeed is then in 0.01 m/s.
  synthSpeed[aint.first - writeInterval.first] = 100.f * G_FORCE * 
    static_cast<float>((samples[aint.first] - avg) / SAMPLE_RATE);

  for (unsigned i = aint.first+1; i < aint.first + aint.len; i++)
    synthSpeed[i - writeInterval.first] = 
      synthSpeed[i - writeInterval.first - 1] + 
      100.f * G_FORCE * static_cast<float>((samples[i] - avg) / SAMPLE_RATE);
}


void SegActive::compensateMedian()
{
  // The acceleration noise generates a random walk in the speed.
  // We attempt to correct for this with a rough median filter.

  const unsigned filterWidth = 1001;
  const unsigned filterMid = (filterWidth-1) >> 1;

  const unsigned ls = synthSpeed.size();
  if (ls <= filterWidth)
  {
    cout << "CAN'T COMPENSATE\n";
    return;
  }

  vector<float> newSpeed(ls);

  Mediator * mediator = MediatorNew(filterWidth);

  for (unsigned i = 0; i < filterWidth; i++)
    MediatorInsert(mediator, synthSpeed[i]);

  MediatorStats ms;
  for (unsigned i = filterMid; i < ls - filterMid; i++)
  {
    GetMediatorStats(mediator, &ms);
    newSpeed[i] = ms.median;
    if (i+1 < ls - filterMid)
      MediatorInsert(mediator, synthSpeed[i+filterMid+1]);
  }

  free(mediator);

  // Compensate down to zero.
  for (unsigned i = 0; i < filterMid; i++)
    newSpeed[i] = newSpeed[filterMid];

  const float step1 = 
    (synthSpeed[ls-1] - newSpeed[ls-filterMid-1]) / filterMid;

  for (unsigned i = ls - filterMid; i < ls; i++)
    newSpeed[i] = synthSpeed[ls-1] - step1 * (ls-1-i);
  
  for (unsigned i = 0; i < ls; i++)
    synthSpeed[i] -= newSpeed[i];
}


void SegActive::compensateSpeed()
{
  // The acceleration noise generates a random walk in the speed.
  // We attempt to correct for this with a rough lowpass filter.

  const unsigned filterWidth = 1001;
  const unsigned filterMid = (filterWidth-1) >> 1;

  const unsigned ls = synthSpeed.size();
  if (ls <= filterWidth)
  {
    cout << "CAN'T COMPENSATE\n";
    return;
  }

  vector<float> newSpeed(ls);

  float runningSum = 0.f;
  for (unsigned i = 0; i < filterWidth; i++)
    runningSum += synthSpeed[i];

  for (unsigned i = filterMid; i < ls - filterMid; i++)
  {
    newSpeed[i] = runningSum / filterWidth;
    if (i+1 < ls - filterMid)
      runningSum += synthSpeed[i + filterMid + 1] -
        synthSpeed[i - filterMid];
  }

  const float step0 = (newSpeed[filterMid] - synthSpeed[0]) /
    filterMid;

  // Compensate down to zero.
  // for (unsigned i = 0; i < filterMid; i++)
    // newSpeed[i] = synthSpeed[0] + step0 * i;
  for (unsigned i = 0; i < filterMid; i++)
    newSpeed[i] = newSpeed[filterMid];

  const float step1 = 
    (synthSpeed[ls-1] - newSpeed[ls-filterMid-1]) / filterMid;

  for (unsigned i = ls - filterMid; i < ls; i++)
    newSpeed[i] = synthSpeed[ls-1] - step1 * (ls-1-i);
  
  for (unsigned i = 0; i < ls; i++)
    synthSpeed[i] -= newSpeed[i];
}


void SegActive::integrateFloat(const Interval& aint)
{
  /*
  float sum = 0.;
  for (unsigned i = aint.first; i < aint.first + aint.len; i++)
    sum += synthSpeed[i - writeInterval.first];
  float avg = sum / aint.len;
  */
  float avg = 0.f;

  // synthPos is then in tenth of a mm.
  synthPos[aint.first - writeInterval.first] = 
    100.f * (synthSpeed[aint.first - writeInterval.first] - avg) / 
    static_cast<float>(SAMPLE_RATE);

  for (unsigned i = aint.first + 1 - writeInterval.first; 
    i < aint.first + aint.len - writeInterval.first; i++)
    synthPos[i] = synthPos[i-1] + 
      100.f * (synthSpeed[i] - avg) / static_cast<float>(SAMPLE_RATE);
}

#include <deque>

void SegActive::getPosStatsNew(const bool maxFlag)
{
  // https://www.nayuki.io/res/
  // sliding-window-minimum-maximum-algorithm/SlidingWindowMinMax.hpp

  const unsigned filterWidth = 1001;
  const unsigned filterMid = (filterWidth-1) >> 1;

  const unsigned ls = synthSpeed.size();
  if (ls <= filterWidth)
  {
    cout << "CAN'T COMPENSATE\n";
    return;
  }

  deque<float> deque;
  unsigned i = 0;
  auto tail = synthPos.cbegin();
  for (const float& val: synthPos)
  {
    while (! deque.empty() && 
      ((maxFlag && val > deque.back()) || 
      (! maxFlag && val < deque.back())))
      deque.pop_back();

    deque.push_back(val);

    if (maxFlag)
      posStats[i].max = deque.front();
    else
      posStats[i].min = deque.front();
    i++;
    
    if (i >= filterWidth)
    {
      if (* tail == deque.front())
        deque.pop_front();
      tail++;
    }
  }
}


void SegActive::getPosStats()
{
  const unsigned filterWidth = 1001;
  const unsigned filterMid = (filterWidth-1) >> 1;

  const unsigned ls = synthSpeed.size();
  if (ls <= filterWidth)
  {
    cout << "CAN'T COMPENSATE\n";
    return;
  }

  Mediator * mediator = MediatorNew(filterWidth);

  // Fill up the part that doesn't directly create values.
  for (unsigned i = 0; i < filterMid; i++)
  {
cout << "Entered " << synthPos[i] << endl;
    MediatorInsert(mediator, synthPos[i]);
  }

  // Fill up the large middle part with (mostly) full data.
  for (unsigned i = filterMid; i < ls-filterMid; i++)
  {
    MediatorInsert(mediator, synthPos[i]);
    GetMediatorStats(mediator, &posStats[i-filterMid]);
cout << "Entered " << synthPos[i] << ", got " <<
  posStats[i-filterMid].min << ", " <<
  posStats[i-filterMid].max << ", " <<
  posStats[i-filterMid].median << endl;

  }

  free(mediator);

  // Special case for the tail.  (Median really should have a
  // way to push out old elements without pushing in new ones.)

  mediator = MediatorNew(filterWidth);

  for (unsigned i  = 0; i < filterMid; i++)
    MediatorInsert(mediator, synthPos[ls-1-i]);

  for (unsigned i = 0; i < filterMid; i++)
  {
    MediatorInsert(mediator, synthPos[ls-1-filterMid-i]);
    GetMediatorStats(mediator, &posStats[ls-1-i]);
  }

  free(mediator);
}


void SegActive::getPeaks(vector<SignedPeak>& peaks) const
{
  const unsigned ls = synthPos.size();
  SignedPeak p;

  for (unsigned i = 1; i < ls-1; i++)
  {
    if (synthPos[i] > synthPos[i-1])
    {
      // Use the last of equals as the starting point.
      while (i < ls-1 && synthPos[i] == synthPos[i+1])
        i++;

      if (i < ls-1 && synthPos[i] > synthPos[i+1])
        p.maxFlag = true;
      else
        continue;
    }
    else if (synthPos[i] < synthPos[i-1])
    {
      while (i < ls-1 && synthPos[i] == synthPos[i+1])
        i++;

      if (i < ls-1 && synthPos[i] < synthPos[i+1])
        p.maxFlag = false;
      else
        continue;
    }

    p.index = i;
    p.value = synthPos[i];
    if (peaks.size() == 0)
    {
      p.leftFlank = i;
      p.leftRange = abs(synthPos[i] - synthPos[0]);
    }
    else
    {
      SignedPeak& prevPeak = peaks.back();
      p.leftFlank = i - prevPeak.index;
      p.leftRange = abs(synthPos[i] - synthPos[prevPeak.index]);
      prevPeak.rightFlank = p.leftFlank;
      prevPeak.rightRange = p.leftRange;
    }
    peaks.push_back(p);
  }

  if (peaks.size() > 0)
  {
    SignedPeak& lastPeak = peaks.back();
    lastPeak.rightFlank = ls - 1 - lastPeak.index;
    lastPeak.rightRange = 
      abs(synthPos[lastPeak.index] - synthPos.back());
  }
}


bool SegActive::checkPeaks(const vector<SignedPeak>& peaks) const
{
  bool flag = true;
  const unsigned lp = peaks.size();
  if (lp == 0)
    return true;
  
  unsigned sumLeft = 0, sumRight = 0;
  for (unsigned i = 0; i < lp; i++)
  {
    const SignedPeak& peak = peaks[i];
    if (synthPos[peak.index] != peak.value)
    {
      cout << "i " << i << ": value " << synthPos[peak.index] <<
        " vs. " << peak.value << endl;
      flag = false;
    }

    if (i == 0)
    {
      if (peak.leftFlank != peak.index)
      {
        cout << "i " << i << ": leftFlank " << peak.leftFlank <<
          " vs. " << peak.index << endl;
        flag = false;
      }
      
      if (abs(peak.leftRange - abs(peak.value - synthPos[0])) > 1.e-4)
      {
        cout << "i " << i << ": leftRange " << peak.leftRange <<
          " vs. value " << peak.value << ", sp[0] " << synthPos[0] << endl;
        flag = false;
      }

    }
    else if (i == lp-1)
    {
      if (peak.rightFlank + peak.index + 1 != synthPos.size())
      {
        cout << "i " << i << ": rightFlank " << peak.rightFlank <<
          " vs. " << peak.index << endl;
        flag = false;
      }
      
      if (abs(peak.rightRange - abs(peak.value - synthPos.back())) > 1.e-4)
      {
        cout << "i " << i << ": rightRange " << peak.rightRange <<
          " vs. value " << peak.value << ", sp[last] " << 
          synthPos.back() << endl;
        flag = false;
      }
    }
    else
    {
      if (peak.leftFlank != peaks[i-1].rightFlank)
      {
        cout << "i " << i << ": leftFlank LR " << peak.leftFlank <<
          " vs. " << peaks[i-1].rightFlank << endl;
        flag = false;
      }
      
      if (peak.leftRange != peaks[i-1].rightRange)
      {
        cout << "i " << i << ": leftRange LR " << peak.leftRange <<
          " vs. " << peaks[i-1].rightRange << endl;
        flag = false;
      }
      
      if (abs(peak.leftRange - abs(peak.value - peaks[i-1].value)) > 1.e-4)
      {
        cout << "i " << i << ": leftRange LV " << peak.leftRange <<
          " vs. value " << peak.value << ", " << peaks[i-1].value <<
          ", diff " << 
          abs(peak.leftRange - abs(peak.value - peaks[i-1].value)) << endl;
        flag = false;
      }
    }

    if (i > 0)
    {
      if (peak.maxFlag == peaks[i-1].maxFlag)
      {
        cout << "i " << i << " maxFlag " << peak.maxFlag << 
          " is same as predecessor" << endl;
        flag = false;
      }
    }

    sumLeft += peak.leftFlank;
    sumRight += peak.rightFlank;
  }

  if (sumLeft + peaks.back().rightFlank + 1 != synthPos.size())
  {
    cout << "sum left " << sumLeft + peaks.back().rightFlank <<
      " vs. " << synthPos.size() << endl;
    flag = false;
  }

  if (sumRight + peaks.front().leftFlank + 1 != synthPos.size())
  {
    cout << "sum right " << sumRight + peaks.front().leftFlank << 
      " vs. " << synthPos.size() << endl;
    flag = false;
  }
  return flag;
}


void SegActive::getLargePeaks(vector<SignedPeak>& peaks) const
{
  if (peaks.size() == 0)
    return;

  if (! SegActive::checkPeaks(peaks))
  {
    cout << "Peak fail" << endl;
  }

  // To have something especially for the tail.
  // TODO: Should be a better measure of the "average" range.
  const unsigned posMid = peaks[peaks.size() / 2].index;
  const float midRange = 
    posStats[posMid].max - posStats[posMid].min;

  bool changeFlag;
  do
  {
    changeFlag = false;
    const unsigned lp = peaks.size();
    for (unsigned i = 0; i < lp; i++)
    {
      const unsigned irev = lp-1-i;
      const SignedPeak& peak = peaks[irev];
      const unsigned index = peak.index;
      const float range = posStats[index].max - posStats[index].min;

// if (index > 1200 && index < 2000)
  // cout << "HERE" << endl;
      const float rangeSum = peak.leftRange + peak.rightRange;
      if (rangeSum < 0.3 * range || rangeSum < 0.3 * midRange)
      {
        // TODO Do we ever need a second pass?
        changeFlag = true;

        if (irev == 0)
        {
          unsigned survivor;
          if ((peak.maxFlag && synthPos.front() >= peaks[irev+1].value) ||
              (! peak.maxFlag && synthPos.front() <= peaks[irev+1].value))
          {
            // We'll change the first real peak (a minimum if maxFlag,
            // a maximum otherwise).
            survivor = irev + 1;
          }
          else
          {
            // We have to remove the next peak as well (a minimum if
            // maxFlag, a maximum otherwise).
            survivor = irev + 2;
          }

          peaks[survivor].leftFlank = peaks[survivor].index;
          peaks[survivor].leftRange = 
            abs(peaks[survivor].value - synthPos.front());

          peaks.erase(peaks.begin() + irev, peaks.begin() + survivor);
if (! SegActive::checkPeaks(peaks))
{
  cout << "Peak fail irev = 0" << endl << endl;
}
          continue;
        }
        else if (irev == peaks.size()-1)
        {
          unsigned survivor;
          if ((peak.maxFlag && synthPos.back() >= peaks[irev-1].value) ||
              (! peak.maxFlag && synthPos.back() <= peaks[irev-1].value))
          {
            // We'll change the last real peak (a minimum if maxFlag,
            // a maximum otherwise).
            survivor = irev - 1;
          }
          else
          {
            // We have to remove the last peak as well (a minimum if
            // maxFlag, a maximum otherwise).
            survivor = irev - 2;
            i++;
          }

          peaks[survivor].rightFlank = 
            synthPos.size() - 1 - peaks[survivor].index;
          peaks[survivor].rightRange = 
            abs(peaks[survivor].value - synthPos.back());

          peaks.erase(peaks.begin() + survivor + 1, peaks.end());
if (! SegActive::checkPeaks(peaks))
{
  cout << "Peak fail irev = last" << endl << endl;
}
          continue;
        }


        if (peak.maxFlag)
        {
unsigned path = 0;
          // We care about this as it will generally cause its two
          // neighbors to combine into one.
          // Keep the deeper of the neighboring minima.
          if (peaks[irev-1].value > peaks[irev+1].value)
          {
path = 1;
            // We really should keep irev+1, but it's more
            // convenient to keep irev-1.
            peaks[irev-1].index = peaks[irev+1].index;
            peaks[irev-1].value = peaks[irev+1].value;
            peaks[irev-1].leftFlank += 
              peaks[irev].leftFlank + peaks[irev].rightFlank;
            peaks[irev-1].rightFlank = peaks[irev+1].rightFlank;
            peaks[irev-1].leftRange += 
              peaks[irev].rightRange - peaks[irev].leftRange;
            peaks[irev-1].rightRange = peaks[irev+1].rightRange;

            if (irev >= 2)
            {
              peaks[irev-2].rightFlank = peaks[irev-1].leftFlank;
              peaks[irev-2].rightRange = peaks[irev-1].leftRange;
            }
          }
          else
          {
path = 2;
            // We're keeping irev-1.
            peaks[irev-1].rightFlank += 
              peaks[irev+1].leftFlank + peaks[irev+1].rightFlank;
            peaks[irev-1].rightRange += 
              peaks[irev+1].rightRange - peaks[irev+1].leftRange;
            
            if (irev <= lp-3)
            {
              peaks[irev+2].leftFlank = peaks[irev-1].rightFlank;
              peaks[irev+2].leftRange = peaks[irev-1].rightRange;
            }
          }


          peaks.erase(peaks.begin()+irev, peaks.begin()+irev+2);
if (! SegActive::checkPeaks(peaks))
{
  cout << "Peak fail max, irev " << irev << " path " << path << endl << endl;
}
        }
        else
        {
unsigned path = 0;
          // TODO Combine with above
          //
          // We're mostly looking for minima, but we still have to
          // keep the maxima organized, as otherwise the deletion of
          // two peaks at a time gets confused.
          // Keep the higher of the neighboring maxima.
          if (peaks[irev-1].value < peaks[irev+1].value)
          {
path = 1;
            // We really should keep irev+1, but it's more
            // convenient to keep irev-1.
            peaks[irev-1].index = peaks[irev+1].index;
            peaks[irev-1].value = peaks[irev+1].value;
            peaks[irev-1].leftFlank += 
              peaks[irev].leftFlank + peaks[irev].rightFlank;
            peaks[irev-1].rightFlank = peaks[irev+1].rightFlank;

            peaks[irev-1].leftRange += 
              peaks[irev].rightRange - peaks[irev].leftRange;
            peaks[irev-1].rightRange = peaks[irev+1].rightRange;

            if (irev >= 2)
            {
              peaks[irev-2].rightFlank = peaks[irev-1].leftFlank;
              peaks[irev-2].rightRange = peaks[irev-1].leftRange;
            }
          }
          else
          {
path = 2;
            // We're keeping irev-1.
            peaks[irev-1].rightFlank += 
              peaks[irev+1].leftFlank + peaks[irev+1].rightFlank;

            peaks[irev-1].rightRange += 
              peaks[irev+1].rightRange - peaks[irev+1].leftRange;

            if (irev <= lp-3)
            {
              peaks[irev+2].leftFlank = peaks[irev-1].rightFlank;
              peaks[irev+2].leftRange = peaks[irev-1].rightRange;
            }
          }

          peaks.erase(peaks.begin()+irev, peaks.begin()+irev+2);
if (! SegActive::checkPeaks(peaks))
{
  cout << "Peak fail min, irev " << irev << " path " << path << endl << endl;
}
        }
      }
    }
  }
  while (changeFlag);
}


void SegActive::levelizePos(const vector<SignedPeak>& peaks)
{
  const unsigned lp = peaks.size();
  if (lp == 0)
    return;

  SignedPeak const * prevPeak = nullptr;
  for (const SignedPeak& peak: peaks)
  {
    // We levelize such that the maxima are are zero, and
    // the minima are negative.
    if (! peak.maxFlag)
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
      synthPos[i] -= prevValue + step * (i - prevIndex);

    prevPeak = &peak;
  }

  // Do the last bit.
  const SignedPeak& lastPeak = peaks.back();
  for (unsigned i = lastPeak.index; i < synthPos.size(); i++)
    synthPos[i] -= lastPeak.value;
}


void SegActive::makeSynthPeaks(const vector<SignedPeak>& peaks)
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


bool SegActive::detect(
  const vector<double>& samples,
  const vector<Interval>& active,
  const double mean)
{
/*
for (unsigned i = 0; i < samples.size(); i++)
  cout << i << ";" << samples[i] << "\n";
cout << "\n";
*/

  writeInterval.first = active.front().first;
  writeInterval.len = active.back().first + active.back().len - 
    writeInterval.first;

  synthSpeed.resize(writeInterval.len);
  synthPos.resize(writeInterval.len);
  for (unsigned i = 0; i < writeInterval.len; i++)
  {
    synthSpeed[i] = 0.f;
    synthPos[i] = 0.f;
  }

  for (const Interval& aint: active)
  {
    SegActive::integrate(samples, aint, mean);

    // SegActive::compensateMedian();
    SegActive::compensateSpeed();

    SegActive::integrateFloat(aint);
  }

  posStats.resize(writeInterval.len);
  // SegActive::getPosStats();
  SegActive::getPosStatsNew(true);
  SegActive::getPosStatsNew(false);

  // Get a reasonable guess at the peaks.
  vector<SignedPeak> posPeaks;
  posPeaks.clear();
  SegActive::getPeaks(posPeaks);
  SegActive::getLargePeaks(posPeaks);
/*

  // Straighten out the position trace, then try again.
  SegActive::levelizePos(posPeaks);
  vector<SignedPeak> posLevelPeaks;
  posLevelPeaks.clear();
  SegActive::getPeaks(posLevelPeaks);
  SegActive::getLargePeaks(posLevelPeaks);

*/
  synthPeaks.resize(writeInterval.len);
  SegActive::makeSynthPeaks(posPeaks);
  // SegActive::makeSynthPeaks(posLevelPeaks);

  return true;
}


void SegActive::writeBinary(
  const string& origname,
  const string& speeddirname,
  const string& posdirname,
  const string& peakdirname) const
{
  // Make the transient file name by:
  // * Replacing /raw/ with /dirname/
  // * Adding _offset_N before .dat

  // TODO Should probably come out of Seg*.cpp and go somewhere
  // central.

  if (synthSpeed.size() == 0)
    return;

  string sname = origname;
  string pname = origname;
  string kname = origname;
  string mname = origname;
  auto tp1 = sname.find("/raw/");
  if (tp1 == string::npos)
    return;

  auto tp2 = sname.find(".dat");
  if (tp2 == string::npos)
    return;

  sname.insert(tp2, "_offset_" + to_string(writeInterval.first));
  sname.replace(tp1, 5, "/" + speeddirname + "/");

  pname.insert(tp2, "_offset_" + to_string(writeInterval.first));
  pname.replace(tp1, 5, "/" + posdirname + "/");

  kname.insert(tp2, "_offset_" + to_string(writeInterval.first));
  kname.replace(tp1, 5, "/" + peakdirname + "/");

  mname.insert(tp2, "_offset_" + to_string(writeInterval.first));
  mname.replace(tp1, 5, "/max/");

  ofstream sout(sname, std::ios::out | std::ios::binary);
  sout.write(reinterpret_cast<const char *>(synthSpeed.data()),
    synthSpeed.size() * sizeof(float));
  sout.close();

  ofstream pout(pname, std::ios::out | std::ios::binary);
  pout.write(reinterpret_cast<const char *>(synthPos.data()),
    synthPos.size() * sizeof(float));
  pout.close();

  ofstream kout(kname, std::ios::out | std::ios::binary);
  kout.write(reinterpret_cast<const char *>(synthPeaks.data()),
    synthPeaks.size() * sizeof(float));
  kout.close();

vector<float> mmax(posStats.size());
for (unsigned i = 0; i < posStats.size(); i++)
  mmax[i] = posStats[i].max;

  ofstream mout(mname, std::ios::out | std::ios::binary);
  mout.write(reinterpret_cast<const char *>(mmax.data()),
    mmax.size() * sizeof(float));
  mout.close();
}


string SegActive::headerCSV() const
{
  stringstream ss;

  return ss.str();
}


string SegActive::strCSV() const
{
  stringstream ss;
  
  return ss.str();
}
