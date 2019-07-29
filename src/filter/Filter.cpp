#include "Filter.h"

#include "../const.h"
#include "../Except.h"

#include "../util/io.h"


Filter::Filter()
{
}


Filter::~Filter()
{
}


void Filter::reset()
{
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

#include <iostream>
#include <iomanip>

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
// if (i < 10)
  // cout << "FORWARD " << i << ": " << forward[i] << "\n";
// if (i + 10 >= ls)
  // cout << "FORWARD LAST " << i << ": " << forward[i] << "\n";
  }

  for (unsigned i = 0; i < order+1; i++)
    state[i] = 0.;

  for (unsigned i = 0; i < ls; i++)
  {
    const unsigned irev = ls-1-i;

    integrand[irev] = filter.numerator[0] * forward[irev] + state[0];
/*
if (i < 10)
{
  cout << "REVERSE " << i << ", irev " << irev << ": " << integrand[irev] << "\n";
  cout << "num0 " << filter.numerator[0] << ", fwd " <<
    forward[irev] << ", state " << state[0] << endl;
}
*/


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


void Filter::makeIntegratingFilter(
  const FilterFloat& filterFloatIn,
  FilterFloat& filterFloat) const
{
  const unsigned orderIn = filterFloatIn.numerator.size() - 1;
  filterFloat.numerator.resize(orderIn + 1);
  filterFloat.denominator.resize(orderIn + 1);

  // Cancel out one DC zero in the numerator.
  filterFloat.numerator[0] = filterFloatIn.numerator[0];
  for (unsigned i = 1; i < orderIn; i++)
  {
    filterFloat.numerator[i] = filterFloat.numerator[i-1] +
      filterFloatIn.numerator[i];
  }

  for (unsigned i = 0; i <= orderIn; i++)
    filterFloat.denominator[i] = filterFloatIn.denominator[i];
}


void Filter::makeIntegratingFilterDouble(
  const FilterDouble& filterDoubleIn,
  FilterDouble& filterDouble) const
{
  const unsigned orderIn = filterDoubleIn.numerator.size() - 1;
  filterDouble.numerator.resize(orderIn + 1);
  filterDouble.denominator.resize(orderIn + 1);

  // Cancel out one DC zero in the numerator.
  filterDouble.numerator[0] = filterDoubleIn.numerator[0];
  for (unsigned i = 1; i < orderIn; i++)
  {
    filterDouble.numerator[i] = filterDouble.numerator[i-1] +
      filterDoubleIn.numerator[i];
  }

  for (unsigned i = 0; i <= orderIn; i++)
    filterDouble.denominator[i] = filterDoubleIn.denominator[i];
}


void Filter::detect(
  const double sampleRate,
  const unsigned start,
  const unsigned len)
{
  // TODO Don't use exact comparison
  if (sampleRate != 2000.)
    THROW(ERR_UNKNOWN_SAMPLE_RATE, "Unknown sample rate");

  startInterval = start;
  lenInterval = len;

  synthSpeed.resize(lenInterval);
  synthPos.resize(lenInterval);

  Filter::filterFloat(Butterworth5LPF_float, accelFloat);

/*
vector<float> accelCopy;
accelCopy.resize(accelFloat.size());
for (unsigned i = 0; i < accelFloat.size(); i++)
  accelCopy[i] = accelFloat[i];

FilterFloat Butterworth6HPF_integr;
Filter::makeIntegratingFilter(Butterworth5HPF_float,
  Butterworth6HPF_integr);
  */

/*
FilterDouble Butterworth6HPF_integr;
Filter::makeIntegratingFilterDouble(Butterworth5HPF_double,
  Butterworth6HPF_integr);

cout << "NEW FILTER\n";
for (unsigned i = 0; i < Butterworth6HPF_integr.numerator.size(); i++)
  cout << i << ";" << Butterworth6HPF_integr.numerator[i] << ";" <<
  Butterworth6HPF_integr.denominator[i] << endl << endl;
  */

// Filter::filterFloat(Butterworth6HPF_integr, accelCopy);
// Filter::highpass(Butterworth6HPF_integr, accelCopy);

  Filter::integrateFloat(accelFloat, static_cast<float>(sampleRate), 
    true, start, len, synthSpeed);

  Filter::highpass(Butterworth5HPF_double, synthSpeed);

  Filter::integrateFloat(synthSpeed, static_cast<float>(sampleRate), 
    false, 0, len, synthPos);

  Filter::highpass(Butterworth5HPF_double, synthPos);

/*
cout << "COMPARISON\n";
for (unsigned i = 0; i < synthPos.size(); i++)
{
  cout << i << ";" << fixed << setprecision(4) << synthPos[i] << ";" <<
    fixed << setprecision(4) << accelCopy[i] << "\n";
}
*/

  // TODO Ideas:
  // - Get out of doubles in highpass(), use filterFloat
  // - Combine integration with a highpass filter.
  //   Note that the filter is run twice, so it will also integrate twice.
  //   Therefore it ought to combine three function calls.
  //
  // Filter::filterFloat(numNoDCFloat, denomNoDCFloat, synthSpeed);
  // Filter::filterFloat(numNoDCFloat, denomNoDCFloat, synthPos);
}


vector<float>& Filter::getAccel()
{
  return accelFloat;
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

