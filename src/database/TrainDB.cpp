#include <iostream>
#include <iomanip>
#include <sstream>
#include <cassert>

#include "CarDB.h"
#include "CorrectionDB.h"
#include "TrainDB.h"

#include "../util/parse.h"

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


bool TrainDB::complete(
  const CarDB& carDB,
  Entity& entry)
{
  string carName;
  bool reverseFlag;
  unsigned count;

  auto& stringVector = entry.getStringVector(TRAIN_CAR_ORDER);
  auto& axles = entry.getIntVector(TRAIN_AXLES);
  int pos = 0;

  for (auto& str: stringVector)
  {
    if (! parseCarSpecifier(str, carName, reverseFlag, count))
    {
      cout << "Could not parse car specification: " << str << endl;
      return false;
    }

    const int carDBNo = carDB.lookupNumber(str);
    if (carDBNo == 0)
    {
      cout << "Car " << str << " not found" << endl;
      return false;
    }

    const int carNo = (reverseFlag ? carDBNo : -carDBNo);
    for (unsigned i = 0; i < count; i++)
    {
      if (! carDB.appendAxles(carNo, pos, axles))
      {
        cout << "TrainDB::complete: appendAxles failed\n";
        return false;
      }
    }
  }

  return true;
}


bool TrainDB::correct(
  const CorrectionDB& correctionDB,
  Entity& entry)
{
  // At this point the official name has not been adorned with _N / _R.
  vector<int> const * corrptr = correctionDB.getIntVector(
    entry.getString(TRAIN_OFFICIAL_NAME));

  if (! corrptr)
    return false;

  vector<int>& axles = entry.getIntVector(TRAIN_AXLES);

  if (axles.size() != corrptr->size())
  {
    cout << "Correction of " << entry.getString(TRAIN_OFFICIAL_NAME) <<
      " attempted with " << corrptr->size() << " axles" << endl;
    return false;
  }

  for (unsigned i = 0; i < axles.size(); i++)
    axles[i] += (* corrptr)[i] - (* corrptr)[0];

  return true;
}


bool TrainDB::readFile(
  const CarDB& carDB,
  const CorrectionDB& correctionDB,
  const string& fname)
{
  Entity entry;
  if (! entry.readFile(fname, fields, fieldCounts))
    return false;

  if (! TrainDB::complete(carDB, entry))
    return false;

  TrainDB::correct(correctionDB, entry);

  // The train is first stored in the normal direction, and the name
  // is suffixed with _N.

  const string officialName = entry.getString(TRAIN_OFFICIAL_NAME);

  entry.getString(TRAIN_OFFICIAL_NAME) += "_N";
  entry.setBool(TRAIN_REVERSED, false);
  offTrainMap[entry.getString(TRAIN_OFFICIAL_NAME)] = entries.size();
  entries.push_back(entry);

  // Then we store the reverse direction.

  entry.getString(TRAIN_OFFICIAL_NAME) = officialName + "_R";
  entry.setBool(TRAIN_REVERSED, true);
  entry.reverseIntVector(TRAIN_AXLES);
  offTrainMap[entry.getString(TRAIN_OFFICIAL_NAME)] = entries.size();
  entries.push_back(entry);

  return true;
}


unsigned TrainDB::numAxles(const unsigned trainNo) const
{
  assert(trainNo < entries.size());
  return entries[trainNo].sizeInt(TRAIN_AXLES);
}


unsigned TrainDB::numCars(const unsigned trainNo) const
{
  assert(trainNo < entries.size());
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
  assert(trainNo < entries.size());
  return entries[trainNo].getString(TRAIN_OFFICIAL_NAME);
}


bool TrainDB::reversed(const unsigned trainNo) const
{
  assert(trainNo < entries.size());
  return entries[trainNo].getBool(TRAIN_REVERSED);
}

