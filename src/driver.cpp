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

using namespace std;

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


void printPeaks(
  const vector<Peak>& peaks,
  const int level);


int main(int argc, char * argv[])
{
  UNUSED(argc);
  UNUSED(argv);

  Database db;
  readCarFiles(db, "../data/cars");
  readTrainFiles(db, "../data/trains");
  db.setSampleRate(2000);
cout << "Read all" << endl;

  Classifier classifier;
  classifier.setSampleRate(2000);
  classifier.setCountry("DEU");
  classifier.setYear(2018);
cout << "Set up classifier" << endl;

  Disturb disturb;
  if (! disturb.readFile("../data/disturbances/case1.txt"))
    cout << "Bad disturbance file" << endl;

cout << "Read disturbance" << endl;

  SynthTrain synth;
  synth.setSampleRate(2000);
  vector<Peak> perfectPeaks;
  if (! db.getPerfectPeaks("ICE1_DEU_56_N", perfectPeaks, 7.2f, 0))
    cout << "Bad perfect peaks" << endl;

cout << "Got perfect peaks" << endl;
printPeaks(perfectPeaks, 1);

vector<Peak> synthP;
int newSpeed0;
synth.disturb(perfectPeaks, disturb, synthP, 
  60, 20, 250, newSpeed0);
cout << "Got disturbed peaks " << endl;
printPeaks(synthP, 2);


exit(0);

    TrainFound trainFound2;
    classifier.classify(perfectPeaks, db, trainFound2);
cout << "Classified " << endl;

  Stats stats;

  for (unsigned i = 0; i < 1; i++)
  {
    vector<Peak> synthPeaks;
    int newSpeed;
    synth.disturb(perfectPeaks, disturb, synthPeaks, 
      60, 20, 250, newSpeed);
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

