#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <math.h>

#include "SegActive.h"
#include "Timers.h"
#include "write.h"

#define SAMPLE_RATE 2000.f

extern Timers timers;


// Fifth-order high-pass Butterworth filter with low cut-off.
// The filter is run forwards and backwards in order to
// get linear phase (like in Python's filtfilt, but without
// the padding or other clever stuff).  It eliminates slow/DC
// components.
//
// from scipy import signal
// num, denom = signal.butter(5, 0.005, btype='high')
// numpy.set_printoptions(precision=16)

const unsigned order = 5;

const vector<double> numNoDC
{
  0.9749039346036602,
  -4.8745196730183009,
  9.7490393460366018,
  -9.7490393460366018,
  4.8745196730183009,
  -0.9749039346036602
};

const vector<double> denomNoDC
{
  1.,
  -4.9491681155566063,
  9.7979620071891631,
  -9.698857155930046,
  4.8005009469355944,
  -0.9504376817056966
};

const vector<double> numNoDCFloat
{
  0.9749039346036602,
  -4.8745196730183009,
  9.7490393460366018,
  -9.7490393460366018,
  4.8745196730183009,
  -0.9749039346036602
};

const vector<double> denomNoDCFloat
{
  1.,
  -4.9491681155566063,
  9.7979620071891631,
  -9.698857155930046,
  4.8005009469355944,
  -0.9504376817056966
};

// Fifth-order low-pass Butterworth filter.  Eliminates
// high frequencies which probably contribute to jitter and drift.
//
// num, denom = signal.butter(5, 0.04, btype = 'low')

// num is also 8.0423564219711682e-07 * (1, 5, 10, 10, 5, 1)

const vector<double> numNoHF
{
  8.0423564219711682e-07,
  4.0211782109855839e-06,
  8.0423564219711678e-06,
  8.0423564219711678e-06,
  4.0211782109855839e-06,
  8.0423564219711682e-07
};

const vector<double> denomNoHF
{
  1., 
  -4.5934213998076876,
  8.4551152235101341,
 -7.7949183180444468,
 3.5989027680539127, 
 -0.6656525381713611
};

const vector<double> numNoHFFloat
{
  8.0423564219711682e-07,
  4.0211782109855839e-06,
  8.0423564219711678e-06,
  8.0423564219711678e-06,
  4.0211782109855839e-06,
  8.0423564219711682e-07
};

const vector<double> denomNoHFFloat
{
  1., 
  -4.5934213998076876,
  8.4551152235101341,
 -7.7949183180444468,
 3.5989027680539127, 
 -0.6656525381713611
};


SegActive::SegActive()
{
}


SegActive::~SegActive()
{
}


void SegActive::reset()
{
}


void SegActive::doubleToFloat(const vector<double>& samples)
{
  samplesFloat.resize(samples.size());
  for (unsigned i = 0; i < samples.size(); i++)
    samplesFloat[i] = static_cast<float>(samples[i]);
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


void SegActive::integrateFloat(
  const vector<float>& integrand,
  const bool a2vFlag,
  const unsigned startIndex,
  const unsigned len,
  vector<float>& result) const
{
  // If acceleration -> speed: Speed is in 0.01 m/s.
  // If speed -> position: Position is in 0.1 mm.

  const float factor =
    (a2vFlag ? 100.f * G_FORCE / SAMPLE_RATE : 100.f / SAMPLE_RATE);

  result[0] = factor * integrand[startIndex];

  for (unsigned i = 1; i < len; i++)
    result[i] = result[i-1] + factor * integrand[startIndex + i];
}


void SegActive::filterFloat(
  const vector<float>& num,
  const vector<float>& denom,
  vector<float>& integrand)
{
  const unsigned ls = integrand.size();
  vector<float> forward(ls);

  vector<float> state(order+1);
  for (unsigned i = 0; i < order+1; i++)
    state[i] = 0.;

  for (unsigned i = 0; i < ls; i++)
  {
    forward[i] = num[0] * integrand[i] + state[0];

    for (unsigned j = 0; j < order; j++)
    {
      state[j] = num[j+1] * integrand[i] - 
        denom[j+1] * forward[i] + state[j+1];
    }
  }

  vector<float> backward(ls);
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
    integrand[i] = backward[i];
}


void SegActive::highpass(
  const vector<double>& num,
  const vector<double>& denom,
  vector<float>& integrand)
{
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
  const Interval& active,
  const Control& control,
  Imperfections& imperf)
{
  timers.start(TIMER_CONDITION);

  writeInterval.first = active.first;
  // writeInterval.len = active.first + active.len - 
    // writeInterval.first;
  writeInterval.len = active.len;

  // SegActive::doubleToFloat(samples);
  // SegActive::highpass(numNoHF, denomNoHF, samplesFloat);

  synthSpeed.resize(writeInterval.len);
  synthPos.resize(writeInterval.len);

  // SegActive::integrateFloat(samplesFloat, active);
  SegActive::integrate(samples, active);
  SegActive::highpass(numNoDC, denomNoDC, synthSpeed);

  SegActive::integrateFloat(synthSpeed, false, 0, active.len, synthPos);
  SegActive::highpass(numNoDC, denomNoDC, synthPos);

  timers.stop(TIMER_CONDITION);

  timers.start(TIMER_DETECT_PEAKS);
  peakDetect.reset();
  peakDetect.log(synthPos, writeInterval.first);
  peakDetect.reduce(control, imperf);
  timers.stop(TIMER_DETECT_PEAKS);


  synthPeaks.resize(writeInterval.len);
  peakDetect.makeSynthPeaks(synthPeaks);

  return true;
}


void SegActive::logPeakStats(
  const vector<PeakPos>& posTrue,
  const string& trainTrue,
  const double speedTrue,
  PeakStats& peakStats)
{
  peakDetect.logPeakStats(posTrue, trainTrue, speedTrue, peakStats);
}


void SegActive::getPeakTimes(
  vector<PeakTime>& times,
  unsigned& numFrontWheels) const
{
  peakDetect.getPeakTimes(times, numFrontWheels);
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

