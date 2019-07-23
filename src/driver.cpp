#include <iostream>
#include <iomanip>
#include <string>
#include <limits>

#include "Trace.h"

#include "database/SensorDB.h"
#include "database/CarDB.h"
#include "database/CorrectionDB.h"
#include "database/TrainDB.h"
#include "database/TraceDB.h"
#include "database/Control.h"

#include "Regress.h"
#include "Align.h"
#include "CompStats.h"
#include "PeakStats.h"
#include "Except.h"
#include "geometry.h"
#include "print.h"

#include "util/Timers.h"
#include "util/io.h"

using namespace std;

Log logger;
Timers timers;


void setup(
  int argc, 
  char * argv[],
  Control& control,
  SensorDB& sensorDB,
  CarDB& carDB,
  TrainDB& trainDB);

unsigned lookupMatchRank(
  const TrainDB& trainDB,
  const vector<Alignment>& matches,
  const string& tag);


int main(int argc, char * argv[])
{
  Control control;

  SensorDB sensorDB;
  CarDB carDB;
  TrainDB trainDB;

  setup(argc, argv, control, sensorDB, carDB, trainDB);

  if (control.traceDir() == "")
  {
    // This was once use to generate synthetic, noisy peaks and
    // to classify them.
    cout << "No traces specified.\n";
    exit(0);
  }

  // This extracts peaks and then extracts train types from peaks.
  TraceDB traceDB;
  traceDB.readFile(control.truthFile(), sensorDB);

  vector<string> datfiles;
  if (! getFilenames(control.traceDir(), datfiles, control.pickFirst()))
  {
    cout << "Bad directory " << control.traceDir() << endl;
    exit(0);
  }

  CompStats sensorStats, trainStats;
  PeakStats peakStats;



  Imperfections imperf;
  Align align;
  Regress regress;

  vector<PeakPos> perfectPositions;
  vector<Alignment> matchesAlign;
  Alignment bestAlign;
  
  const unsigned order = 2;
  vector<double> motionActual(order+1);
  vector<double> motionEstimate(order+1);
  motionActual[0] = 0.; // Offset in m

  Trace trace;
  vector<PeakTime> times;
  vector<int> actualToRef;
  unsigned numFrontWheels;

  for (auto& fname: datfiles)
  {
    const unsigned t2no = traceDB.traceNumber(fname);
    const string sensor = traceDB.sensor(t2no);
    const string trainTrue = traceDB.train(t2no);

    const string country = sensorDB.country(sensor);

    if (! control.pickAny().empty() &&
        ! nameMatch(trainTrue, control.pickAny()))
      continue;

    cout << "File " << fname << ":\n\n";

    // This is only used for diagnostics in trace.
    const double speedTrue = traceDB.speed(t2no);
    const int trainNoTrue = trainDB.lookupNumber(trainTrue);
    vector<double> posTrue;
      
    try
    {
      if (trainNoTrue == -1)
        THROW(ERR_NO_PEAKS, "True train not known");

      trainDB.getPeakPositions(static_cast<unsigned>(trainNoTrue), posTrue);

      // Refuse trace if sample rate is not 2000, maybe in SegActive

      trace.read(fname);
      trace.detect(control, traceDB.sampleRate(t2no), imperf);
      trace.logPeakStats(posTrue, trainTrue, speedTrue, peakStats);
      trace.write(control);

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
        imperf, trainDB, country, 10, control, matchesAlign);

      if (matchesAlign.size() == 0)
      {
        sensorStats.log(sensor, 10, 1000.);
        trainStats.log(trainTrue, 10, 1000.);
        continue;
      }

      regress.bestMatch(times, trainDB, order, control, matchesAlign,
        bestAlign, motionEstimate);

      if (! control.pickAny().empty())
      {
        const string s = sensor + "/" + traceDB.time(t2no);
        dumpResiduals(times, trainDB, order, matchesAlign, s, trainTrue, 
          control.pickAny(), 
          trainDB.numAxles(static_cast<unsigned>(trainNoTrue)));
      }

      const string trainDetected = trainDB.lookupName(bestAlign.trainNo);
      const unsigned rank = lookupMatchRank(trainDB, matchesAlign, 
        trainTrue);

      sensorStats.log(sensor, rank, bestAlign.distMatch);
      trainStats.log(trainTrue, rank, bestAlign.distMatch);

if (trainDetected != trainTrue)
  cout << "DRIVER MISMATCH\n";

    }
    catch (Except& ex)
    {
      ex.print(cout);
      sensorStats.log(sensor, 10, 1000.);
      trainStats.log(trainTrue, 10, 1000.);
    }
    catch(...)
    {
      cout << "Unknown exception" << endl;
    }
  }

  sensorStats.write("sensorstats.txt", "Sensor");
  trainStats.write("trainstats.txt", "Train");
  peakStats.print("peakstats.txt");

  cout << timers.str(2) << endl;
}


void setup(
  int argc, 
  char * argv[],
  Control& control,
  SensorDB& sensorDB,
  CarDB& carDB,
  TrainDB& trainDB)
{
  if (! control.parseCommandLine(argc, argv))
  {
    cout << "Bad command line" << endl;
    exit(0);
  }

  // carDB
  vector<string> textfiles;
  if (! getFilenames(control.carDir(), textfiles))
  {
    cout << "Bad directory " << control.carDir() << endl;
    exit(0);
  }

  for (auto& fname: textfiles)
    carDB.readFile(fname);

  // correctionDB
  CorrectionDB correctionDB;
  vector<string> correctionFiles;

  if (! getFilenames(control.correctionDir(), correctionFiles))
  {
    cout << "Bad directory " << control.correctionDir() << endl;
    exit(0);
  }

  for (auto& fname: correctionFiles)
    correctionDB.readFile(fname);

  // trainDB
  textfiles.clear();
  if (! getFilenames(control.trainDir(), textfiles))
  {
    cout << "Bad directory " << control.trainDir() << endl;
    exit(0);
  }

  for (auto& fname: textfiles)
    trainDB.readFile(carDB, correctionDB, fname);

  // sensorDB
  if (control.sensorFile() != "")
    sensorDB.readFile(control.sensorFile());

  if (! trainDB.selectByAxles({"ALL"}, 0, 100))
  {
    cout << "No trains selected" << endl;
    exit(0);
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

