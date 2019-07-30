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
  vector<float>& integrand) const
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


void Filter::runIntegratingFilterFloat(
  const vector<FilterFloat>& filterFloatIn,
  const double sampleRate,
  vector<float>& pos) const
{
  /* 
     Conceptually speaking, we conduct the following steps:

     1. Integrate from acceleration to speed.
        As the acceleration is in units of g, we multiply by
          100.f * G_FORCE / sampleRate
        in order to get a speed unit of 0.01 m/s.
     2. High-pass filter the speed in order to reduce or remove
        the numerical drift in the integration.
     3. Integrate from speed to position.
        We multiply by
          100.f / sampleRate
        in order to get a position unit of 0.1 mm.
     4. High-pass filter the position in order to reduce or remove
        the numerical drift in the integration.

     The high-pass filter has a number of zeroes at DC.
     Therefore we can accomplish an integration by cancelling out
     one of those zeroes.

     The high-pass filter is run once from left to right and once from
     right to left in order to have zero phase overall.  Therefore
     if we cancel out one zero at DC, we get two integrations.

     In summary we can conduct all four steps by cancelling out
     a single zero at DC, and scaling the signal by the product of
     the two constants.  I'm not quite sure about the negative sign
     to be honest, but it makes the deflections towards the ground
     have negative signs.

     The filter is implemented for numerical stability as the product
     of several second-order sections ("SOS" in Python-speak).
   */

  const float sampleRateF = static_cast<float>(sampleRate);
  const float factor = - (100.f * G_FORCE / sampleRateF) *
    (100.f / sampleRateF);

  pos.resize(lenInterval);
  for (unsigned i = 0; i < lenInterval; i++)
    pos[i] = factor * accelFloat[startInterval + i];

  // Filter by each of the second-order stages in succession.
  for (auto& f: filterFloatIn)
    Filter::filterFloat(f, pos);
}


void Filter::runIntegratingFilterDouble(
  const vector<FilterDouble>& filterDoubleIn,
  const double sampleRate,
  vector<float>& pos)
{
  const float sampleRateF = static_cast<float>(sampleRate);
  const float factor = - (100.f * G_FORCE / sampleRateF) *
    (100.f / sampleRateF);

  pos.resize(lenInterval);
  for (unsigned i = 0; i < lenInterval; i++)
    pos[i] = factor * accelFloat[startInterval + i];

  // Filter by each of the second-order stages in succession.
  for (auto& f: filterDoubleIn)
    Filter::highpass(f, pos);
}


void Filter::runIntegrationSeparately(
  const FilterDouble& filterDouble,
  const double sampleRate,
  vector<float>& pos)
{
  Filter::integrateFloat(accelFloat, static_cast<float>(sampleRate), 
    true, startInterval, lenInterval, synthSpeed);

  Filter::highpass(filterDouble, synthSpeed);

  Filter::integrateFloat(synthSpeed, static_cast<float>(sampleRate), 
    false, 0, lenInterval, pos);

  Filter::highpass(filterDouble, pos);
}


void Filter::process(
  const double sampleRate,
  const unsigned start,
  const unsigned len)
{
  // TODO 
  // * Don't use exact comparison with 2000.
  // * Keep the original signal as well?
  // * Flag in Control if attempting to write speed (now gone).
  // * Flag same in member function below.
  // * No double, Double, DOUBLE anywhere in the program.

  if (sampleRate != 2000.)
    THROW(ERR_UNKNOWN_SAMPLE_RATE, "Unknown sample rate");

  startInterval = start;
  lenInterval = len;

  synthSpeed.resize(lenInterval);
  synthPos.resize(lenInterval);

  // First we low-pass filter the acceleration in order to remove
  // some of the jitter.
  Filter::filterFloat(Butterworth5LPF_float, accelFloat);

  // Cancel out one zero at DC in the first section (for example).
  vector<FilterFloat> Butterworth5HPF_SOS_integr = 
    Butterworth5HPF_SOS_float;
  Filter::makeIntegratingFilter(Butterworth5HPF_SOS_float[0],
    Butterworth5HPF_SOS_integr[0]);

  // Cancel out one zero at DC in the first section (for example).
  // vector<FilterDouble> Butterworth5HPF_SOS_integr_double = 
    // Butterworth5HPF_SOS_double;

  /*
  vector<FilterDouble> Butterworth5HPF_SOS_integr_double;
  Butterworth5HPF_SOS_integr_double.resize(3);
  Butterworth5HPF_SOS_integr_double[0] = Butterworth5HPF_SOS_double[1];
  Butterworth5HPF_SOS_integr_double[1] = Butterworth5HPF_SOS_double[0];
  Butterworth5HPF_SOS_integr_double[2] = Butterworth5HPF_SOS_double[2];

  Filter::makeIntegratingFilterDouble(Butterworth5HPF_SOS_double[0],
    Butterworth5HPF_SOS_integr_double[0]);
    */

  // vector<float> synthPosNew;
  // Filter::runIntegratingFilterDouble(Butterworth5HPF_SOS_integr_double,
    // sampleRate, synthPos);

  // vector<float> synthPosNew;
  // Filter::runIntegratingFilterFloat(Butterworth5HPF_SOS_integr, sampleRate,
    // synthPos);

  Filter::runIntegrationSeparately(Butterworth5HPF_double, sampleRate,
    synthPos);

/*
cout << "COMPARISON\n";
for (unsigned i = 0; i < synthPos.size(); i++)
{
  cout << i << ";" << fixed << setprecision(4) << synthPos[i] << ";" <<
    fixed << setprecision(4) << synthPosNew[i] << "\n";
}
*/

/*
   Effects of new, simpler float filter:
   - 24 fewer correct traces.
     
   - 20 more exceptions.
     sensor08: 0 -> 4 (all correct traces)
     sensor10: 2 -> 5 (all correct traces)
     sensor13: 0 -> 8 (all correct, plus two new errors)
     sensor19: 8 -> 17 (mostly errors anyway)
     sensor22: 8 -> 16 (mostly correct traces)
     sensor24: 28 -> 32 (lose 7 correct traces)
     sensor25: 24 -> 18 (so "better", but still wrong)
     sensor31: 16 -> 8 (all correct, so good)
     sensor32: 31 -> 28 (all correct, so good)
*/
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

