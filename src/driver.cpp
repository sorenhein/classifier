#include <iostream>
#include <iomanip>
#include <string>

#include "read.h"
#include "Trace.h"
#include "Database.h"
#include "TraceDB.h"
#include "SynthTrain.h"
#include "Regress.h"
#include "Disturb.h"
#include "Align.h"
#include "Timer.h"
#include "Stats.h"
#include "print.h"

using namespace std;


void setup(
  int argc, 
  char * argv[],
  Control& control,
  Database& db,
  Disturb& disturb);


int main(int argc, char * argv[])
{
  /*
  Trace trace0;
  const string fn = "../../../mini_dataset_v012/data/sensors/062493/raw/20180625_114917_062493_001_channel1.dat";
  trace0.read(fn, true);

      vector<PeakTime> times0;
      trace0.getTrace(times0);

      Control control0;
      Database db0;
      Disturb disturb0;
      setup(argc, argv, control0, db0, disturb0);

      TraceDB traceDB0;
      readTraceTruth(control0.truthFile, db0, traceDB0);

      const string sensor0 = traceDB0.lookupSensor(fn);
      const string country0 = db0.lookupCountry(sensor0);
      unsigned trainNoExample0;
      if (country0 == "DEU")
        trainNoExample0 = 0;
      else if (country0 == "SWE")
        trainNoExample0 = 30;
      else
      {
        cout << "Could not recognize country '" << country0 << "'\n";
        exit(0);
      }

      Align align0;
      vector<Alignment> matchesAlign0;
      align0.bestMatches(times0, db0, trainNoExample0, 10, matchesAlign0);

  exit(0);
  */
  // vector<PeakTime> times;
  // trace.getTrace(times);

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
  Timer timerSynth, timerAlign, timerRegress;

/*
vector<PeakPos> tmppos;
if (! db.getPerfectPeaks("ICE4_DEU_28_N", tmppos))
  cout << "Bad perfect positions" << endl;
cout << "Input positions " << "ICE4_DEU_28_N" << "\n";
printPeakPosCSV(tmppos, 1);

if (! db.getPerfectPeaks("ICET_DEU_28_N", tmppos))
  cout << "Bad perfect positions" << endl;
cout << "Input positions " << "ICET_DEU_28_N" << "\n";
printPeakPosCSV(tmppos, 2);

bool errFlag = false;
*/

  if (control.inputFile != "")
  {
    vector<InputEntry> actualList;
    if (! readInputFile(control.inputFile, actualList))
    {
      cout << "Could not read " << control.inputFile << endl;
      exit(0);
    }

    for (auto& actualEntry: actualList)
    {
      timerAlign.start();
      // Assume Germany.
      align.bestMatches(actualEntry.actual, db, 0, 10, matchesAlign);
      timerAlign.stop();

      if (matchesAlign.size() == 0)
      {
        cout << "number " << actualEntry.number << 
          ", date " << actualEntry.date <<
          ", time " << actualEntry.time <<
          ", axles: " << actualEntry.actual.size() << endl;
        cout << "NO MATCH\n\n";
        continue;
      }

      timerRegress.start();
      regress.bestMatch(actualEntry.actual, db, order,
        matchesAlign, bestAlign, motionEstimate);
      timerRegress.stop();

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
    TraceDB traceDB;
    readTraceTruth(control.truthFile, db, traceDB);

    vector<string> datfiles;
    getFilenames(control.traceDir, datfiles);
    Trace trace;
    vector<PeakTime> times;

    // ofstream tout("trans.csv", ios_base::app);
    // tout << "Sensor;" << trace.strTransientHeaderCSV() << "\n";

    for (auto& fname: datfiles)
    {
      cout << "File " << fname << ":" << endl;
      trace.read(fname, true);
      trace.getTrace(times);

      // TEMP
      // tout << fname << ";" << trace.strTransientCSV() << "\n";
      // trace.writeTransient();

      // trace.writeQuietBack();
      // trace.writeQuietFront();

      trace.writeSegActive();
      // continue;

      const string sensor = traceDB.lookupSensor(fname);
      const string country = db.lookupCountry(sensor);
      unsigned trainNoExample;
      if (country == "DEU")
        trainNoExample = 0;
      else if (country == "SWE")
        trainNoExample = 30;
      else
      {
        cout << "Could not recognize country '" << country << "'\n";
        continue;
      }

      timerAlign.start();
      align.bestMatches(times, db, trainNoExample, 10, matchesAlign);
      timerAlign.stop();

      if (matchesAlign.size() == 0)
      {
        cout << "NO MATCH\n\n";
      }
      else
      {
        timerRegress.start();
        regress.bestMatch(times, db, order,
          matchesAlign, bestAlign, motionEstimate);
        timerRegress.stop();

        for (auto& match: matchesAlign)
        {
          cout << setw(24) << left << db.lookupTrainName(match.trainNo) << 
            setw(10) << fixed << setprecision(2) << match.dist <<
            setw(10) << fixed << setprecision(2) << match.distMatch <<
            setw(8) << match.numAdd <<
            setw(8) << match.numDelete << endl;
        }
        cout << endl;
      }  
      traceDB.log(fname, matchesAlign, times.size());
    }

    traceDB.printCSV("comp.csv", db);
    // tout.close();
  }
  else
  {
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
            timerSynth.start();
            if (! synth.disturb(perfectPositions, disturb, synthTimes, 
              0., speed, accel))
            {
              continue;
            }
            timerSynth.stop();

            timerAlign.start();
            align.bestMatches(synthTimes, db, trainNo, 10, matchesAlign);
            timerAlign.stop();

            if (matchesAlign.size() == 0)
            {
              stats.log(trainName, motionActual,
                "UNKNOWN", motionActual, 0.);
              continue;
            }

            timerRegress.start();
            regress.bestMatch(synthTimes, db, order,
              matchesAlign, bestAlign, motionEstimate);
            timerRegress.stop();

            stats.log(trainName, motionActual,
              db.lookupTrainName(bestAlign.trainNo),
              motionEstimate, bestAlign.distMatch);

/*
if (! errFlag && trainName == "ICE4_DEU_28_N" && bestAlign.trainNo == 14)
{
 errFlag = true;
cout << "speed " << speed << ", accel " << accel << endl;
cout << "Input positions " << "ICET_DEU_28_N" << "\n";
printPeakTimeCSV(synthTimes, 3);
}
*/
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

  cout << "Time synth   " << timerSynth.str(2) << endl;
  cout << "Time align   " << timerAlign.str(2) << endl;
  cout << "Time regress " << timerRegress.str(2) << endl;
}


void setup(
  int argc, 
  char * argv[],
  Control& control,
  Database& db,
  Disturb& disturb)
{
  if (argc != 2)
  {
    cout << "Usage: ./driver control_file\n";
    exit(0);
  }

  if (! readControlFile(control, string(argv[1])))
  {
    cout << "Bad control file" << string(argv[1]) << endl;
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

