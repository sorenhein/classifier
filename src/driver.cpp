#include <iostream>
#include <iomanip>
#include <string>
#include <limits>

#include "args.h"
#include "read.h"
#include "Trace.h"
#include "Database.h"
#include "TraceDB.h"
#include "Regress.h"
#include "Align.h"
#include "Timers.h"
#include "CompStats.h"
#include "PeakStats.h"
#include "Except.h"
#include "geometry.h"
#include "print.h"

using namespace std;

Log logger;
Timers timers;


void setup(
  int argc, 
  char * argv[],
  Control& control,
  Database& db);

unsigned lookupMatchRank(
  const Database& db,
  const vector<Alignment>& matches,
  const string& tag);


int main(int argc, char * argv[])
{
  Control control;
  Database db;
  setup(argc, argv, control, db);

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
    readTraceTruth(control.truthFile, db, traceDB);

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
      const string country = db.lookupSensorCountry(sensor);
      const string trainTrue = traceDB.lookupTrueTrain(fname);

if (! control.pickTrainString.empty() &&
    ! nameMatch(trainTrue, control.pickTrainString))
  continue;

      cout << "File " << fname << ":\n\n";

      // This is only used for diagnostics in trace.
      const double speedTrue = traceDB.lookupTrueSpeed(fname);
      const int trainNoTrue = db.lookupTrainNumber(trainTrue);
      vector<PeakPos> posTrue;
      db.getPerfectPeaks(static_cast<unsigned>(trainNoTrue), posTrue);
      
      try
      {
        trace.read(fname, true);
        trace.detect(control, imperf);
        trace.logPeakStats(posTrue, trainTrue, speedTrue, peakStats);
        trace.write(control);

        bool fullTrainFlag;
        if (trace.getAlignment(times, actualToRef, numFrontWheels))
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
          imperf, db, country, 10, control, matchesAlign);

        if (matchesAlign.size() == 0)
        {
          traceDB.log(fname, matchesAlign, times.size());
          sensorStats.log(sensor, 10, 1000.);
          trainStats.log(trainTrue, 10, 1000.);
          continue;
        }

        regress.bestMatch(times, db, order, control, matchesAlign,
          bestAlign, motionEstimate);

        if (! control.pickTrainString.empty())
        {
          const string s = sensor + "/" + traceDB.lookupTime(fname);
          dumpResiduals(times, db, order, matchesAlign, s, trainTrue, 
            control.pickTrainString, 
            db.axleCount(static_cast<unsigned>(trainNoTrue)));
        }

        traceDB.log(fname, matchesAlign, times.size());

        const string trainDetected = db.lookupTrainName(bestAlign.trainNo);
        const unsigned rank = lookupMatchRank(db, matchesAlign, trainTrue);

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

    traceDB.printCSV(control.summaryFile, control.summaryAppendFlag, db);

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
  Database& db)
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

  readCarFiles(db, control.carDir);
  readTrainFiles(db, control.trainDir, control.correctionDir);

  if (control.sensorFile != "")
    readSensorFile(db, control.sensorFile);

  if (! db.select("ALL", 0, 100))
  {
    cout << "No trains selected" << endl;
    exit(0);
  }
}


unsigned lookupMatchRank(
  const Database& db,
  const vector<Alignment>& matches,
  const string& tag)
{
  const unsigned tno = static_cast<unsigned>(db.lookupTrainNumber(tag));
  for (unsigned i = 0; i < matches.size(); i++)
  {
    if (matches[i].trainNo == tno)
      return i;
  }
  return numeric_limits<unsigned>::max();
}

