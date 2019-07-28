#include <iostream>
#include <iomanip>
#include <iterator>
#include <fstream>
#include <sstream>

#include "Trace.h"

#include "database/Control.h"

#include "stats/Timers.h"

#include "util/io.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

extern vector<Timers> timers;


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


/*
bool Trace::readBinary(const string& filenameFull)
{
  vector<float> fsamples;
  if (! readBinaryTrace(filenameFull, fsamples))
    return false;

  // TODO Leave samples as floats?
  samples.resize(fsamples.size());
  for (unsigned i = 0; i < fsamples.size(); i++)
    samples[i] = fsamples[i];
  return true;
}
*/


void Trace::printSamples(const string& title) const
{
  cout << title << "\n";
  for (unsigned i = 0; i < samples.size(); i++)
    cout << i << ";" << samples[i] << "\n";
  cout << "\n";
}


/*
void Trace::read(
  const string& filenameFull,
  const unsigned thid)
{
  timers[thid].start(TIMER_READ);

  samples.clear();
  Trace::readBinary(filenameFull);

  timers[thid].stop(TIMER_READ);
}
*/


void Trace::detect(
  const Control& control,
  const double sampleRate,
  const vector<float>& fsamples,
  const unsigned thid,
  Imperfections& imperf)
{
  timers[thid].start(TIMER_TRANSIENT);

  samples.resize(fsamples.size());
  for (unsigned i = 0; i < fsamples.size(); i++)
    samples[i] = fsamples[i];

  runs.clear();
  Trace::calcRuns();
  transientFlag = transient.detect(samples, runs);

  Interval intAfterTransient;
  intAfterTransient.first = transient.lastSampleNo();
  intAfterTransient.len = samples.size() - intAfterTransient.first;

  Interval intAfterBack;
  quietFlag = quietBack.detect(samples, intAfterTransient, 
    QUIET_BACK, intAfterBack);

  Interval intAfterFront;
  (void) quietFront.detect(samples, intAfterBack, 
    QUIET_FRONT, intAfterFront);
  timers[thid].stop(TIMER_TRANSIENT);

  if (control.verboseTransient())
    cout << transient.str() << "\n";

  (void) segActive.detect(samples, sampleRate, intAfterFront, 
    control, thid, imperf);
}


void Trace::logPeakStats(
  const vector<double>& posTrue,
  const string& trainTrue,
  const double speedTrue,
  PeakStats& peakStats)
{
  segActive.logPeakStats(posTrue, trainTrue, speedTrue, peakStats);
}


bool Trace::getAlignment(
  vector<double>& times,
  vector<int>& actualToRef,
  unsigned& numFrontWheels)
{
  return segActive.getAlignment(times, actualToRef, numFrontWheels);
}


void Trace::getTrace(
  vector<double>& times,
  unsigned& numFrontWheels) const
{
  segActive.getPeakTimes(times, numFrontWheels);

cout << "Got " << times.size() << " peaks, " <<
  numFrontWheels << " front wheels\n\n";
}


string Trace::strTransientHeaderCSV()
{
  return transient.headerCSV();
}


string Trace::strTransientCSV()
{
  return transient.strCSV();
}


void Trace::write(
  const Control& control,
  const string& filename,
  const unsigned thid) const
{
  timers[thid].start(TIMER_WRITE);

  if (control.writeTransient())
    transient.writeFile(control.transientDir() + "/" + filename);
  if (control.writeBack())
    quietBack.writeFile(control.backDir() + "/" + filename);
  if (control.writeFront())
    quietFront.writeFile(control.frontDir() + "/" + filename);
  if (control.writeSpeed())
    segActive.writePeak(control.speedDir() + "/" + filename);
  if (control.writePos())
    segActive.writePeak(control.posDir() + "/" + filename);
  if (control.writePeak())
    segActive.writePeak(control.peakDir() + "/" + filename);

  timers[thid].stop(TIMER_WRITE);
}

