#include <iostream>
#include <iomanip>
#include <string>
#include <limits>

#include "args.h"
#include "read.h"
#include "Trace.h"

#include "database/SensorDB.h"
#include "database/CarDB.h"
#include "database/CorrectionDB.h"
#include "database/TrainDB.h"

#include "TraceDB.h"
#include "Regress.h"
#include "Align.h"
#include "CompStats.h"
#include "PeakStats.h"
#include "Except.h"
#include "geometry.h"
#include "print.h"

#include "util/Timers.h"
#include "util/misc.h"

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

  if (control.traceDir != "")
  {
    // This extracts peaks and then extracts train types from peaks.

    TraceDB traceDB;
    readTraceTruth(control.truthFile, sensorDB, trainDB, traceDB);

    vector<string> datfiles;
    getFilenames(control.traceDir, datfiles, control.pickFileString);

    Trace trace;
    vector<PeakTime> times;
    vector<int> actualToRef;
    unsigned numFrontWheels;

    CompStats sensorStats, trainStats;
    PeakStats peakStats;

    for (auto& fname: datfiles)
    {
      const string sensor = traceDB.lookupSensor(fname);
      const string country = sensorDB.country(sensor);
      const string trainTrue = traceDB.lookupTrueTrain(fname);

if (! control.pickTrainString.empty() &&
    ! nameMatch(trainTrue, control.pickTrainString))
  continue;

      cout << "File " << fname << ":\n\n";

      // This is only used for diagnostics in trace.
      const double speedTrue = traceDB.lookupTrueSpeed(fname);
      const int trainNoTrue = trainDB.lookupNumber(trainTrue);
      vector<double> posTrue;
      
      try
      {
        if (trainNoTrue == -1)
          THROW(ERR_NO_PEAKS, "True train not known");

        trainDB.getPeakPositions(static_cast<unsigned>(trainNoTrue), 
          posTrue);

        trace.read(fname);
        trace.detect(control, imperf);
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
          traceDB.log(fname, matchesAlign, times.size());
          sensorStats.log(sensor, 10, 1000.);
          trainStats.log(trainTrue, 10, 1000.);
          continue;
        }

        regress.bestMatch(times, trainDB, order, control, matchesAlign,
          bestAlign, motionEstimate);

        if (! control.pickTrainString.empty())
        {
          const string s = sensor + "/" + traceDB.lookupTime(fname);
          dumpResiduals(times, trainDB, order, matchesAlign, s, trainTrue, 
            control.pickTrainString, 
            trainDB.numAxles(static_cast<unsigned>(trainNoTrue)));
        }

        traceDB.log(fname, matchesAlign, times.size());

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

    traceDB.printCSV(control.summaryFile, control.summaryAppendFlag, 
      trainDB);

    sensorStats.print("sensorstats.txt", "Sensor");
    trainStats.print("trainstats.txt", "Train");
    peakStats.print("peakstats.txt");
  }
  else
  {
    // This was once use to generate synthetic, noisy peaks and
    // to classify them.
    cout << "No traces specified.\n";
  }

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
  if (argc == 2)
    control.controlFile = string(argv[1]);
  else
    readArgs(argc, argv, control);

  if (! readControlFile(control, control.controlFile))
  {
    cout << "Bad control file" << control.controlFile << endl;
    exit(0);
  }

  // carDB
  vector<string> textfiles;
  getFilenames(control.carDir, textfiles);
  for (auto& fname: textfiles)
    carDB.readFile(fname);

  // correctionDB
  CorrectionDB correctionDB;
  vector<string> correctionFiles;
  getFilenames(control.correctionDir, correctionFiles);
  for (auto& fname: correctionFiles)
    correctionDB.readFile(fname);

  // trainDB
  textfiles.clear();
  getFilenames(control.trainDir, textfiles);
  for (auto& fname: textfiles)
    trainDB.readFile(carDB, correctionDB, fname);

  // sensorDB
  if (control.sensorFile != "")
    sensorDB.readFile(control.sensorFile);

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

