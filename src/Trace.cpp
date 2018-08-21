#include <iostream>
#include <iomanip>
#include <iterator>
#include <fstream>
#include <sstream>
#include <math.h>

#include "Trace.h"
#include "read.h"

#define NUM_RUNS_BACK 1
#define NUM_RUNS_FRONT 1


Trace::Trace()
{
}


Trace::~Trace()
{
}



void Trace::calcRuns()
{
  Run run;
  run.first = 0;
  run.len = 1;
  if (samples[0] >= 0.)
  {
    run.posFlag = true;
    run.cum = samples[0];
  }
  else
  {
    run.posFlag = false;
    run.cum = -samples[0];
  }

  const unsigned l = samples.size();
  for (unsigned i = 1; i < l; i++)
  {
    if (run.posFlag)
    {
      if (samples[i] >= 0.)
      {
        run.len++;
        run.cum += samples[i];
      }
      else
      {
        runs.push_back(run);
        run.first = i;
        run.len = 1;
        run.posFlag = false;
        run.cum = -samples[i];
      }
    }
    else if (samples[i] < 0.)
    {
      run.len++;
      run.cum -= samples[i];
    }
    else
    {
      runs.push_back(run);
      run.first = i;
      run.len = 1;
      run.posFlag = true;
      run.cum = samples[i];
    }
  }
  runs.push_back(run);
}


#define DECLINE_FACTOR 1.0

void Trace::combineRuns(
  vector<Run>& runvec,
  const double thr) const
{
  vector <Run> tmp;
  tmp.clear();

  const unsigned l = runvec.size();
  for (unsigned i = 0; i < l; i++)
  {
    if (i+2 >= l)
    {
      tmp.push_back(runvec[i]);
      continue;
    }

    Run run = runvec[i];

    while (i+2 < l)
    {
      if (runvec[i+1].cum < thr &&
         (runvec[i].cum < thr || runvec[i+2].cum < thr) &&
          DECLINE_FACTOR * runvec[i].cum > runvec[i+1].cum &&
          DECLINE_FACTOR * runvec[i+2].cum > runvec[i+1].cum)
      {
        // Consider this a spurious bump.
        run.len += runvec[i+1].len + runvec[i+2].len;
        run.cum += - runvec[i+1].cum + runvec[i+2].cum;
        i += 2;
      }
      else
        break;
    }

    tmp.push_back(run);
  }
  runvec = tmp;
}


bool Trace::runsToBumps(vector<Run>& bumps) const
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
  // Used to be global: Kludge TODO
  
  double threshold = 0.;

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
    if (samples[i] >= 0.)
      continue;
    const unsigned p = static_cast<unsigned>
    ((- samples[i]) / HISTO_PEAK_BIN);
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


void Trace::printSamples(const string& title) const
{
  cout << title << "\n";
  for (unsigned i = 0; i < samples.size(); i++)
    cout << i << ";" << samples[i] << "\n";
  cout << "\n";
}


void Trace::printFirstRuns(
  const string& title,
  const vector<Run>& runvec,
  const unsigned num) const
{
  cout << title << "\n";
  for (unsigned i = 0; i < num; i++)
  {
    cout << i << " " << setw(6) << left << runvec[i].first << " " <<
      setw(6) << runvec[i].len << " " <<
      runvec[i].posFlag << " " <<
      setw(12) << fixed << setprecision(2) << right <<
        runvec[i].cum << endl;
  }
}


void Trace::printRunsAsVector(
  const string& title,
  const vector<Run>& runvec) const
{
  cout << title << "\n";
  const unsigned l = runvec.size();
  for (unsigned i = 0; i < l; i++)
  {
    const double v = (runvec[i].posFlag ? runvec[i].cum :
      -runvec[i].cum);
    for (unsigned j = runvec[i].first;
        j < runvec[i].first + runvec[i].len; j++)
    {
      cout << j << ";" << v << "\n";
    }
  }
}


#define THRESHOLD_WIDTH 0.1
#define THRESHOLD_SIZE 1001

double Trace::calcThreshold(
  const vector<Run>& runvec,
  const unsigned num) const
{
  vector<unsigned> histo(THRESHOLD_SIZE);

cout << "Histo start" << endl;
  const unsigned l = runvec.size();
  for (unsigned i = 0; i < l; i++)
  {
    double r = runvec[i].cum;
    const double d = (r >= 0. ? r : -r);
    const unsigned j = static_cast<unsigned>(d / THRESHOLD_WIDTH);

    if (j >= THRESHOLD_SIZE)
    {
      cout << "Memory long exceeded: " << j << endl;
      continue;
    }

    histo[j]++;
  }

cout << "\n";
for (unsigned i = 0; i < THRESHOLD_SIZE; i++)
{
  if (histo[i] > 0)
    cout << i << " " << histo[i] << "\n";
}

cout << "Histo thr" << endl;
  unsigned c = 0;
  for (unsigned i = 0; i < THRESHOLD_SIZE; i++)
  {
    const unsigned j = THRESHOLD_SIZE - 1 - i;
    c += histo[j];

    if (c >= num)
{
  cout << "Thr found " << j << endl;
      return j * THRESHOLD_WIDTH;
}
  }

  cout << "No threshold found\n";
  return 0.;
}


bool Trace::read(const string& fname)
{
  // TODO Make something of the file name

  samples.clear();
  times.clear();

  filename = fname;
  Trace::readBinary();
  // Trace::readText();

  runs.clear();
  Trace::calcRuns();

  transientFlag = transient.detect(samples, runs);

  vector<Interval> available(1);
  available[0].first = transient.lastSampleNo();
  available[0].len = samples.size() - available[0].first;

  vector<Interval> active;
  quietFlag = quietBack.detect(samples, available, 
    QUIET_BACK, active);

  vector<Interval> active2;
  (void) quietFront.detect(samples, active, QUIET_FRONT, active2);

  /*
  for (unsigned i = 0; i < active2.size(); i++)
  {
    cout << i << " " << active2[i].first << " " << active2[i].len << "\n";
  }
  */

  vector<Interval> active3;
  (void) quietIntra.detect(samples, active2, QUIET_INTRA, active3);


cout << "firstActiveSample " << firstActiveSample << endl;
return true;


  cout << "\n";
  Trace::printRunsAsVector("Original", runs);

  const double threshold = Trace::calcThreshold(runs, 200);
  cout << "Threshold " << threshold << endl;

  unsigned l;
  unsigned i = 0;
  do
  {
    l = runs.size();
    Trace::combineRuns(runs, threshold);

    if (l > runs.size())
    {
      cout << "\n";
      Trace::printRunsAsVector("Iter " + to_string(i) + 
        " " + to_string(runs.size()), runs);
    }
    i++;
    if (i > 10)
      break;
  }
  while (l > runs.size());

  /*
  cout << "\n";
  cout << "Combiruns\n";
  const unsigned l = combinedRuns.size();
  for (unsigned i = 0; i < l; i++)
  {
    if (i < l-1 && combinedRuns[i].posFlag)
    {
      cout << combinedRuns[i].len << ";" <<
        combinedRuns[i+1].len << "\n";
    }
  }
  */

  // cout << "\n";
  // Trace::printRunsAsVector("CombiRuns", combinedRuns);
  // cout << "\n";

  // Trace::printSamples("samples");

  // Trace::printFirstRuns("Leadruns", 5);

  // Trace::thresholdPeaks();

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


string Trace::strTransientHeaderCSV()
{
  return transient.headerCSV();
}


string Trace::strTransientCSV()
{
  return transient.strCSV();
}


void Trace::printStats() const
{
}


void Trace::writeTransient() const
{
  transient.writeBinary(filename);
}


void Trace::writeQuietBack() const
{
  quietBack.writeBinary(filename, "back");
}


void Trace::writeQuietFront() const
{
  quietFront.writeBinary(filename, "front");
}


void Trace::writeQuietIntra() const
{
  quietIntra.writeBinary(filename, "intra");
}

