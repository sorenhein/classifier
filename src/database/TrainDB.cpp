#include "TrainDB.h"

#include <iostream>
#include <iomanip>
#include <sstream>

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


TrainDB::TrainDB()
{
  TrainDB::reset();
}


TrainDB::~TrainDB()
{
}


void TrainDB::reset()
{
  fields.clear();
  fieldCounts.clear();
  entries.clear();
  offTrainMap.clear();

  TrainDB::configure();
}


void TrainDB::configure()
{
  fields =
  {
    { "OFFICIAL_NAME", CORRESPONDENCE_STRING, TRAIN_OFFICIAL_NAME },
    { "NAME", CORRESPONDENCE_STRING, TRAIN_NAME },

    { "COUNTRIES", CORRESPONDENCE_STRING_VECTOR, TRAIN_COUNTRIES },
    { "ORDER", CORRESPONDENCE_STRING_VECTOR, TRAIN_CAR_ORDER },

    { "INTRODUCTION", CORRESPONDENCE_INT, TRAIN_INTRODUCTION },
    { "RETIREMENT", CORRESPONDENCE_INT, TRAIN_RETIREMENT },

    { "SYMMETRY", CORRESPONDENCE_BOOL, TRAIN_SYMMETRY }
  };

  fieldCounts =
  {
    TRAIN_STRINGS_SIZE,
    TRAIN_STRING_VECTORS_SIZE,
    TRAIN_INT_VECTORS_SIZE,
    TRAIN_INTS_SIZE,
    TRAIN_BOOLS_SIZE
  };
}


bool TrainDB::complete(Entity& entry)
{
  UNUSED(entry);
  return true;

  // TODO
  // makeTrainAxles
  // parse order
  // method for corrections
}


bool TrainDB::readFile(const string& fname)
{
  Entity entry;
  if (! entry.readFile(fname, fields, fieldCounts))
    return false;

  if (! TrainDB::complete(entry))
    return false;

  offTrainMap[entry.getString(TRAIN_OFFICIAL_NAME)] = entries.size();

  entries.push_back(entry);
  return true;
}


unsigned TrainDB::numAxles(const unsigned trainNo) const
{
  if (trainNo >= entries.size())
    return 0;
  else
    return entries[trainNo].sizeInt(TRAIN_AXLES);
}


unsigned TrainDB::numCars(const unsigned trainNo) const
{
  if (trainNo >= entries.size())
    return 0;
  else
    return entries[trainNo].sizeString(TRAIN_CARS);
}


int TrainDB::lookupTrainNumber(const string& offName) const
{
  auto it = offTrainMap.find(offName);
  if (it == offTrainMap.end())
    return -1;
  else
    return it->second;
}


string TrainDB::lookupTrainName(const unsigned trainNo) const
{
  if (trainNo >= entries.size())
    return "Bad index";
  else
    return entries[trainNo].getString(TRAIN_OFFICIAL_NAME);
}


bool TrainDB::reversed(const unsigned trainNo) const
{
  if (trainNo >= entries.size())
    return false;
  else
    return entries[trainNo].getBool(TRAIN_REVERSED);
}

