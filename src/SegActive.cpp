#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <math.h>

#include "SegActive.h"
#include "const.h"
#include "Except.h"

#include "database/Control.h"

#include "util/io.h"

#include "stats/Timers.h"

extern vector<Timers> timers;


SegActive::SegActive()
{
}


SegActive::~SegActive()
{
}


void SegActive::reset()
{
}


void SegActive::doubleToFloat(
  const vector<double>& samples,
  vector<float>& samplesFloat)
{
  for (unsigned i = 0; i < samples.size(); i++)
    samplesFloat[i] = static_cast<float>(samples[i]);
}


void SegActive::integrateFloat(
  const vector<float>& integrand,
  const float sampleRate,
  const bool a2vFlag,
  const unsigned startIndex,
  const unsigned len,
  vector<float>& result) const
{
  // If acceleration -> speed: Speed is in 0.01 m/s.
  // If speed -> position: Position is in 0.1 m.

  const float factor =
    (a2vFlag ? 100.f * G_FORCE / sampleRate : 100.f / sampleRate);

  result[0] = factor * integrand[startIndex];

  for (unsigned i = 1; i < len; i++)
    result[i] = result[i-1] + factor * integrand[startIndex + i];
}


void SegActive::filterFloat(
  const FilterFloat& filter,
  vector<float>& integrand)
{
  const unsigned ls = integrand.size();
  vector<float> forward(ls);

  const unsigned order = filter.numerator.size()-1;
  vector<float> state(order+1);
  for (unsigned i = 0; i < order+1; i++)
    state[i] = 0.;

  for (unsigned i = 0; i < ls; i++)
  {
    forward[i] = filter.numerator[0] * integrand[i] + state[0];

    for (unsigned j = 0; j < order; j++)
    {
      state[j] = filter.numerator[j+1] * integrand[i] - 
        filter.denominator[j+1] * forward[i] + state[j+1];
    }
  }

  for (unsigned i = 0; i < order+1; i++)
    state[i] = 0.;

  for (unsigned i = 0; i < ls; i++)
  {
    const unsigned irev = ls-1-i;

    integrand[irev] = filter.numerator[0] * forward[irev] + state[0];

    for (unsigned j = 0; j < order; j++)
    {
      state[j] = filter.numerator[j+1] * forward[irev] - 
        filter.denominator[j+1] * integrand[irev] + state[j+1];
    }
  }
}


void SegActive::highpass(
  const FilterDouble& filter,
  vector<float>& integrand)
{
  const unsigned ls = integrand.size();
  vector<double> forward(ls);

  const unsigned order = filter.numerator.size()-1;
  vector<double> state(order+1);
  for (unsigned i = 0; i < order+1; i++)
    state[i] = 0.;

  for (unsigned i = 0; i < ls; i++)
  {
    forward[i] = filter.numerator[0] * 
      static_cast<double>(integrand[i]) + state[0];

    for (unsigned j = 0; j < order; j++)
    {
      state[j] = filter.numerator[j+1] * static_cast<double>(integrand[i]) - 
        filter.denominator[j+1] * forward[i] + state[j+1];
    }
  }

  vector<double> backward(ls);
  for (unsigned i = 0; i < order+1; i++)
    state[i] = 0.;

  for (unsigned i = 0; i < ls; i++)
  {
    const unsigned irev = ls-1-i;

    backward[irev] = filter.numerator[0] * forward[irev] + state[0];

    for (unsigned j = 0; j < order; j++)
    {
      state[j] = filter.numerator[j+1] * forward[irev] - 
        filter.denominator[j+1] * backward[irev] + state[j+1];
    }
  }

  for (unsigned i = 0; i < ls; i++)
    integrand[i] = static_cast<float>(backward[i]);
}


bool SegActive::detect(
  const vector<double>& samples,
  const double sampleRate,
  const Interval& active,
  const Control& control,
  const unsigned thid,
  Imperfections& imperf)
{
  // TODO Don't use exact comparison
  if (sampleRate != 2000.)
    THROW(ERR_UNKNOWN_SAMPLE_RATE, "Unknown sample rate");

  timers[thid].start(TIMER_CONDITION);

  writeInterval.first = active.first;
  writeInterval.len = active.len;

  accelFloat.resize(samples.size());
  synthSpeed.resize(writeInterval.len);
  synthPos.resize(writeInterval.len);

  SegActive::doubleToFloat(samples, accelFloat);

  SegActive::filterFloat(Butterworth5LPF_float, accelFloat);

  SegActive::integrateFloat(accelFloat, static_cast<float>(sampleRate), 
    true, active.first, active.len, synthSpeed);

  SegActive::highpass(Butterworth5HPF_double, synthSpeed);

  SegActive::integrateFloat(synthSpeed, static_cast<float>(sampleRate), 
    false, 0, active.len, synthPos);

  SegActive::highpass(Butterworth5HPF_double, synthPos);

  // TODO Ideas:
  // - Get out of doubles in highpass(), use filterFloat
  // - Combine integration with a highpass filter.
  //   Note that the filter is run twice, so it will also integrate twice.
  //   Therefore it ought to combine three function calls.
  //
  // SegActive::filterFloat(numNoDCFloat, denomNoDCFloat, synthSpeed);
  // SegActive::filterFloat(numNoDCFloat, denomNoDCFloat, synthPos);

  timers[thid].stop(TIMER_CONDITION);

  timers[thid].start(TIMER_DETECT_PEAKS);
  peakDetect.reset();
  peakDetect.log(synthPos, writeInterval.first);
  peakDetect.reduce(control, imperf);
  timers[thid].stop(TIMER_DETECT_PEAKS);


  synthPeaks.resize(writeInterval.len);
  peakDetect.makeSynthPeaks(synthPeaks);

  return true;
}


void SegActive::logPeakStats(
  const vector<double>& posTrue,
  const string& trainTrue,
  const double speedTrue,
  PeakStats& peakStats)
{
  peakDetect.logPeakStats(posTrue, trainTrue, speedTrue, peakStats);
}


bool SegActive::getAlignment(
  vector<double>& times,
  vector<int>& actualToRef,
  unsigned& numFrontWheels)
{
  return peakDetect.getAlignment(times, actualToRef, numFrontWheels);
}


void SegActive::getPeakTimes(
  vector<double>& times,
  unsigned& numFrontWheels) const
{
  peakDetect.getPeakTimes(times, numFrontWheels);
}


void SegActive::writeSpeed(const string& filename) const
{
  writeBinary(filename, writeInterval.first, synthSpeed);
}


void SegActive::writePos(const string& filename) const
{
  writeBinary(filename, writeInterval.first, synthPos);
}


void SegActive::writePeak(const string& filename) const
{
  writeBinary(filename, writeInterval.first, synthPeaks);
}

