#include <iostream>
#include <iomanip>
#include <string>

#include "read.h"
#include "Database.h"
#include "Classifier.h"
#include "SynthTrain.h"
#include "Regress.h"
#include "Disturb.h"
#include "Align.h"
#include "Timer.h"
#include "Stats.h"
#include "print.h"

#include "Metrics.h"

Metrics metrics;

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

  Classifier classifier;
  classifier.setCountry(control.country);
  classifier.setYear(control.year);

  Align align;

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

  vector<PeakTime> synthTimes;
  Regress regress;
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

// cout << "Logging " << bestMatch << endl;
          // statCross.log(trainName, bestMatch);
          statCross.log(trainName, 
            db.lookupTrainName(matches[0].trainNo));

          vector<Alignment> matchesAlign;

          align.bestMatches(synthTimes, db, trainNo,
            matches, 10, matchesAlign);
 /*
 cout << "Speed " << speed << " accel " << accel << " no " << no << endl;
 for (unsigned i = 0; i < matchesAlign.size(); i++)
   cout << i << " " << matchesAlign[i].dist << " " <<
     matchesAlign[i].trainNo << endl;
cout << endl;
*/

          // Take anything with a reasonable range of the best few
          // and regress these rigorously.
          // As soon as the indel's alone exceed the distance, we
          // can drop these.  We can use that to prune matchesAlign.

          statCross2.log(trainName, 
            db.lookupTrainName(matchesAlign[0].trainNo));

          Alignment bestAlign;
          regress.bestMatch(synthTimes, db, order,
            matchesAlign, bestAlign, motionEstimate);

          statCross3.log(trainName, 
            db.lookupTrainName(bestAlign.trainNo));

          continue;


// TrainFound trainFound;
// classifier.classify(synthTimes, db, trainFound);

          /*
          const unsigned l = perfectPositions.size();
          vector<double> x(l), y(l), coeffs(l);
          for (unsigned i = 0; i < l; i++)
          {
            y[i] = perfectPositions[i].pos;
            x[i] = synthTimes[i].time;
          }
          pol.fitIt(x, y, order, coeffs);

          timer.stop();

          motionEstimate[0] = coeffs[0];
          motionEstimate[1] = coeffs[1];
          motionEstimate[2] = 2. * coeffs[2];
          // As the regression estimates 0.5 * a in the physics formula.

          // printCorrelation(motionActual, motionEstimate);

          double residuals = 0.;
          // TODO: Calculate residuals, or find them in code
          stats.log(trainName, motionActual,
            trainName, motionEstimate, residuals);
          */
        }
      }
    }
  }

cout << "Count " << countAll << endl;
cout << "Bad   " << countBad << endl;

metrics.printCSV("metrics.csv");

  statCross.printCountCSV("classify.csv");
  statCross2.printCountCSV("classify2.csv");
  statCross3.printCountCSV("classify3.csv");

  stats.printCrossCountCSV(control.crossCountFile);
  stats.printCrossPercentCSV(control.crossPercentFile);
  stats.printOverviewCSV(control.overviewFile);
  stats.printDetailsCSV(control.detailFile);


  cout << "Time " << timer.str(2) << endl;

}


    // classifier.classify(perfectPeaks, db, trainFound2);
