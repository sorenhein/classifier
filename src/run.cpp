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
#include "stats/CountStats.h"
#include "stats/CrossStats.h"
#include "stats/ResidualsStats.h"
#include "stats/Timers.h"

// TODO Eliminate?
// #include "AccelDetect.h"

#include "PeakDetect.h"
#include "PeakSeeds.h"
#include "PeakLabel.h"

#include "align/Align.h"

#include "Except.h"
#include "errors.h"

#include "run.h"

#include "util/io.h"


using namespace std;

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


extern CompStats sensorStats;
extern CompStats trainStats;

extern CountStats overallStats;
extern CountStats deviationStats;

extern vector<CompStats> sensorStatsList;
extern vector<CrossStats> crossStatsList;
extern vector<CompStats> trainStatsList;

extern ResidualsStats residualsStats;

extern vector<Timers> timers;


string runHeader(const TraceData& traceData);

void runRead(
  const string& filename,
  const unsigned thid,
  vector<float>& accel);

void runTransient(
  const Control& control,
  const vector<float>& accel,
  const float sampleRate,
  const unsigned thid,
  Transient& transient,
  unsigned& lastIndex);

void runQuiet(
  const Control& control,
  const vector<float>& accel,
  const float sampleRate,
  const unsigned lastIndex,
  const unsigned thid,
  Quiet& quietBack,
  Quiet& quietFront,
  Interval& interval);

void runFilter(
  const float sampleRate,
  const unsigned start,
  const unsigned len,
  const unsigned thid,
  Filter& filter);

void runPeakDetect(
  const Control& control,
  const vector<float>& deflection,
  const unsigned offset,
  const unsigned len,
  const unsigned thid,
  PeakDetect& peakDetect);

void runPeakLabel(
  PeakPool& peaks, 
  const float scale,
  const unsigned offset,
  const unsigned thid,
  PeakLabel& peakLabel);

void runPeakExtract(
  PeakPool& peaks, 
  const double sampleRate,
  const unsigned offset,
  const unsigned thid,
  PeakStructure& pstruct,
  PeaksInfo& peaksInfo);

void runWrite(
  const Control& control,
  const Transient& transient,
  const Quiet& quietBack,
  const Quiet& quietFront,
  const Filter& filter,
  const PeakDetect& peakDetect,
  const string& filename,
  const unsigned offset,
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
  const float sampleRate,
  const unsigned thid,
  Transient& transient,
  unsigned& lastIndex)
{
  timers[thid].start(TIMER_TRANSIENT);

  transient.detect(accel, sampleRate, lastIndex);

  if (control.verboseTransient())
    cout << transient.str();

  timers[thid].stop(TIMER_TRANSIENT);
}


void runQuiet(
  const Control& control,
  const vector<float>& accel,
  const float sampleRate,
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

  if (control.verboseQuiet())
  {
    cout << "Active interval\n";
    cout << string(15, '-') << "\n\n";
    cout << interval.first << " to " << 
      interval.first + interval.len << 
      " out of 0 to " << accel.size() << "\n\n";
  }

  timers[thid].stop(TIMER_QUIET);

list<Interval> actives;
quietFront.detectIntervals(accel, interval, actives);
}


void runFilter(
  const float sampleRate,
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
  const unsigned len,
  const unsigned thid,
  PeakDetect& peakDetect)
{
  UNUSED(len);

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
  PeakLabel& peakLabel)
{
  timers[thid].start(TIMER_LABEL_PEAKS);

  peakLabel.mark(peaks, scale, offset);

  timers[thid].stop(TIMER_LABEL_PEAKS);
}



void runPeakExtract(
  PeakPool& peaks, 
  const double sampleRate,
  const unsigned offset,
  const unsigned thid,
  PeakStructure& pstruct,
  PeaksInfo& peaksInfo)
{
  timers[thid].start(TIMER_EXTRACT_CARS);

  pstruct.markCars(peaks, offset);

  // Get PeakStruct results in a useful format for Alignment.
  pstruct.getPeaksInfo(peaks, peaksInfo, sampleRate);

  timers[thid].stop(TIMER_EXTRACT_CARS);
}



void runWrite(
  const Control& control,
  const Transient& transient,
  const Quiet& quietBack,
  const Quiet& quietFront,
  const Filter& filter,
  const PeakDetect& peakDetect,
  const string& filename,
  const unsigned offset,
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
    peakDetect.writePeak(control, offset, filename);
    // peakDetect.writePeak(control.peakDir() + "/" + filename);

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
    overallStats.log("count");

    if (traceData.trainNoTrue == -1)
      THROW(ERR_TRUE_TRAIN_UNKNOWN, "Truth train not known");

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
    runQuiet(control, accel, traceData.sampleRate, lastIndex, thid,
      quietBack, quietFront, interval);

/*
vector<float> rawAccel;
rawAccel.resize(interval.len);
for (unsigned i = interval.first; i < interval.first + interval.len; i++)
  rawAccel[i-interval.first] = accel[i];
  */

    // Integrate and condition the signal.
    runFilter(traceData.sampleRate, interval.first, interval.len, thid, 
      filter);

/*
cout << "ACCEL\n";
for (unsigned i = 0; i < rawAccel.size(); i++)
  cout << i << ";" << rawAccel[i] << "\n";
cout << "\n";

AccelDetect accelDetect;
accelDetect.log(rawAccel, interval.first, interval.len);
accelDetect.extract();
*/

    // Extract immediate peaks from the signal.
    PeakDetect peakDetect;
    runPeakDetect(control, filter.getDeflection(), interval.first, 
      interval.len, thid, peakDetect);

    // Label some negative mimina as wheels, bogies etc.
    PeakPool& peaks = peakDetect.getPeaks();
    PeakLabel peakLabel;
    runPeakLabel(peaks, peakDetect.getScale().getRange(), 
      interval.first, thid, peakLabel);


    // Use the labels to extract the car structure from the peaks.
    PeakStructure pstruct;
    PeaksInfo peaksInfo;
    runPeakExtract(peaks, traceData.sampleRate, interval.first, 
      thid, pstruct, peaksInfo);

    if (peaksInfo.numPeaks == 0)
      THROW(ERR_NO_PEAKS_IN_STRUCTURE, "No peaks in structure");

    if (peaksInfo.numCars == 0)
      THROW(ERR_CARS_NOT_COMPLETE, "Not all cars detected");
  

    // Put in runPeakExtract, controlled by new Control flag
    cout << "PEAKPOOL\n";
    cout << peaks.strCounts();

    timers[thid].start(TIMER_ALIGN);

    Align align;
    if (! align.realign(trainDB, traceData.countrySensor, peaksInfo))
      THROW(ERR_NO_ALIGN_MATCHES, "No alignment matches");

    cout << "True train " << traceData.trainTrue << " at " <<
      fixed << setprecision(2) << traceData.speed << " m/s\n\n";

    if (control.verboseAlignMatches())
      cout << align.strMatches("Matching alignment");


    // Run the regression with the given alignment.
    timers[thid].start(TIMER_REGRESS);

    align.regress(trainDB, peaksInfo, control.verboseRegressTopResiduals());
    cout << align.strRegress(control);

    if (! control.pickAny().empty())
    {
      const string s = traceData.sensor + "/" + traceData.time;
      cout << align.strMatchingResiduals(traceData.trainTrue,
        control.pickAny(), s);
    }

    timers[thid].stop(TIMER_ALIGN);

    string trainDetected;
    float distDetected;
    align.getBest(trainDetected, distDetected);
    unsigned rankDetected = align.getMatchRank(traceData.trainNoTrueU);

    // Write any binary output files.

    // peakDetect.makeSynthPeaks(interval.first, interval.len);

    runWrite(control, transient, quietBack, quietFront, filter,
      peakDetect, traceData.filename, interval.first, thid);

    sensorStats.log(traceData.sensor, rankDetected, distDetected);
    trainStats.log(traceData.trainTrue, rankDetected, distDetected);
    sensorStatsList[thid].log(traceData.sensor, rankDetected, distDetected);
    crossStatsList[thid].log(traceData.trainTrue, trainDetected);
    trainStatsList[thid].log(traceData.trainTrue, rankDetected, distDetected);

    if (control.writeBox())
    {
      align.writeTrainBoxes(
        control, trainDB, traceData.filename, interval.first,
          traceData.sampleRate, traceData.trainTrue);
    }

    if (trainDetected == traceData.trainTrue)
    {
      overallStats.log("good");
      align.updateStats();

      // TODO Make a control out of it
      if (1)
      {
        residualsStats.log(trainDetected, align.best());
      }
    }
    else if (trainDB.equivalent(trainDetected, traceData.trainTrue))
    {
      cout << "EQUIVMATCH\n";
      overallStats.log("good");
      align.updateStats();
    }
    else
    {
      // TODO Could put offset into traceData.
      // Could pass traceData to writeTrainBoxes().
      /*
      if (control.writeBox())
      {
        align.writeTrainBoxes(
          control, trainDB, traceData.filename, interval.first,
            traceData.sampleRate, traceData.trainTrue);

        align.writeTrain(
          trainDB,
          control.matchDir() + "/" + traceData.filename,
          filter.getDeflection(),
          interval.first, 
          traceData.sampleRate,
          trainDetected);
      }
          */

      overallStats.log("error");
      cout << "DRIVER MISMATCH\n";
    }

    deviationStats.log(align.strDeviation());
  }
  catch (Except& ex)
  {
    ex.print(cout);
    sensorStats.fail(traceData.sensor);
    trainStats.fail(traceData.trainTrue);
    sensorStatsList[thid].fail(traceData.sensor);
    trainStatsList[thid].fail(traceData.trainTrue);
  }
  catch(...)
  {
    cout << "Unknown exception" << endl;
  }
}

