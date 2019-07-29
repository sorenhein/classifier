#include <iostream>

#include "Filter.h"

#include "../const.h"
#include "../Except.h"

#include "../stats/Timers.h"

#include "../util/io.h"

extern vector<Timers> timers;


Filter::Filter()
{
}


Filter::~Filter()
{
}


void Filter::reset()
{
}


void Filter::doubleToFloat(
  const vector<double>& samples,
  vector<float>& samplesFloat)
{
  for (unsigned i = 0; i < samples.size(); i++)
    samplesFloat[i] = static_cast<float>(samples[i]);
}


void Filter::integrateFloat(
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


void Filter::filterFloat(
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


void Filter::highpass(
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


bool Filter::detect(
  const vector<double>& samples,
  const double sampleRate,
  const unsigned start,
  const unsigned len)
{
  // TODO Don't use exact comparison
  if (sampleRate != 2000.)
    THROW(ERR_UNKNOWN_SAMPLE_RATE, "Unknown sample rate");

  startInterval = start;
  lenInterval = len;

  accelFloat.resize(samples.size());
  synthSpeed.resize(lenInterval);
  synthPos.resize(lenInterval);

  Filter::doubleToFloat(samples, accelFloat);

  Filter::filterFloat(Butterworth5LPF_float, accelFloat);

  Filter::integrateFloat(accelFloat, static_cast<float>(sampleRate), 
    true, start, len, synthSpeed);

  Filter::highpass(Butterworth5HPF_double, synthSpeed);

  Filter::integrateFloat(synthSpeed, static_cast<float>(sampleRate), 
    false, 0, len, synthPos);

  Filter::highpass(Butterworth5HPF_double, synthPos);

  // TODO Ideas:
  // - Get out of doubles in highpass(), use filterFloat
  // - Combine integration with a highpass filter.
  //   Note that the filter is run twice, so it will also integrate twice.
  //   Therefore it ought to combine three function calls.
  //
  // Filter::filterFloat(numNoDCFloat, denomNoDCFloat, synthSpeed);
  // Filter::filterFloat(numNoDCFloat, denomNoDCFloat, synthPos);

  return true;
}


const vector<float>& Filter::getDeflection() const
{
  return synthPos;
}


void Filter::writeSpeed(const string& filename) const
{
  writeBinary(filename, startInterval, synthSpeed);
}


void Filter::writePos(const string& filename) const
{
  writeBinary(filename, startInterval, synthPos);
}

