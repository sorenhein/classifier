#include <iostream>
#include <limits>

#include "database/TraceDB.h"
#include "database/TrainDB.h"
#include "database/Control.h"

#include "stats/CompStats.h"
#include "stats/PeakStats.h"

#include "Trace.h"
#include "Regress.h"
#include "Align.h"
#include "Except.h"
#include "errors.h"

#include "geometry.h"
#include "run.h"

#include "util/Motion.h"


using namespace std;

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


extern CompStats sensorStats;
extern CompStats trainStats;
extern PeakStats peakStats;


unsigned lookupMatchRank(
  const TrainDB& trainDB,
  const vector<Alignment>& matches,
  const string& tag);


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

  Trace trace;
  vector<double> times;
  vector<int> actualToRef;
  unsigned numFrontWheels;

  cout << "File " << traceData.filename << ": number " <<
    traceData.traceNoInRun << "\n\n";

  vector<double> posTrue;
      
  try
  {
    if (traceData.trainNoTrue == -1)
      THROW(ERR_NO_PEAKS, "True train not known");

    trainDB.getPeakPositions(traceData.trainNoTrueU, posTrue);

    // Refuse trace if sample rate is not 2000, maybe in SegActive

    trace.read(traceData.filenameFull, thid);
    trace.detect(control, traceData.sampleRate, thid, imperf);
    trace.logPeakStats(posTrue, traceData.trainTrue, 
      traceData.speed, peakStats);
    trace.write(control, traceData.filename, thid);

    bool fullTrainFlag;
    if (trace.getAlignment(times, actualToRef, numFrontWheels) &&
        ! actualToRef.empty())
    {
      cout << "FULLALIGN\n";
      fullTrainFlag = true;
    }
    else
    {
      trace.getTrace(times, numFrontWheels);
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

