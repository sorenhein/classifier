#include <iostream>
#include <iomanip>
#include <string>
#include <limits>

#include "args.h"
#include "read.h"
#include "Trace.h"
#include "Database.h"
#include "TraceDB.h"
#include "SynthTrain.h"
#include "Regress.h"
#include "Disturb.h"
#include "Align.h"
#include "Timers.h"
#include "Stats.h"
#include "CompStats.h"
#include "Except.h"
#include "print.h"

using namespace std;

Log logger;
Timers timers;


void setup(
  int argc, 
  char * argv[],
  Control& control,
  Database& db,
  Disturb& disturb);

unsigned lookupMatchRank(
  const Database& db,
  const vector<Alignment>& matches,
  const string& tag);

int main(int argc, char * argv[])
{
  Control control;
  Database db;
  Disturb disturb;
  setup(argc, argv, control, db, disturb);

  SynthTrain synth;
  Align align;
  Regress regress;

  vector<PeakPos> perfectPositions;
  vector<PeakTime> synthTimes;
  vector<Alignment> matchesAlign;
  Alignment bestAlign;
  
  const unsigned order = 2;
  vector<double> motionActual(order+1);
  vector<double> motionEstimate(order+1);
  motionActual[0] = 0.; // Offset in m

  Stats stats;

  if (control.inputFile != "")
  {
    // This was an early attempt to read in peaks from the current
    // system.  I moved to generate my own peaks.

    vector<InputEntry> actualList;
    if (! readInputFile(control.inputFile, actualList))
    {
      cout << "Could not read " << control.inputFile << endl;
      exit(0);
    }

    for (auto& actualEntry: actualList)
    {
      align.bestMatches(actualEntry.actual, db, "DEU", 10, 
        control.verboseAlignMatches, matchesAlign);

      if (matchesAlign.size() == 0)
      {
        cout << "number " << actualEntry.number << 
          ", date " << actualEntry.date <<
          ", time " << actualEntry.time <<
          ", axles: " << actualEntry.actual.size() << endl;
        cout << "NO MATCH\n\n";
        continue;
      }

      regress.bestMatch(actualEntry.actual, db, order,
        control, matchesAlign, bestAlign, motionEstimate);

      cout << "number " << actualEntry.number << 
        ", date " << actualEntry.date <<
        ", time " << actualEntry.time << 
        ", axles: " << actualEntry.actual.size() << endl;
      for (auto& match: matchesAlign)
      {
        cout << setw(24) << left << db.lookupTrainName(match.trainNo) << 
          setw(10) << fixed << setprecision(2) << match.dist <<
          setw(10) << fixed << setprecision(2) << match.distMatch <<
          setw(8) << match.numAdd <<
          setw(8) << match.numDelete << endl;
      }
      cout << endl;

      // TODO print result, scores, axle counts, ...
      // Count axles that stick out, too small or too large?
    }
  }
  else if (control.traceDir != "")
  {
    // This extracts peaks and then extracts train types from peaks.

    TraceDB traceDB;
    readTraceTruth(control.truthFile, db, traceDB);

    vector<string> datfiles;
    getFilenames(control.traceDir, datfiles, control.pickFileString);

    Trace trace;
    vector<PeakTime> times;

    CompStats sensorStats, trainStats;

    for (auto& fname: datfiles)
    {
      cout << "File " << fname << ":\n\n";
      const string sensor = traceDB.lookupSensor(fname);
      const string country = db.lookupSensorCountry(sensor);
      const string trainTrue = traceDB.lookupTrueTrain(fname);
      
      try
      {
        trace.read(fname, true);
        trace.detect(control);
        trace.write(control);

        trace.getTrace(times);
        align.bestMatches(times, db, country, 10, 
          control.verboseAlignMatches, matchesAlign);


        if (matchesAlign.size() == 0)
        {
          traceDB.log(fname, matchesAlign, times.size());
          sensorStats.log(sensor, 10, 1000.);
          trainStats.log(trainTrue, 10, 1000.);
          continue;
        }

        regress.bestMatch(times, db, order, control, matchesAlign,
          bestAlign, motionEstimate);

        traceDB.log(fname, matchesAlign, times.size());

        const string trainDetected = db.lookupTrainName(bestAlign.trainNo);
        const unsigned rank = lookupMatchRank(db, matchesAlign, trainTrue);

        sensorStats.log(sensor, rank, bestAlign.distMatch);
        trainStats.log(trainTrue, rank, bestAlign.distMatch);
      }
      catch (Except& ex)
      {
        ex.print(cout);
        sensorStats.log(sensor, 10, 1000.);
        trainStats.log(trainTrue, 10, 1000.);
      }
    }

    traceDB.printCSV(control.summaryFile, control.summaryAppendFlag, db);

    sensorStats.print("sensorstats.txt", "Sensor");
    trainStats.print("trainstats.txt", "Train");
  }
  else
  {
    // This generates synthetic, noisy peaks and classifies them.

    for (auto& trainName: db)
    // string trainName = "ICE1_DEU_56_N";
    // string trainName = "MERIDIAN_DEU_22_N";
    {
      cout << "Train " << trainName << endl;
      const int trainNoI = db.lookupTrainNumber(trainName);
      if (trainNoI == -1)
      {
        cout << "Bad train name\n";
        exit(0);
      }
      const unsigned trainNo = static_cast<unsigned>(trainNoI);

      // Actually a train might run in more than one country.
      const string country = db.lookupTrainCountry(trainNo);

      if (! db.getPerfectPeaks(trainName, perfectPositions))
        cout << "Bad perfect positions" << endl;
// cout << "Input positions " << trainName << "\n";
// printPeakPosCSV(perfectPositions, 1);

      for (double speed = control.speedMin; 
          speed <= control.speedMax + 0.1 * control.speedStep; 
          speed += control.speedStep)
      {
        motionActual[1] = speed;

        for (double accel = control.accelMin; 
            accel <= control.accelMax + 0.1 * control.accelStep; 
            accel += control.accelStep)
        {
          motionActual[2] = accel;

          for (int no = 0; no < control.simCount; no++)
          {
            if (! synth.disturb(perfectPositions, disturb, synthTimes, 
              0., speed, accel))
            {
              continue;
            }

            align.bestMatches(synthTimes, db, country, 10, 
              control.verboseAlignMatches, matchesAlign);

            if (matchesAlign.size() == 0)
            {
              stats.log(trainName, motionActual,
                "UNKNOWN", motionActual, 0.);
              continue;
            }

            regress.bestMatch(synthTimes, db, order,
              control, matchesAlign, bestAlign, motionEstimate);

            stats.log(trainName, motionActual,
              db.lookupTrainName(bestAlign.trainNo),
              motionEstimate, bestAlign.distMatch);
          }
        }
      }
    }

    stats.printCrossCountCSV(control.crossCountFile);
    stats.printCrossPercentCSV(control.crossPercentFile);
    stats.printOverviewCSV(control.overviewFile);
    stats.printDetailsCSV(control.detailFile);
    stats.printQuality();
  }

  cout << timers.str(2) << endl;
}


void setup(
  int argc, 
  char * argv[],
  Control& control,
  Database& db,
  Disturb& disturb)
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
  readTrainFiles(db, control.trainDir);

  if (control.sensorFile != "")
    readSensorFile(db, control.sensorFile);

  if (! disturb.readFile(control.disturbFile))
  {
    cout << "Bad disturbance file" << endl;
    exit(0);
  }

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
  const unsigned tno = db.lookupTrainNumber(tag);
  for (unsigned i = 0; i < matches.size(); i++)
  {
    if (matches[i].trainNo == tno)
      return i;
  }
  return numeric_limits<unsigned>::max();
}

