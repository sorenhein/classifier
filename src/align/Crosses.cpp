#include <iostream>
#include <iomanip>
#include <sstream>

#include "Align.h"

#include "../database/TrainDB.h"

#include "Crosses.h"


Crosses::Crosses()
{
  Crosses::reset();
}


Crosses::~Crosses()
{
}


void Crosses::reset()
{
  crosses.clear();
}


void Crosses::log(const TrainDB& trainDB)
{
  Crosses::reset();
  Align align;
  Alignment match;
  vector<float> scaledPeaks;

  for (auto& obsTrain: trainDB)
  {
    const unsigned obsTrainNo = trainDB.lookupNumber(obsTrain);
    const PeaksInfo& obsInfo = trainDB.getRefInfo(obsTrainNo);

cout << "SETTING0 " << obsTrain << endl;
    crosses[obsTrain] = map<string, Alignment>();
cout << "SET0 " << obsTrain << endl;

    for (auto& refTrain: trainDB)
    {
cout << "P0 " << obsTrain << ", " << refTrain << endl;
      match.trainName = refTrain;
      match.trainNo = static_cast<unsigned>(trainDB.lookupNumber(refTrain));
      match.numCars = trainDB.numCars(match.trainNo);
      match.numAxles = trainDB.numAxles(match.trainNo);

cout << "P1 " << obsTrain << ", " << refTrain << endl;
      if (! align.trainMightFitGeometrically(obsInfo, match))
        continue;

cout << "P2 " << obsTrain << ", " << refTrain << endl;
      const PeaksInfo& refInfo = trainDB.getRefInfo(match.trainNo);

cout << "P3 " << obsTrain << ", " << refTrain << endl;
      if (! align.scalePeaks(refInfo, obsInfo, match, scaledPeaks))
        continue;

cout << "SETTING " << obsTrain << ", " << refTrain << endl;
      crosses[obsTrain][refTrain] = match;
cout << "SET" << obsTrain << ", " << refTrain << endl;
    }
  }
}


string Crosses::strTrue() const
{
  stringstream ss;
  const string s = "STATS: CROSSES";
  ss << s << "\n" << string(s.size(), '-') << "\n\n";

  for (auto& obs: crosses)
  {
    ss << "Observed train: " << obs.first << "\n";
    for (auto &ref: obs.second)
      ss << ref.second.str();
      // ss << ref.second.strDeviation();
    ss << "\n";
  }

  return ss.str();
}

