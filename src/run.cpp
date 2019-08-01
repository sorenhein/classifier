#include <iostream>
#include <sstream>
#include <limits>

#include "database/TraceDB.h"
#include "database/TrainDB.h"
#include "database/Control.h"

#include "transient/Transient.h"
#include "transient/Quiet.h"

#include "filter/Filter.h"

#include "stats/CompStats.h"
#include "stats/PeakStats.h"
#include "stats/Timers.h"

#include "PeakDetect.h"
#include "PeakSeeds.h"
#include "PeakMinima.h"
#include "PeakMatch.h"

#include "Regress.h"
#include "Align.h"
#include "Except.h"
#include "errors.h"

#include "run.h"

#include "util/io.h"


using namespace std;

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


extern CompStats sensorStats;
extern CompStats trainStats;
extern PeakStats peakStats;

extern vector<Timers> timers;


string runHeader(const TraceData& traceData);

void runRead(
  const string& filename,
  const unsigned thid,
  vector<float>& accel);

void runTransient(
  const Control& control,
  const vector<float>& accel,
  const double sampleRate,
  const unsigned thid,
  Transient& transient,
  unsigned& lastIndex);

void runQuiet(
  const vector<float>& accel,
  const double sampleRate,
  const unsigned lastIndex,
  const unsigned thid,
  Quiet& quietBack,
  Quiet& quietFront,
  Interval& interval);

void runFilter(
  const double sampleRate,
  const unsigned start,
  const unsigned len,
  const unsigned thid,
  Filter& filter);

void runPeakDetect(
  const Control& control,
  const vector<float>& deflection,
  const unsigned offset,
  const unsigned thid,
  PeakDetect& peakDetect);

void runPeakLabel(
  PeakPool& peaks, 
  const float scale,
  const unsigned offset,
  const unsigned thid,
  PeakMinima& peakMinima);

void runWrite(
  const Control& control,
  const Transient& transient,
  const Quiet& quietBack,
  const Quiet& quietFront,
  const Filter& filter,
  const PeakDetect& peakDetect,
  const string& filename,
  const unsigned thid);


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
  vector<float>& accel)
{
  timers[thid].start(TIMER_READ);

  if (! readBinaryTrace(filename, accel))
    THROW(ERR_NO_TRACE_FILE, "Trace file not read");

  timers[thid].stop(TIMER_READ);
}


void runTransient(
  const Control& control,
  const vector<float>& accel,
  const double sampleRate,
  const unsigned thid,
  Transient& transient,
  unsigned& lastIndex)
{
  timers[thid].start(TIMER_TRANSIENT);

  transient.detect(accel, sampleRate, lastIndex);

  if (control.verboseTransient())
    cout << transient.str() << "\n";

  timers[thid].stop(TIMER_TRANSIENT);
}


void runQuiet(
  const vector<float>& accel,
  const double sampleRate,
  const unsigned lastIndex,
  const unsigned thid,
  Quiet& quietBack,
  Quiet& quietFront,
  Interval& interval)
{
  timers[thid].start(TIMER_QUIET);

  interval.first = lastIndex;
  interval.len = accel.size() - lastIndex;

  quietBack.detect(accel, sampleRate, true, interval);
  quietFront.detect(accel, sampleRate, false, interval);

  timers[thid].stop(TIMER_QUIET);
}


void runFilter(
  const double sampleRate,
  const unsigned start,
  const unsigned len,
  const unsigned thid,
  Filter& filter)
{
  timers[thid].start(TIMER_FILTER);

  filter.process(sampleRate, start, len);

  timers[thid].stop(TIMER_FILTER);
}


void runPeakDetect(
  const Control& control,
  const vector<float>& deflection,
  const unsigned offset,
  const unsigned thid,
  PeakDetect& peakDetect)
{
  timers[thid].start(TIMER_EXTRACT_PEAKS);

  peakDetect.reset();
  peakDetect.log(deflection, offset);

  peakDetect.extract(control);

  timers[thid].stop(TIMER_EXTRACT_PEAKS);
}


void runPeakLabel(
  PeakPool& peaks, 
  const float scale,
  const unsigned offset,
  const unsigned thid,
  PeakMinima& peakMinima)
{
  timers[thid].start(TIMER_LABEL_PEAKS);

  peakMinima.mark(peaks, scale, offset);

  timers[thid].stop(TIMER_LABEL_PEAKS);
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
  try
  {
    // TODO If something?
    cout << runHeader(traceData);

    if (traceData.trainNoTrue == -1)
      THROW(ERR_TRUE_TRAIN_UNKNOWN, "True train not known");

    // Write directly into filter storage.
    Filter filter;
    vector<float>& accel = filter.getAccel();

    // Read the trace.
    runRead(traceData.filenameFull, thid, accel);

    // Detect any leading transient.
    Transient transient;
    unsigned lastIndex;
    runTransient(control, accel, traceData.sampleRate, thid, 
      transient, lastIndex);

    // Detect any leading or trailing quiet periods.
    Quiet quietBack;
    Quiet quietFront;
    Interval interval;
    runQuiet(accel, traceData.sampleRate, lastIndex, thid,
      quietBack, quietFront, interval);

    // Integrate and condition the signal.
    runFilter(traceData.sampleRate, interval.first, interval.len, thid, 
      filter);

    // Extract immediate peaks from the signal.
    PeakDetect peakDetect;
    runPeakDetect(control, filter.getDeflection(), interval.first, thid,
      peakDetect);

    // Label some negative mimina as wheels, bogies etc.
    PeakPool& peaks = peakDetect.getPeaks();
    PeakMinima peakMinima;
    runPeakLabel(peaks, peakDetect.getScale().getRange(), 
      interval.first, thid, peakMinima);



    // Use the labels to extract the car structure from the peaks.
    PeakStructure pstruct;
    timers[thid].start(TIMER_EXTRACT_CARS);
    pstruct.markCars(peaks, interval.first);

    Imperfections imperf;
    if (! pstruct.markImperfections(imperf))
      cout << "WARNING: Failed to mark imperfections\n";

    cout << "PEAKPOOL\n";
    cout << peaks.strCounts();
    timers[thid].stop(TIMER_EXTRACT_CARS);



  Align align;
  Regress regress;

  vector<float> times;
  vector<int> actualToRef;
  unsigned numFrontWheels;




  peakDetect.makeSynthPeaks(interval.first, interval.len);


    const vector<float>& posTrue = 
      trainDB.getPeakPositions(traceData.trainNoTrueU);

    PeakMatch peakMatch;
    peakMatch.logPeakStats(peaks, posTrue, traceData.trainTrue,
      traceData.speed, peakStats);

    runWrite(control, transient, quietBack, quietFront, filter,
      peakDetect, traceData.filename, thid);

    bool fullTrainFlag;
    if (pstruct.getAlignment(times, actualToRef, numFrontWheels) &&
        ! actualToRef.empty())
    {
      cout << "FULLALIGN\n";
      fullTrainFlag = true;
    }
    else
    {

  peaks.topConst().getTimes(&Peak::isSelected, times);
  numFrontWheels = pstruct.getNumFrontWheels();

cout << "Got " << times.size() << " peaks, " <<
  numFrontWheels << " front wheels\n\n";


      fullTrainFlag = false;
    }

    // TODO Kludge.  Should be in PeakDetect or so.
    if (imperf.numSkipsOfSeen > 0)
      numFrontWheels = 4 - imperf.numSkipsOfSeen;

    // The storage is in Regress, but it is first used in Align.
    auto& matches = regress.getMatches();

    align.bestMatches(times, actualToRef, numFrontWheels, fullTrainFlag,
      imperf, trainDB, traceData.countrySensor, 10, control, thid,
      matches);

    if (matches.size() == 0)
      THROW(ERR_NO_ALIGN_MATCHES, "No alignment matches");


    // Run the regression with the given alignment.
    timers[thid].start(TIMER_REGRESS);

    regress.bestMatch(trainDB, times);
    cout << regress.str(control);


    if (! control.pickAny().empty())
    {
      const string s = traceData.sensor + "/" + traceData.time;
      cout << regress.strMatchingResiduals(traceData.trainTrue,
        control.pickAny(), s);
    }

    timers[thid].stop(TIMER_REGRESS);


    // Update statistics.
    string trainDetected;
    float distDetected;
    unsigned rankDetected;
    regress.getBest(traceData.trainNoTrueU,
      trainDetected, distDetected, rankDetected);

    sensorStats.log(traceData.sensor, rankDetected, distDetected);
    trainStats.log(traceData.trainTrue, rankDetected, distDetected);

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

