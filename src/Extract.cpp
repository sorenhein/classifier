#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <math.h>

#include "Extract.h"
#include "read.h"

// Once the early samples on the wake-up start changing by this
// factor, we are on the exponential part of the wake-up.

#define INITIAL_FACTOR 1.20

// Two points at which to fix the transient.

#define FIRST_CROSSING 1.00
#define SECOND_CROSSING 2.00

// Default for the mid voltage of the measurement.

#define MID_LEVEL 2.50


Extract::Extract()
{
}


Extract::~Extract()
{
}


unsigned Extract::findCrossing(const double level) const
{
  for (unsigned i = firstActiveSample; i < samples.size(); i++)
  {
    if (samples[i] > level)
      return i;
  }
  return 0;
}


bool Extract::processTransient()
{
  // This is mainly to check that there are no peaks in the transient.

  const unsigned l = samples.size();
  bool found = false;
  for (unsigned i = 1; i < l; i++)
  {
    if (samples[i] > INITIAL_FACTOR * samples[i-1])
    {
      found = true;
      firstActiveSample = i-1;
      break;
    }
  }

  if (! found)
    return false;

  const unsigned n1 = findCrossing(FIRST_CROSSING);
  if (n1 == 0)
    return false;

  const unsigned n2 = findCrossing(SECOND_CROSSING);
  if (n2 == 0)
    return false;

  // Assume the transient is of the form
  // avg * (1 - exp(-(i-firstActiveSample) / tau)

  const double nd1 = static_cast<double>(n1 - firstActiveSample);
  const double nd2 = static_cast<double>(n2 - firstActiveSample);

  unsigned iter = 0;
  double err = 1.e6, avg = MID_LEVEL, tau = 0.;
  while (iter < 10 && err > 1.e-6)
  {
    const double tau1 = nd1 / log(avg / (avg - samples[n1]));
    const double tau2 = nd2 / log(avg / (avg - samples[n2]));
    tau = 0.5 * (tau1 + tau2);

    const double avg1 = samples[n1] / (1. - exp(-nd1 / tau));
    const double avg2 = samples[n2] / (1. - exp(-nd2 / tau));
    avg = 0.5 * (avg1 + avg2);

    err = (avg1 > avg2 ? avg1-avg2 : avg2-avg1);
    iter++;
  }

  if (err > 1.e-6)
    return false;

  midLevel = avg;
  timeConstant = tau;
  return true;
}


bool Extract::read(const string& fname)
{
  // TODO Make something of the file name

  ifstream fin;
  fin.open(fname);
  string line;
  double v;
  while (getline(fin, line))
  {
    if (line == "" || line.front() == '#')
      continue;

    // The format seems to have a trailing comma.
    if (line.back() == ',')
      line.pop_back();

    const string err = "File " + fname + ": Bad line '" + line + "'"; 
    if (! readDouble(line, v, err))
      break;

    samples.push_back(v);
  }

  fin.close();

  Extract::processTransient();
  return true;
}


void Extract::printStats() const
{
}

