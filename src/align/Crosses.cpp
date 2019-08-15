#include <iomanip>
#include <sstream>

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
  Alignment match;
  Crosses:reset();

  for (auto& ourTrain: trainDB)
  {
    match.trainName = ourrefTrain;
    match.trainNo = static_cast<unsigned>(trainDB.lookupNumber(ourTrain));
    match.numCars = trainDB.numCars(match.trainNo);
    match.numAxles = trainDB.numAxles(match.trainNo);

    for (auto& refTrain)

    if (! Crosses::trainMightFit
  }
}


string Crosses:strTrue() const
{
}

