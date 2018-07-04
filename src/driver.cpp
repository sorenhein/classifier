#include <iostream>
#include <iomanip>
#include <string>

#include "read.h"
#include "Database.h"
#include "Classifier.h"
#include "SynthTrain.h"
#include "Timer.h"
#include "Stats.h"

using namespace std;

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


void printPeaks(const vector<Peak>& peaks);


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

  SynthTrain synth;
  synth.readDisturbance("../data/disturbances/case1.txt");
cout << "Read disturbance" << endl;

  vector<Peak> perfectPeaks;
  db.getPerfectPeaks("ICE1 Refurbishment", "DEU", perfectPeaks, 60);
cout << "Got perfect peaks" << endl;
printPeaks(perfectPeaks);

  Stats stats;

  for (unsigned i = 0; i < 1; i++)
  {
    vector<Peak> synthPeaks;
    int newSpeed;
    synth.disturb(perfectPeaks, synthPeaks, 60, 20, 250, newSpeed);
cout << "Got disturbed peaks " << i << endl;
printPeaks(synthPeaks);

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


void printPeaks(const vector<Peak>& peaks)
{
  for (unsigned i = 0; i < peaks.size(); i++)
    cout << setw(4) << i << setw(12) << peaks[i].sampleNo << endl;
  cout << endl;
}

