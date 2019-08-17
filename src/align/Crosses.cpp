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

  for (auto& obsTrain: trainDB)
  {
    const unsigned obsTrainNo = trainDB.lookupNumber(obsTrain);
    const PeaksInfo& obsInfo = trainDB.getRefInfo(obsTrainNo);

crosses[obsTrain] = map<string, Alignment>();

    for (auto& refTrain: trainDB)
    {
      if (refTrain == obsTrain)
        continue;

      match.trainName = refTrain;
      match.trainNo = static_cast<unsigned>(trainDB.lookupNumber(refTrain));
      match.numCars = trainDB.numCars(match.trainNo);
      match.numAxles = trainDB.numAxles(match.trainNo);

      if (! align.trainMightFitGeometrically(obsInfo, match))
        continue;

      const PeaksInfo& refInfo = trainDB.getRefInfo(match.trainNo);

      if (! align.alignPeaks(refInfo, obsInfo, match))
        continue;

      align.regressTrain(obsInfo.positions, refInfo.positions, true, match);

      crosses[obsTrain][refTrain] = match;
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
    ss << "True train: " << obs.first << "\n";
    for (auto &ref: obs.second)
    {
      if (ref.second.distMatch < 50.f)
        ss << ref.second.str();
    }
    ss << "\n";
  }

  return ss.str();
}

