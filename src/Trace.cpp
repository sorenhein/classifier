#include <iostream>
#include <iomanip>
#include <iterator>
#include <fstream>
#include <sstream>

#include "Trace.h"
#include "Timers.h"
#include "read.h"

extern Timers timers;


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


bool Trace::readText()
{
  return readTextTrace(filename, samples);
}


bool Trace::readBinary()
{
  vector<float> fsamples;
  if (! readBinaryTrace(filename, fsamples))
    return false;

  // TODO Leave samples as floats?
  samples.resize(fsamples.size());
  for (unsigned i = 0; i < fsamples.size(); i++)
    samples[i] = fsamples[i];
  return true;
}


void Trace::printSamples(const string& title) const
{
  cout << title << "\n";
  for (unsigned i = 0; i < samples.size(); i++)
    cout << i << ";" << samples[i] << "\n";
  cout << "\n";
}


void Trace::read(
  const string& fname,
  const bool binaryFlag)
{
  timers.start(TIMER_READ);

  filename = fname;

  samples.clear();
  if (binaryFlag)
    Trace::readBinary();
  else
     Trace::readText();

  timers.stop(TIMER_READ);
}


void Trace::detect(const Control& control)
{
  timers.start(TIMER_TRANSIENT);
  runs.clear();
  Trace::calcRuns();
  transientFlag = transient.detect(samples, runs);
  timers.stop(TIMER_TRANSIENT);

  timers.start(TIMER_ENDS);
  Interval intAfterTransient;
  intAfterTransient.first = transient.lastSampleNo();
  intAfterTransient.len = samples.size() - intAfterTransient.first;

  Interval intAfterBack;
  quietFlag = quietBack.detect(samples, intAfterTransient, 
    QUIET_BACK, intAfterBack);

  Interval intAfterFront;
  (void) quietFront.detect(samples, intAfterBack, 
    QUIET_FRONT, intAfterFront);
  timers.stop(TIMER_ENDS);

  if (control.verboseTransientMatch)
    cout << transient.str() << "\n";

  (void) segActive.detect(samples, intAfterFront);
}


void Trace::getTrace(vector<PeakTime>& times) const
{
  segActive.getPeakTimes(times);

cout << "Got " << times.size() << " peaks\n\n";
}


string Trace::strTransientHeaderCSV()
{
  return transient.headerCSV();
}


string Trace::strTransientCSV()
{
  return transient.strCSV();
}


void Trace::write(const Control& control) const
{
  timers.start(TIMER_WRITE);

  if (control.writingTransient)
    transient.writeFile(filename, "transient");
  if (control.writingBack)
    quietBack.writeFile(filename, "back");
  if (control.writingFront)
    quietFront.writeFile(filename, "front");
  if (control.writingSpeed)
    segActive.writeSpeed(filename, "speed");
  if (control.writingPos)
    segActive.writePos(filename, "pos");
  if (control.writingPeak)
    segActive.writePeak(filename, "peak");
  if (control.writingOutline)
  {
    cout << "Not yet implemented\n";
    // TODO
  }

  timers.stop(TIMER_WRITE);
}


