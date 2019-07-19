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
    TRAIN_VECTORS_SIZE,
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


int TrainDB::lookupTrainNumber(const string& offName) const
{
  auto it = offTrainMap.find(offName);
  if (it == offTrainMap.end())
    return 0; // Invalid number
  else
    return it->second;
}

