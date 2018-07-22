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

  vector<PeakTime> synthTimes;
  const unsigned order = 2;
  
  vector<double> motionActual;
  motionActual.resize(order+1);
  motionActual[0] = 0.; // Offset in m

  vector<double> motionEstimate;
  motionEstimate.resize(order+1);

  Stats stats;
  StatCross statCross;
  StatCross statCross2;
  StatCross statCross3;
  Timer timer1, timer2, timer3, timer4;

int countAll = 0;
int countBad = 0;

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

    vector<PeakPos> perfectPositions;
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
          timer1.start();

          if (! synth.disturb(perfectPositions, disturb, synthTimes, 
            0., speed, accel))
          {
            continue;
          }

          timer1.stop();

          vector<Alignment> matchesAlign;

          timer2.start();
          align.bestMatches(synthTimes, db, trainNo, 10, matchesAlign);
          timer2.stop();
bool found = false;
for (unsigned i = 0; ! found && i < matchesAlign.size(); i++)
{
  if (matchesAlign[i].trainNo == trainNo)
    found = true;
}
countAll++;
if (! found)
  countBad++;


          // Take anything with a reasonable range of the best few
          // and regress these rigorously.
          // As soon as the indel's alone exceed the distance, we
          // can drop these.  We can use that to prune matchesAlign.

          if (matchesAlign.size() == 0)
          {
            // TODO If we eliminate clusters, then this can't happen
            // anymore which would be the right thing.  For now.
            statCross2.log(trainName, "UNKNOWN");
cout << "UNKNOWN\n";
            continue;
          }

          statCross2.log(trainName, 
            db.lookupTrainName(matchesAlign[0].trainNo));

          Alignment bestAlign;
          timer4.start();
          regress.bestMatch(synthTimes, db, order,
            matchesAlign, bestAlign, motionEstimate);
          timer4.stop();

          statCross3.log(trainName, 
            db.lookupTrainName(bestAlign.trainNo));

/*
if (! errFlag && trainName == "ICE4_DEU_28_N" && bestAlign.trainNo == 14)
{
 errFlag = true;
cout << "speed " << speed << ", accel " << accel << endl;
cout << "Input positions " << "ICET_DEU_28_N" << "\n";
printPeakTimeCSV(synthTimes, 3);
}
*/
          double residuals = 0.;
          // TODO: Calculate residuals, or find them in code
          stats.log(trainName, motionActual,
            trainName, motionEstimate, residuals);
        }
      }
    }
  }

cout << "Count " << countAll << endl;
cout << "Bad   " << countBad << endl;

  statCross.printCountCSV("classify.csv");
  statCross2.printCountCSV("classify2.csv");
  statCross3.printCountCSV("classify3.csv");
  statCross3.printQuality();

  stats.printCrossCountCSV(control.crossCountFile);
  stats.printCrossPercentCSV(control.crossPercentFile);
  stats.printOverviewCSV(control.overviewFile);
  stats.printDetailsCSV(control.detailFile);


  cout << "Time synth   " << timer1.str(2) << endl;
  cout << "Time match   " << timer2.str(2) << endl;
  cout << "Time regress " << timer4.str(2) << endl;

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

