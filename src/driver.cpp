#include <iostream>
#include <iomanip>
#include <string>

#include "read.h"
#include "Database.h"
#include "Classifier.h"
#include "SynthTrain.h"
#include "Disturb.h"
#include "Timer.h"
#include "Stats.h"
#include "print.h"

#include "regress/PolynomialRegression.h"

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

  vector<PeakSample> synthTimes;
  PolynomialRegression pol;
  const int order = 2;
  
  vector<double> motionActual;
  motionActual.resize(order+1);
  motionActual[0] = 0.; // Offset in m

  vector<double> motionEstimate;
  motionEstimate.resize(order+1);

  Stats stats;
  Timer timer;

  for (auto& trainName: db)
  {
    cout << "Train " << trainName << endl;

    vector<PeakPos> perfectPositions;
    if (! db.getPerfectPeaks(trainName, perfectPositions))
      cout << "Bad perfect positions" << endl;

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
          // As ... 0.5 * a * t^2 

          // printCorrelation(motionActual, motionEstimate);

          double residuals = 0.;
          // TODO: Calculate residuals, or find them in code
          stats.log(trainName, motionActual,
            trainName, motionEstimate, residuals);
        }
      }
    }
  }

  stats.printCrossCountCSV(control.crossCountFile);
  stats.printCrossPercentCSV(control.crossPercentFile);
  stats.printOverviewCSV(control.overviewFile);
  stats.printDetailsCSV(control.detailFile);

  cout << "Time " << timer.str(2) << endl;

}


    // classifier.classify(perfectPeaks, db, trainFound2);
