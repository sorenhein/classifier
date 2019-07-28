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


void Trace::detect(
  const Control& control,
  const double sampleRate,
  const vector<float>& fsamples,
  const unsigned thid,
  Imperfections& imperf)
{
  timers[thid].start(TIMER_TRANSIENT);

  transientFlag = transient.detect(fsamples);

  Interval intAfterTransient;
  intAfterTransient.first = transient.lastSampleNo();
  intAfterTransient.len = fsamples.size() - intAfterTransient.first;

  // TODO Leave as floats for a while longer.
  samples.resize(fsamples.size());
  for (unsigned i = 0; i < fsamples.size(); i++)
    samples[i] = fsamples[i];

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

