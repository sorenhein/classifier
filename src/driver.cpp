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


int main(int argc, char * argv[])
{
  UNUSED(argc);
  UNUSED(argv);

  Database db;
  readCarFiles(db, "../data/cars");
  readTrainFiles(db, "../data/trains");

  Classifier classifier;
  classifier.setSampleRate(2000);
  classifier.setCountry("DEU");
  classifier.setYear(2018);

  SynthTrain synth;
  synth.readDisturbance("../data/disturbances/case1.txt");

  vector<Peak> perfectPeaks;
  db.getPerfectPeaks("ICE1 Refurbishment", "DEU", perfectPeaks, 60);

  Stats stats;

  for (unsigned i = 0; i < 100; i++)
  {
    vector<Peak> synthPeaks;
    int newSpeed;
    synth.disturb(perfectPeaks, synthPeaks, 60, newSpeed);

    TrainFound trainFound;
    classifier.classify(synthPeaks, db, trainFound);

    const string trainName = db.lookupTrainName(trainFound.dbTrainNo);

    stats.log("ICE1", trainName);
  }

  stats.print("output.txt");
}

