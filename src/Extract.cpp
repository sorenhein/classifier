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
#define MID_RANGE 0.10
#define MID_HYSTERESIS 0.000

#define NUM_RUNS_BACK 1
#define NUM_RUNS_FRONT 1


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


bool Extract::skipTransient()
{
  for (unsigned i = 0; i < samples.size(); i++)
  {
    if (samples[i] > MID_LEVEL)
    {
      firstActiveSample = i;
      return true;
    }
  }
  return false;
}


bool Extract::calcAverage()
{
  const unsigned l = samples.size();
  double sum = 0.;
  for (unsigned i = firstActiveSample; i < l; i++)
    sum += samples[i];
  average = sum / static_cast<double>(l - firstActiveSample);
  
  if (average < MID_LEVEL - MID_RANGE || average > MID_LEVEL + MID_RANGE)
    return false;
  else
    return true;
}


void Extract::calcRuns(vector<Run>& runs) const
{
  Run run;

  run.first = firstActiveSample;
  run.len = 1;
  if (samples[firstActiveSample] >= average)
  {
    run.posFlag = true;
    run.cum = samples[firstActiveSample] - average;
  }
  else
  {
    run.posFlag = false;
    run.cum = average - samples[firstActiveSample];
  }

  const unsigned l = samples.size();
  for (unsigned i = firstActiveSample; i < l; i++)
  {
    if (run.posFlag)
    {
      if (samples[i] > average - MID_HYSTERESIS)
      {
        run.len++;
        run.cum += samples[i] - average;
      }
      else
      {
        runs.push_back(run);
        run.first = i;
        run.len = 1;
        run.posFlag = false;
        run.cum = average - samples[i];
      }
    }
    else if (samples[i] < average + MID_HYSTERESIS)
    {
      run.len++;
      run.cum += average - samples[i];
    }
    else
    {
      runs.push_back(run);
      run.first = i;
      run.len = 1;
      run.posFlag = true;
      run.cum = samples[i] - average;
    }
  }
  runs.push_back(run);
}


bool Extract::runsToBumps(
  const vector<Run>& runs,
  vector<Run>& bumps) const
{
  const unsigned lr = runs.size();

  vector<double> times;
  times.resize(samples.size());
  for (unsigned i = 0; i < firstActiveSample; i++)
    times[i] = 0.;

  bumps.resize(lr);

  for (unsigned i = 0; i < lr; i++)
  {
    const unsigned lower = (i >= NUM_RUNS_BACK ? i-NUM_RUNS_BACK : 0);
    const unsigned upper = (i+NUM_RUNS_FRONT < lr ? i+NUM_RUNS_BACK : lr-1);
    double value = 0.;
    for (unsigned j = lower; j <= upper; j++)
      value += runs[j].cum;

    const double factor = (runs[i].posFlag ? 1. : -1.);
    for (unsigned j = runs[i].first; j < runs[i].first + runs[i].len; j++)
      times[j] = factor * value;

    bumps[i] = runs[i];
    bumps[i].cum = factor * value;
  }

  /*
  cout << "Times\n";
  for (unsigned i = 0; i < times.size(); i++)
    cout << i << ";" << times[i] << endl;
  cout << "\n";
  */

  cout << "Bumps\n";
  for (unsigned i = 0; i < bumps.size(); i++)
    cout << i << ";" << bumps[i].len << " " << bumps[i].cum << endl;
  cout << "\n";

  return true;
}


#define HISTO_LONG_MAX 100
#define HISTO_TALL_MAX 501
#define HISTO_TALL_WIDTH 0.01
#define CUTOFF_LONG 0.95
#define CUTOFF_TALL 0.9

void Extract::tallyBumps(
  const vector<Run>& bumps, 
  unsigned& longRun, 
  double& tallRun) const
{
  vector<unsigned> histoLong(HISTO_LONG_MAX);
  vector<unsigned> histoTall(HISTO_TALL_MAX);

  const unsigned lr = bumps.size();
  for (unsigned i = 0; i < lr; i++)
  {
    if (bumps[i].len >= HISTO_LONG_MAX)
    {
      cout << "Memory long exceeded: " << i << endl;
      continue;
    }

    const double d = (bumps[i].cum >= 0. ? bumps[i].cum : -bumps[i].cum);
    const unsigned j = static_cast<unsigned>(d / HISTO_TALL_WIDTH);

    if (j >= HISTO_TALL_MAX)
    {
      cout << "Memory tall exceeded: " << j << endl;
      continue;
    }
    
    histoLong[bumps[i].len]++;
    histoTall[j]++;
  }

  unsigned c = 0;
  for (unsigned i = 0; i < HISTO_LONG_MAX; i++)
  {
    c += histoLong[i];
    if (c >= CUTOFF_LONG * lr)
    {
      longRun = i;
      break;
    }
  }

  c = 0;
  for (unsigned i = 0; i < HISTO_TALL_MAX; i++)
  {
    c += histoTall[i];
    if (c >= CUTOFF_TALL * lr)
    {
      tallRun = i * HISTO_TALL_WIDTH;
      break;
    }
  }


  cout << "Long\n";
  for (unsigned i = 0; i < HISTO_LONG_MAX; i++)
  {
    if (histoLong[i] > 0)
      cout << i << " " << histoLong[i] << "\n";
  }
  cout << "longRun " << longRun << "\n\n";

  cout << "Tall\n";
  for (unsigned i = 0; i < HISTO_TALL_MAX; i++)
  {
    if (histoTall[i] > 0)
      cout << (i * HISTO_TALL_WIDTH) << " " << 
      histoTall[i] << "\n";
  }
  cout << "tallRun " << tallRun << "\n\n";
}


bool Extract::read(const string& fname)
{
  // TODO Make something of the file name

  filename = fname;
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

for (unsigned i = 0; i < samples.size(); i++)
  cout << i << " " << samples[i] << "\n";
cout << "\n";

  // Extract::processTransient();

  if (! Extract::skipTransient())
  {
    cout << "Couldn't skip transient\n";
    return false;
  }
cout << "firstActiveSample " << firstActiveSample << endl;

  if (! Extract::calcAverage())
  {
    cout << "Couldn't find a good average\n";
    return false;
  }
cout << "average " << average << endl;

  vector<Run> runs;
  Extract::calcRuns(runs);

  vector<Run> bumps;
  Extract::runsToBumps(runs, bumps);

  unsigned longBump;
  double tallBump;

  Extract::tallyBumps(bumps, longBump, tallBump);

  cout << "Candidate peaks\n";
  unsigned num = 0;
  const unsigned lr = runs.size();
  for (unsigned i = 1; i < lr-2; i++)
  {
    // The middle of three tall bumps (pos-neg-pos), with lengths
    // long-long-short (from back to front).
    if ((bumps[i+2].len < longBump || bumps[i+1].len < longBump) &&
        bumps[i].len >= longBump && 
        bumps[i-1].len >= longBump &&
        bumps[i+1].cum >= tallBump && 
        bumps[i].cum <= -tallBump && 
        bumps[i-1].cum >= tallBump)
    {
      /*
      if (i < lr-2 &&
        bumps[i+2].len >= longBump && bumps[i+1].len >= longBump &&
        bumps[i+2].cum <= -tallBump && bumps[i+1].cum >= tallBump &&
        bumps[i+1].cum - bumps[i+2].cum >
        bumps[i-1].cum - bumps[i].cum)
      {
        // Next bump is even bigger.
        continue;
      }
      else if (i >= 2 &&
        bumps[i-2].len >= longBump && bumps[i-3].len >= longBump &&
        bumps[i-2].cum <= -tallBump && bumps[i-3].cum >= tallBump &&
        bumps[i-3].cum - bumps[i-2].cum >
        bumps[i-1].cum - bumps[i].cum)
      {
        // Previous bump was even bigger.
        continue;
      }
      */

      cout << bumps[i-1].first << ", " <<
        bumps[i-1].len << ", " <<
        bumps[i-1].posFlag << ", " <<
        bumps[i-1].cum << "\n";

      cout << bumps[i].first << ", " <<
        bumps[i].len << ", " <<
        bumps[i].posFlag << ", " <<
        bumps[i].cum << "\n\n";

      num++;
    }
  }
  cout << "Counted " << num << "\n\n";

  return true;
}


void Extract::printStats() const
{
}

