#include <iostream>
#include <iomanip>
#include <iterator>
#include <fstream>
#include <sstream>
#include <math.h>

#include "Trace.h"
#include "read.h"

// Once the early samples on the wake-up start changing by this
// factor, we are on the exponential part of the wake-up.

#define INITIAL_FACTOR 1.20

// Two points at which to fix the transient.

#define FIRST_CROSSING 1.00
#define SECOND_CROSSING 2.00

// Default for the mid voltage of the measurement.

// #define MID_LEVEL 2.50
// #define MID_RANGE 0.10
#define MID_LEVEL 0.00
#define MID_RANGE 1.00
#define MID_HYSTERESIS 0.000

#define NUM_RUNS_BACK 1
#define NUM_RUNS_FRONT 1


Trace::Trace()
{
}


Trace::~Trace()
{
}


unsigned Trace::findCrossing(const double level) const
{
  for (unsigned i = firstActiveSample; i < samples.size(); i++)
  {
    if (samples[i] > level)
      return i;
  }
  return 0;
}


bool Trace::processTransient()
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


bool Trace::skipTransient()
{
  for (unsigned i = 0; i < samples.size(); i++)
  {
    // if (samples[i] > MID_LEVEL)
    if (samples[i] < MID_LEVEL + MID_RANGE &&
        samples[i] > MID_LEVEL - MID_RANGE)
    {
      firstActiveSample = i;
      return true;
    }
  }
  return false;
}


bool Trace::calcAverage()
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


void Trace::calcRuns(vector<Run>& runs) const
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


bool Trace::runsToBumps(
  const vector<Run>& runs,
  vector<Run>& bumps) const
{
  const unsigned lr = runs.size();

  vector<double> timelist;
  timelist.resize(samples.size());
  for (unsigned i = 0; i < firstActiveSample; i++)
    timelist[i] = 0.;

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
      timelist[j] = factor * value;

    bumps[i] = runs[i];
    bumps[i].cum = factor * value;
  }

  /* */
  cout << "Times\n";
  for (unsigned i = 0; i < times.size(); i++)
    cout << i << ";" << timelist[i] << endl;
  cout << "\n";
  /* */

  cout << "Bumps\n";
  for (unsigned i = 0; i < bumps.size(); i++)
    cout << i << ";" << bumps[i].len << ";" << bumps[i].cum << endl;
  cout << "\n";

  return true;
}


#define HISTO_LONG_MAX 250
#define HISTO_TALL_MAX 1001
#define HISTO_TALL_WIDTH 2.00
#define CUTOFF_LONG 0.95
#define CUTOFF_TALL 0.9

void Trace::tallyBumps(
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
      cout << "Memory long exceeded: " << bumps[i].len << endl;
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



bool Trace::readText()
{
  ifstream fin;
  fin.open(filename);
  string line;
  double v;
  while (getline(fin, line))
  {
    if (line == "" || line.front() == '#')
      continue;

    // The format seems to have a trailing comma.
    if (line.back() == ',')
      line.pop_back();

    const string err = "File " + filename + 
      ": Bad line '" + line + "'"; 
    if (! readDouble(line, v, err))
    {
      fin.close();
      return false;
    }
    samples.push_back(v);
  }

  fin.close();
  return true;
}


bool Trace::readBinary()
{
  ifstream fin(filename, std::ios::binary);
  fin.unsetf(std::ios::skipws);
  fin.seekg(0, std::ios::end);
  const unsigned filesize = static_cast<unsigned>(fin.tellg());
  fin.seekg(0, std::ios::beg);
  samples.resize(filesize/4);

vector<float> ff;
ff.resize(filesize/4);
  fin.read(reinterpret_cast<char *>(ff.data()),
    ff.size() * sizeof(float));
  fin.close();
for (unsigned i = 0; i < filesize/4; i++)
  samples[i] = ff[i];
  return true;
}


#define HISTO_PEAK_MAX 100.
#define HISTO_PEAK_BIN 1.
#define HISTO_PEAK_NUM_BINS 101
#define THRESHOLD_CUTOFF 0.25
#define TOPS_TO_AVERAGE 10

bool Trace::thresholdPeaks()
{
  // Looks for negative (physically, upward) peaks, as these seem to 
  // be cleaner.
  vector<unsigned> histo;
  histo.resize(HISTO_PEAK_NUM_BINS);
  for (unsigned i = 0; i < HISTO_PEAK_NUM_BINS; i++)
    histo[i] = 0;

  const unsigned ls = samples.size();
  unsigned c = 0;
  for (unsigned i = 0; i < ls; i++)
  {
    if (samples[i] >= average)
      continue;
    const unsigned p = static_cast<unsigned>
    ((average - samples[i]) / HISTO_PEAK_BIN);
    if (p >= HISTO_PEAK_NUM_BINS)
    {
      cout << "Got " << p << "\n";
    }
    histo[p]++;
    c++;
  }

  if (c < TOPS_TO_AVERAGE)
  {
    cout << "No positive acceleration in trace.\n";
    return false;
  }

  unsigned sum = 0;
  unsigned count = 0;
  for (unsigned i = 0; i < HISTO_PEAK_NUM_BINS; i++)
  {
    const unsigned irev = HISTO_PEAK_NUM_BINS - 1 - i;
    count += histo[irev];
    sum += histo[irev] * irev;
    if (count >= TOPS_TO_AVERAGE)
      break;
  }

  threshold = (THRESHOLD_CUTOFF * sum) / count;

cout << "Number " << ls << endl;
cout << "count " << count << " sum " << sum << endl;
cout << "threshold " << threshold << endl;

  for (unsigned i = 1; i < ls-1; i++)
  {
    if (samples[i] > threshold &&
        samples[i] > samples[i-1] &&
        samples[i] > samples[i+1])
    {
      PeakTime peak;
      peak.time = i / 2000.;
      peak.value = samples[i];

      if (times.size() == 0)
      {
        times.push_back(peak);
        continue;
      }

      // Ignore close, lower peaks.  10 ms is 1 meter even at 360 km/h.
      PeakTime& last = times.back();
      if (peak.time > last.time + 0.03)
      {
        // Quite far spaced out.
        times.push_back(peak);
      }
      else if (peak.time > last.time + 0.01)
      {
        // Somewhat close in time.
        if (peak.value > 2. * threshold &&
            peak.value > 0.75 * last.value)
        {
          // Large enough to count.
          if (last.value <= 2. * threshold &&
              peak.value > 2. * last.value)
          {
            // Previous peak was close and rather weak.
            last = peak;
          }
          else
          {
            // Stands on its own.
            times.push_back(peak);
          }
        }
      }
      else if (peak.value > last.value)
      {
        // So close that we always replace the earlier peak.
        last = peak;
      }
    }
  }
cout << "peaks " << times.size() << "\n";
  return true;
}


bool Trace::read(const string& fname)
{
  // TODO Make something of the file name

  samples.clear();
  times.clear();

  filename = fname;
  Trace::readBinary();
  // Trace::readText();

/* */
for (unsigned i = 0; i < samples.size(); i++)
  cout << i << ";" << samples[i] << "\n";
cout << "\n";
/* */

  // Trace::processTransient();

  if (! Trace::skipTransient())
  {
    cout << "Couldn't skip transient\n";
    return false;
  }
cout << "firstActiveSample " << firstActiveSample << endl;

  // if (! Trace::calcAverage())
  // {
    // cout << "Couldn't find a good average, " << average << "\n";
//  return false;
  // }
// cout << "average " << average << endl;
average = 0.;

  Trace::thresholdPeaks();

  /*
  const unsigned ls = times.size();
  if (ls == 0)
  {
    cout << "No peaks\n";
    return false;
  }

  cout << "Peaks " << ls << "\n";
  for (unsigned i = 0; i < ls; i++)
  {
    cout << i << " " << 
      static_cast<unsigned>(2000. * times[i].time + 0.5) << " " <<
      times[i].value << "\n";
  }
  */

/*
  // Eliminate spurious peaks in short intervals.
  // Half as the upper limit of "closeness".
  const double halfDist = 2000. * (shortCluster.median / 2.);

cout << "Short cluster " << shortCluster.count <<
  " " << shortCluster.lower << " " << shortCluster.upper << 
  " " << shortCluster.median << 
  " " << halfDist << "\n\n";


  for (unsigned i = ls-1; i > 0; i--)
  {
    if (times[i].time <= times[i-1].time + halfDist &&
        times[i].value > times[i-1].value &&
        times[i].value > -2. * threshold)
    {
      // Close to predecessor in time, smaller and relative small.
      times.erase(times.begin() + i);
    }
  }

  cout << "Pruned peaks " << times.size() << "\n";
  for (unsigned i = 0; i < times.size(); i++)
  {
    cout << i << " " << 
      static_cast<unsigned>(2000. * times[i].time + 0.5) << " " <<
      times[i].value << "\n";
  }
*/



  /*
  vector<Run> runs;
  Trace::calcRuns(runs);

  vector<Run> bumps;
  Trace::runsToBumps(runs, bumps);

  unsigned longBump;
  double tallBump;

  Trace::tallyBumps(bumps, longBump, tallBump);

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
  */

  return true;
}


void Trace::getTrace(vector<PeakTime>& timesOut) const
{
  // TODO Inefficient
  timesOut = times;
}


void Trace::printStats() const
{
}

