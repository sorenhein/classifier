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


void printPeaks(
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

  const int sampleRate = SAMPLE_RATE;

  Database db;
  readCarFiles(db, "../data/cars");
  readTrainFiles(db, "../data/trains");
  db.setSampleRate(sampleRate);

  Classifier classifier;
  classifier.setSampleRate(sampleRate);
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
  synth.setSampleRate(sampleRate);

  vector<PeakSample> synthP;
  PolynomialRegression pol;
  const int order = 2;
  const double offset = 1.;

  for (auto& trainName: db)
  {
cout << "Train " << trainName << endl;

    vector<PeakPos> perfectPositions;
    if (! db.getPerfectPeaks(trainName, perfectPositions))
      cout << "Bad perfect positions" << endl;

// cout << "Got perfect peaks in mm" << endl;
// printPeaks(perfectPeaks, 1);

    // for (double speed = 20.; speed <= 290.; speed += 20.)
    for (double speed = 20.; speed <= 50.; speed += 20.)
    {
      // for (unsigned accel = -0.3f; accel <= 0.35f; accel += 0.1f)
      for (double accel = -0.3; accel <= 0.35; accel += 0.3)
      {

        for (unsigned no = 0; no < SIM_NUMBER; no++)
        {
          synth.disturb(perfectPositions, disturb, synthP, 
            offset, speed, accel);

// cout << "Got disturbed peaks " << endl;
// printPeaks(synthP, 2);

          const unsigned l = perfectPositions.size();
          vector<double> x(l), y(l), coeffs(l);
          for (unsigned i = 0; i < l; i++)
          {
            y[i] = static_cast<double>(perfectPositions[i].pos);
            x[i] = static_cast<double>(synthP[i].no);
          }

          pol.fitIt(x, y, order, coeffs);

          // for (unsigned i = 0; i <= order; i++)
            // cout << "i " << i << ", coeff " << coeffs[i] << endl;

          printDataHeader();
          printDataLine("Offset", offset, coeffs[0]);
          printDataLine("Speed", speed, 
            sampleRate * coeffs[1] / 1000.);
          printDataLine("Accel", accel, 
            2. * sampleRate * sampleRate * coeffs[2] / 1000.);
          cout << endl;
        }
      }
    }
  }

    // classifier.classify(perfectPeaks, db, trainFound2);
    // Stats stats;
    // stats.log("ICE1", trainName);
    // stats.print("output.txt");
}


void printPeaksCSV(
  const vector<PeakSample>& peaks,
  const int level)
{
  for (unsigned i = 0; i < peaks.size(); i++)
    cout << peaks[i].no << ";" << level << endl;
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

