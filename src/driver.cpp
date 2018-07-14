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

  vector<PeakTime> synthTimes;
  PolynomialRegression pol;
  const int order = 2;
  
  vector<double> motionActual;
  motionActual.resize(order+1);
  motionActual[0] = 0.; // Offset in m

  vector<double> motionEstimate;
  motionEstimate.resize(order+1);

  Stats stats;
  StatCross statCross;
  Timer timer;

  // const string trainName = "ICE1_DEU_56_N";
  for (auto& trainName: db)
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
// cout << "Input positions\n";
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
          double dist = numeric_limits<double>::max();
          string bestMatch = "UNKNOWN";
          for (unsigned numCl = 2; numCl <= 6; numCl++)
          {
            const double dIntraCluster = 
              sqrt(clusters.log(synthTimes, numCl));

            for (auto& refTrain: db)
            {
              const int refTrainNo = db.lookupTrainNumber(refTrain);
              if (! db.TrainsShareCountry(trainNo, refTrainNo))
                continue;

              const Clusters * otherClusters = db.getClusters(refTrainNo,
                numCl);
              if (otherClusters == nullptr)
                continue;

              const double dInterCluster = clusters.distance(* otherClusters);
              const double d = dIntraCluster + dInterCluster;
              if (d < dist)
              {
                dist = d;
                bestMatch = refTrain;
// cout << "Noisy own cluster\n";
// clusters.print();

// cout << "numCl " << numCl << ", " << refTrain << ", dist " << dist << "\n";
// (* otherClusters).print();
              }
            }
          }

// cout << "Logging " << bestMatch << endl;
          statCross.log(trainName, bestMatch);
          continue;


// TrainFound trainFound;
// classifier.classify(synthTimes, db, trainFound);

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
        }
      }
    }
  }

  statCross.printCountCSV("classify.csv");

  stats.printCrossCountCSV(control.crossCountFile);
  stats.printCrossPercentCSV(control.crossPercentFile);
  stats.printOverviewCSV(control.overviewFile);
  stats.printDetailsCSV(control.detailFile);


  cout << "Time " << timer.str(2) << endl;

}


    // classifier.classify(perfectPeaks, db, trainFound2);
