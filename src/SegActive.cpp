#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <math.h>

#include "SegActive.h"
#include "Timers.h"
#include "write.h"

#define SAMPLE_RATE 2000.

extern Timers timers;


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
  const Interval& aint)
{
  // Integrate acceleration into speed.
  // synthSpeed is then in 0.01 m/s.
  const float factor = 100.f *
    static_cast<float>(G_FORCE / SAMPLE_RATE);

  synthSpeed[aint.first - writeInterval.first] = 
    factor * static_cast<float>(samples[aint.first]);

  for (unsigned i = aint.first+1; i < aint.first + aint.len; i++)
    synthSpeed[i - writeInterval.first] = 
      synthSpeed[i - writeInterval.first - 1] + 
      factor * static_cast<float>(samples[i]);
}


void SegActive::compensateSpeed()
{
  // TODO Try highpass instead!!

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

  for (unsigned i = 0; i < filterMid; i++)
    newSpeed[i] = newSpeed[filterMid];

  const float step1 = 
    (synthSpeed[ls-1] - newSpeed[ls-filterMid-1]) / filterMid;

  for (unsigned i = ls - filterMid; i < ls; i++)
    newSpeed[i] = synthSpeed[ls-1] - step1 * (ls-1-i);
  
  for (unsigned i = 0; i < ls; i++)
    synthSpeed[i] -= newSpeed[i];
}


void SegActive::integrateFloat()
{
  // Integrate speed into position.
  // synthPos is then in 0.1 mm.

  const float factor = 100.f / static_cast<float>(SAMPLE_RATE);

  synthPos[0] = factor * synthSpeed[0];

  for (unsigned i = 1; i < synthPos.size(); i++)
    synthPos[i] = synthPos[i-1] + factor * synthSpeed[i];
}


void SegActive::highpass(vector<float>& integrand)
{
  // Fifth-order high-pass Butterworth filter with low cut-off.
  // The filter is run forwards and backwards in order to
  // get linear phase (like in Python's filtfilt, but without
  // the padding or other clever stuff).

  const unsigned order = 5;

  const vector<double> num
  {
    0.9749039346036602,
    -4.8745196730183009,
    9.7490393460366018,
    -9.7490393460366018,
    4.8745196730183009,
    -0.9749039346036602
  };

  const vector<double> denom
  {
    1.,
    -4.9491681155566063,
    9.7979620071891631,
    -9.698857155930046,
    4.8005009469355944,
    -0.9504376817056966
  };

  const unsigned ls = integrand.size();
  vector<double> forward(ls);

  vector<double> state(order+1);
  for (unsigned i = 0; i < order+1; i++)
    state[i] = 0.;

  for (unsigned i = 0; i < ls; i++)
  {
    forward[i] = num[0] * static_cast<double>(integrand[i]) + state[0];

    for (unsigned j = 0; j < order; j++)
    {
      state[j] = num[j+1] * static_cast<double>(integrand[i]) - 
        denom[j+1] * forward[i] + state[j+1];
    }
  }

  vector<double> backward(ls);
  for (unsigned i = 0; i < order+1; i++)
    state[i] = 0.;

  for (unsigned i = 0; i < ls; i++)
  {
    const unsigned irev = ls-1-i;

    backward[irev] = num[0] * forward[irev] + state[0];

    for (unsigned j = 0; j < order; j++)
    {
      state[j] = num[j+1] * forward[irev] - 
        denom[j+1] * backward[irev] + state[j+1];
    }
  }

  for (unsigned i = 0; i < ls; i++)
    integrand[i] = static_cast<float>(backward[i]);
}


bool SegActive::detect(
  const vector<double>& samples,
  const Interval& active)
{
  timers.start(TIMER_CONDITION);

  writeInterval.first = active.first;
  writeInterval.len = active.first + active.len - 
    writeInterval.first;

  synthSpeed.resize(writeInterval.len);
  synthPos.resize(writeInterval.len);

  SegActive::integrate(samples, active);
  SegActive::highpass(synthSpeed);
  // SegActive::compensateSpeed();

  SegActive::integrateFloat();
  SegActive::highpass(synthPos);

  timers.stop(TIMER_CONDITION);

  timers.start(TIMER_DETECT_PEAKS);
  peakDetect.log(synthPos, writeInterval.first);
  // peakDetect.reduce();
  peakDetect.reduceNew();
  timers.stop(TIMER_DETECT_PEAKS);

  synthPeaks.resize(writeInterval.len);
  // peakDetect.makeSynthPeaks(synthPeaks);
  peakDetect.makeSynthPeaksList(synthPeaks);

  return true;
}


void SegActive::getPeakTimes(vector<PeakTime>& times) const
{
  peakDetect.getPeakTimes(times);
}


void SegActive::writeSpeed(
  const string& origname,
  const string& dirname) const
{
  writeBinary(origname, dirname, writeInterval.first, synthSpeed);
}


void SegActive::writePos(
  const string& origname,
  const string& dirname) const
{
  writeBinary(origname, dirname, writeInterval.first, synthPos);
}


void SegActive::writePeak(
  const string& origname,
  const string& dirname) const
{
  writeBinary(origname, dirname, writeInterval.first, synthPeaks);
}

