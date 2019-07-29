#include <iostream>
#include <sstream>
#include <limits>

#include "transient/Transient.h"
#include "transient/Quiet.h"

#include "PeakDetect.h"

#include "database/TraceDB.h"
#include "database/TrainDB.h"
#include "database/Control.h"

#include "filter/Filter.h"

#include "stats/CompStats.h"
#include "stats/PeakStats.h"
#include "stats/Timers.h"

#include "Regress.h"
#include "Align.h"
#include "Except.h"
#include "errors.h"

#include "geometry.h"
#include "run.h"

#include "util/io.h"
#include "util/Motion.h"


using namespace std;

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


extern CompStats sensorStats;
extern CompStats trainStats;
extern PeakStats peakStats;

extern vector<Timers> timers;


unsigned lookupMatchRank(
  const TrainDB& trainDB,
  const vector<Alignment>& matches,
  const string& tag);


string runHeader(const TraceData& traceData);

void runRead(
  const string& filename,
  const unsigned thid,
  vector<float>& samples);

void runTransient(
  const Control& control,
  const vector<float>& samples,
  const double sampleRate,
  const unsigned thid,
  Transient& transient,
  unsigned& lastIndex);

void runQuiet(
  const vector<float>& samples,
  const double sampleRate,
  const unsigned lastIndex,
  const unsigned thid,
  Quiet& quietBack,
  Quiet& quietFront,
  Interval& interval);


string runHeader(const TraceData& traceData)
{
  stringstream ss;
  ss << "File " << traceData.filename << 
    ": number " << traceData.traceNoInRun;

  const unsigned l = ss.str().size();
  ss << "\n" << string(l, '=') << "\n\n";

  return ss.str();
}


void runRead(
  const string& filename,
  const unsigned thid,
  vector<float>& samples)
{
  timers[thid].start(TIMER_READ);

  if (! readBinaryTrace(filename, samples))
    THROW(ERR_NO_TRACE_FILE, "Trace file not read");

  timers[thid].stop(TIMER_READ);
}


void runTransient(
  const Control& control,
  const vector<float>& samples,
  const double sampleRate,
  const unsigned thid,
  Transient& transient,
  unsigned& lastIndex)
{
  timers[thid].start(TIMER_TRANSIENT);

  transient.detect(samples, sampleRate, lastIndex);

  if (control.verboseTransient())
    cout << transient.str() << "\n";

  timers[thid].stop(TIMER_TRANSIENT);
}


void runQuiet(
  const vector<float>& samples,
  const double sampleRate,
  const unsigned lastIndex,
  const unsigned thid,
  Quiet& quietBack,
  Quiet& quietFront,
  Interval& interval)
{
  timers[thid].start(TIMER_QUIET);

  interval.first = lastIndex;
  interval.len = samples.size() - lastIndex;

  quietBack.detect(samples, sampleRate, true, interval);
  quietFront.detect(samples, sampleRate, false, interval);

  timers[thid].stop(TIMER_QUIET);
}


void runWrite(
  const Control& control,
  const Transient& transient,
  const Quiet& quietBack,
  const Quiet& quietFront,
  const Filter& filter,
  const PeakDetect& peakDetect,
  const string& filename,
  const unsigned thid)
{
  timers[thid].start(TIMER_WRITE);

  if (control.writeTransient())
    transient.writeFile(control.transientDir() + "/" + filename);
  if (control.writeBack())
    quietBack.writeFile(control.backDir() + "/" + filename);
  if (control.writeFront())
    quietFront.writeFile(control.frontDir() + "/" + filename);
  if (control.writeSpeed())
    filter.writeSpeed(control.speedDir() + "/" + filename);
  if (control.writePos())
    filter.writePos(control.posDir() + "/" + filename);
  if (control.writePeak())
    peakDetect.writePeak(control.peakDir() + "/" + filename);

  timers[thid].stop(TIMER_WRITE);
}


void run(
  const Control& control,
  const TrainDB& trainDB,
  const TraceData& traceData,
  const unsigned thid)
{
  Imperfections imperf;
  Align align;
  Regress regress;

  vector<Alignment> matchesAlign;
  Alignment bestAlign;
  
  Motion motion;

  Transient transient;
  Quiet quietFront;
  Quiet quietBack;
  Filter filter;
  PeakDetect peakDetect;

  vector<double> times;
  vector<int> actualToRef;
  unsigned numFrontWheels;

  // TODO If something?
  cout << runHeader(traceData);

  vector<double> posTrue;
      
  try
  {
    if (traceData.trainNoTrue == -1)
      THROW(ERR_TRUE_TRAIN_UNKNOWN, "True train not known");

    vector<float> samples;
    runRead(traceData.filenameFull, thid, samples);

    unsigned lastIndex;
    runTransient(control, samples, traceData.sampleRate, thid, 
      transient, lastIndex);

    Interval interval;
    runQuiet(samples, traceData.sampleRate, lastIndex, thid,
      quietBack, quietFront, interval);


  // TODO Leave as floats for a while longer.
  vector<double> dsamples;
  dsamples.resize(samples.size());
  for (unsigned i = 0; i < samples.size(); i++)
    dsamples[i] = samples[i];

  timers[thid].start(TIMER_CONDITION);
  (void) filter.detect(dsamples, traceData.sampleRate, 
    interval.first, interval.len);
  timers[thid].stop(TIMER_CONDITION);

  const vector<float>& synthPos = filter.getDeflection();

  timers[thid].start(TIMER_DETECT_PEAKS);
  peakDetect.reset();
  peakDetect.log(synthPos, interval.first);
  peakDetect.reduce(control, imperf);
  timers[thid].stop(TIMER_DETECT_PEAKS);

  vector<float> synthPeaks;
  synthPeaks.resize(interval.len);
  peakDetect.makeSynthPeaks(interval.first, interval.len);


    trainDB.getPeakPositions(traceData.trainNoTrueU, posTrue);

    peakDetect.logPeakStats(posTrue, traceData.trainTrue, 
      traceData.speed, peakStats);

    runWrite(control, transient, quietBack, quietFront, filter,
      peakDetect, traceData.filename, thid);

    bool fullTrainFlag;
    if (peakDetect.getAlignment(times, actualToRef, numFrontWheels) &&
        ! actualToRef.empty())
    {
      cout << "FULLALIGN\n";
      fullTrainFlag = true;
    }
    else
    {
      peakDetect.getPeakTimes(times, numFrontWheels);

cout << "Got " << times.size() << " peaks, " <<
  numFrontWheels << " front wheels\n\n";


      fullTrainFlag = false;
    }

    // TODO Kludge.  Should be in PeakDetect or so.
    if (imperf.numSkipsOfSeen > 0)
      numFrontWheels = 4 - imperf.numSkipsOfSeen;

    align.bestMatches(times, actualToRef, numFrontWheels, fullTrainFlag,
      imperf, trainDB, traceData.countrySensor, 10, control, thid,
      matchesAlign);

    if (matchesAlign.size() == 0)
      THROW(ERR_NO_ALIGN_MATCHES, "No alignment matches");

    regress.bestMatch(times, trainDB, control, thid, matchesAlign,
      bestAlign, motion);

    if (! control.pickAny().empty())
    {
      const string s = traceData.sensor + "/" + traceData.time;
      dumpResiduals(times, trainDB, motion.order, matchesAlign, s, 
        traceData.trainTrue, control.pickAny(), 
          trainDB.numAxles(traceData.trainNoTrueU));
    }

    const string trainDetected = trainDB.lookupName(bestAlign.trainNo);
    const unsigned rank = lookupMatchRank(trainDB, matchesAlign, 
      traceData.trainTrue);

    sensorStats.log(traceData.sensor, rank, bestAlign.distMatch);
    trainStats.log(traceData.trainTrue, rank, bestAlign.distMatch);

if (trainDetected != traceData.trainTrue)
  cout << "DRIVER MISMATCH\n";

  }
  catch (Except& ex)
  {
    ex.print(cout);
    sensorStats.fail(traceData.sensor);
    trainStats.fail(traceData.trainTrue);
  }
  catch(...)
  {
    cout << "Unknown exception" << endl;
  }
}


unsigned lookupMatchRank(
  const TrainDB& trainDB,
  const vector<Alignment>& matches,
  const string& tag)
{
  const unsigned tno = static_cast<unsigned>(trainDB.lookupNumber(tag));
  for (unsigned i = 0; i < matches.size(); i++)
  {
    if (matches[i].trainNo == tno)
      return i;
  }
  return numeric_limits<unsigned>::max();
}

