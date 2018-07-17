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


int main(int argc, char * argv[])
{
  if (argc != 2)
  {
    cout << "Usage: ./driver control_file\n";
    exit(0);
  }

  Control control;
  if (! readControlFile(control, string(argv[1])))
  {
    cout << "Bad control file" << string(argv[1]) << endl;
    exit(0);
  }

  Database db;
  readCarFiles(db, control.carDir);
  readTrainFiles(db, control.trainDir);

  Disturb disturb;
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

  SynthTrain synth;
  Align align;
  Regress regress;

  vector<PeakTime> synthTimes;
  // PolynomialRegression pol;
  const int order = 2;
  
  vector<double> motionActual;
  motionActual.resize(order+1);
  motionActual[0] = 0.; // Offset in m

  vector<double> motionEstimate;
  motionEstimate.resize(order+1);

  Stats stats;
  StatCross statCross;
  StatCross statCross2;
  StatCross statCross3;
  Timer timer;

int countAll = 0;
int countBad = 0;

  for (auto& trainName: db)
  // string trainName = "ICE1_DEU_56_N";
  // string trainName = "MERIDIAN_DEU_22_N";
  {
    cout << "Train " << trainName << endl;
    const int trainNo = db.lookupTrainNumber(trainName);
    if (trainNo == -1)
    {
      cout << "Bad train name\n";
      exit(0);
    }

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
          timer.start();

          if (! synth.disturb(perfectPositions, disturb, synthTimes, 
            0., speed, accel))
          {
            continue;
          }
// cout << "Synth no. " << no << "\n";
// printPeakTimeCSV(synthTimes, no+2);

          Clusters clusters;
          vector<HistMatch> matches;
          clusters.bestMatches(synthTimes, db, trainNo, 3, matches);
bool found = false;
for (unsigned i = 0; ! found && i < matches.size(); i++)
{
  if (matches[i].trainNo == trainNo)
    found = true;
}
countAll++;
if (! found)
  countBad++;

          statCross.log(trainName, 
            db.lookupTrainName(matches[0].trainNo));

          vector<Alignment> matchesAlign;

          align.bestMatches(synthTimes, db, matches, 10, matchesAlign);

          statCross2.log(trainName, 
            db.lookupTrainName(matchesAlign[0].trainNo));

          Alignment bestAlign;
          regress.bestMatch(synthTimes, db, order,
            matchesAlign, bestAlign, motionEstimate);

          statCross3.log(trainName, 
            db.lookupTrainName(bestAlign.trainNo));

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

  stats.printCrossCountCSV(control.crossCountFile);
  stats.printCrossPercentCSV(control.crossPercentFile);
  stats.printOverviewCSV(control.overviewFile);
  stats.printDetailsCSV(control.detailFile);


  cout << "Time " << timer.str(2) << endl;

}

