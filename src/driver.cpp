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


void printPeaks(
  const vector<Peak>& peaks,
  const int level);


int main(int argc, char * argv[])
{
  UNUSED(argc);
  UNUSED(argv);

  const int sampleRate = SAMPLE_RATE;

  Database db;
  readCarFiles(db, "../data/cars");
  readTrainFiles(db, "../data/trains");
  db.setSampleRate(sampleRate);
cout << "Read all" << endl;

  Classifier classifier;
  classifier.setSampleRate(sampleRate);
  classifier.setCountry("DEU");
  classifier.setYear(2018);
cout << "Set up classifier" << endl;

  Disturb disturb;
  if (! disturb.readFile("../data/disturbances/case1.txt"))
    cout << "Bad disturbance file" << endl;

cout << "Read disturbance" << endl;

  SynthTrain synth;
  synth.setSampleRate(sampleRate);
  vector<Peak> perfectPeaks;
  if (! db.getPerfectPeaks("ICE1_DEU_56_N", perfectPeaks))
    cout << "Bad perfect peaks" << endl;

cout << "Got perfect peaks in mm" << endl;
printPeaks(perfectPeaks, 1);

const int offset = 10000;
vector<Peak> perfectPeakTimes;
Peak peak;
peak.value = 1.f;
const float speed = 2.f;
// perfectPeaks are in mm
// sampleRate is in Hz
// speed is in m/s.
const float factor = static_cast<float>(sampleRate) / (1000.f * speed);
cout << "factor " << factor << endl;

for (auto& it: perfectPeaks)
{
  peak.sampleNo = offset + static_cast<int>(factor * it.sampleNo);
  perfectPeakTimes.push_back(peak);
}
cout << "Got perfect peaktimes in samples" << endl;
printPeaks(perfectPeakTimes, 1);

vector<Peak> synthP;
float newSpeed0;
synth.disturb(perfectPeaks, disturb, synthP, 
  2.0f, 2.0f, 0.00001f, newSpeed0);
cout << "Got disturbed peaks " << endl;
printPeaks(synthP, 2);

const unsigned l = perfectPeaks.size();
vector<double> x(l), y(l), coeffs(l);
for (unsigned i = 0; i < l; i++)
{
  y[i] = static_cast<double>(perfectPeakTimes[i].sampleNo);
  x[i] = static_cast<double>(synthP[i].sampleNo);
}

PolynomialRegression pol;
const int order = 2;
pol.fitIt(x, y, order, coeffs);

for (unsigned i = 0; i <= order; i++)
  cout << "i " << i << ", coeff " << coeffs[i] << endl;

cout << "Orig speed " << 2.f << endl;
cout << "New speed " << newSpeed0 << endl;
cout << "Ratio " << newSpeed0 / 2.f << endl;



exit(0);

    TrainFound trainFound2;
    classifier.classify(perfectPeaks, db, trainFound2);
cout << "Classified " << endl;

  Stats stats;

  for (unsigned i = 0; i < 1; i++)
  {
    vector<Peak> synthPeaks;
    float newSpeed;
    synth.disturb(perfectPeaks, disturb, synthPeaks, 
      60.0f, 20.0f, 250.0f, newSpeed);
cout << "Got disturbed peaks " << i << endl;
// printPeaks(synthPeaks, 2);

    TrainFound trainFound;
    classifier.classify(synthPeaks, db, trainFound);
cout << "Classified " << i << endl;

    const string trainName = db.lookupTrainName(trainFound.dbTrainNo);
cout << "Got train name " << i << endl;

    stats.log("ICE1", trainName);
cout << "Logged stats " << i << endl;

    // TODO Also stats on speed accuracy
  }

  stats.print("output.txt");
}


void printPeaks(
  const vector<Peak>& peaks,
  const int level)
{
  for (unsigned i = 0; i < peaks.size(); i++)
    cout << peaks[i].sampleNo << ";" << level << endl;
    // cout << setw(4) << i << setw(12) << peaks[i].sampleNo << endl;
  cout << endl;
}

