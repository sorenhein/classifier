#include <iostream>
#include <iomanip>
#include <string>

#include "read.h"
#include "Database.h"
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

