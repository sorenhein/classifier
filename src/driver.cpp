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

#include "regress/PolynomialRegression.h"

using namespace std;

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))
#define SIM_NUMBER 10


void printPeakPosCSV(
  const vector<PeakPos>& peaks,
  const int level);

void printPeakSampleCSV(
  const vector<PeakSample>& peaks,
  const int level);

void printDataHeader();

void printDataLine(
  const string& text,
  const double actual,
  const double estimate);


int main(int argc, char * argv[])
{
  UNUSED(argc);
  UNUSED(argv);

  Database db;
  readCarFiles(db, "../data/cars");
  readTrainFiles(db, "../data/trains");

  Classifier classifier;
  classifier.setCountry("DEU");
  classifier.setYear(2018);

  Disturb disturb;
  if (! disturb.readFile("../data/disturbances/case1.txt"))
    cout << "Bad disturbance file" << endl;

  if (! db.select("ALL", 0, 100))
  {
    cout << "No trains selected" << endl;
    exit(0);
  }

  SynthTrain synth;

  vector<PeakSample> synthP;
  PolynomialRegression pol;
  const int order = 2;
  // const double offset = 1.;
  
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

// cout << "Got perfect positions in mm, " << perfectPositions.size() << endl;
// printPeakPosCSV(perfectPositions, 1);

    for (double speed = 5.; speed <= 75.; speed += 5.)
    // for (double speed = 50.; speed <= 55.; speed += 10.)
    // for (double speed = 20.; speed <= 50.; speed += 20.)
    // for (double speed = 40.; speed <= 50.; speed += 20.)
    {
      motionActual[1] = speed;

      for (double accel = -0.5; accel <= 0.55; accel += 0.1)
      // for (double accel = 0.; accel <= 0.05; accel += 0.3)
      // for (double accel = 0.3; accel <= 0.35; accel += 0.3)
      {
        motionActual[2] = accel;

        for (unsigned no = 0; no < SIM_NUMBER; no++)
        {
          timer.start();

          if (! synth.disturb(perfectPositions, disturb, synthP, 
            0., speed, accel))
          {
            continue;
          }

// cout << "Got synth times, " << synthP.size() << endl;
// printPeakSampleCSV(synthP, 2);

          const unsigned l = perfectPositions.size();
          vector<double> x(l), y(l), coeffs(l);
          for (unsigned i = 0; i < l; i++)
          {
            y[i] = perfectPositions[i].pos;
            x[i] = synthP[i].time;
          }

          pol.fitIt(x, y, order, coeffs);

          timer.stop();

          motionEstimate[0] = coeffs[0];
          motionEstimate[1] = coeffs[1];
          motionEstimate[2] = 2. * coeffs[2];

          // for (unsigned i = 0; i <= order; i++)
            // cout << "i " << i << ", est " << motionEstimate[i] << endl;

          // printDataHeader();
          // printDataLine("Offset", 0., motionEstimate[0]);
          // printDataLine("Speed", speed, motionEstimate[1]);
          // printDataLine("Accel", accel, motionEstimate[2]);

          double residuals = 0.;
          stats.log(trainName, motionActual,
            trainName, motionEstimate, residuals);
        }
      }
    }
  }

  cout << "Time " << timer.str(2) << endl;

// cout << "Done with loop" << endl;
  stats.printCrossCountCSV("crosscount.csv");
// cout << "Done with 1" << endl;
  stats.printCrossPercentCSV("crosspercent.csv");
// cout << "Done with 2" << endl;

  stats.printOverviewCSV("overview.csv");
// cout << "Done with 3" << endl;
  stats.printDetailsCSV("details.csv");
// cout << "Done with 4" << endl;

    // classifier.classify(perfectPeaks, db, trainFound2);
    // Stats stats;
    // stats.log("ICE1", trainName);
    // stats.print("output.txt");
}


void printPeakPosCSV(
  const vector<PeakPos>& peaks,
  const int level)
{
  for (unsigned i = 0; i < peaks.size(); i++)
    cout << peaks[i].pos << ";" << level << endl;
  cout << endl;
}


void printPeakSampleCSV(
  const vector<PeakSample>& peaks,
  const int level)
{
  for (unsigned i = 0; i < peaks.size(); i++)
    cout << peaks[i].time << ";" << level << endl;
  cout << endl;
}


void printDataHeader()
{
  cout << setw(8) << left << "" <<
    setw(10) << right << "Actual" <<
    setw(10) << right << "Est" <<
    setw(10) << right << "Error" << endl;
}


void printDataLine(
  const string& text,
  const double actual,
  const double estimate)
{
  double dev;
  if (actual == 0.)
    dev = 0.;
  else
    dev = 100. * abs((actual - estimate) / actual);

  cout << setw(8) << left << text <<
    setw(10) << right << fixed << setprecision(2) << actual <<
    setw(10) << right << fixed << setprecision(2) << estimate <<
    setw(9) << right << fixed << setprecision(1) << dev << "%" << endl;
}

