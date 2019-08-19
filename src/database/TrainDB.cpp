#include <iostream>
#include <iomanip>
#include <sstream>
#include <cassert>

#include "CarDB.h"
#include "CorrectionDB.h"
#include "TrainDB.h"
#include "parse.h"


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
  trainInfo.clear();
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
    // A split of the input format intro strings; gets parsed further.
    { "ORDER", CORRESPONDENCE_STRING_VECTOR, TRAIN_CAR_ORDER },

    // Year of introduction.
    { "INTRODUCTION", CORRESPONDENCE_INT, TRAIN_INTRODUCTION },
    // Year of retirement.
    { "RETIREMENT", CORRESPONDENCE_INT, TRAIN_RETIREMENT },

    // "true" or "false".
    { "SYMMETRY", CORRESPONDENCE_BOOL, TRAIN_SYMMETRY }
  };

  fieldCounts =
  {
    TRAIN_STRINGS_SIZE,
    TRAIN_STRING_VECTORS_SIZE,
    0,
    TRAIN_INT_VECTORS_SIZE,
    TRAIN_FLOAT_VECTORS_SIZE,
    TRAIN_INTS_SIZE,
    TRAIN_BOOLS_SIZE,
    0
  };
}


bool TrainDB::complete(
  const CarDB& carDB,
  Entity& entry,
  PeaksInfo& peaksInfo)
{
  string carName;
  bool reverseFlag;
  unsigned count;

  auto& stringVector = entry.getStringVector(TRAIN_CAR_ORDER);
  int pos = 0;
  int carRunning = 0;
  int numRunning = 0;
  int _numCars = 0;

  for (auto& str: stringVector)
  {
    if (! parseCarSpecifier(str, carName, reverseFlag, count))
    {
      cout << "Could not parse car specification: " << str << endl;
      return false;
    }

    const int carDBNo = carDB.lookupNumber(carName);
    if (carDBNo == 0)
    {
      cout << "Car " << str << " not found" << endl;
      return false;
    }

    const int carNo = (reverseFlag ? -carDBNo : carDBNo);
    for (unsigned i = 0; i < count; i++)
    {
      if (! carDB.appendAxles(carNo, pos, carRunning, numRunning, 
          peaksInfo))
      {
        cout << "TrainDB::complete: appendAxles failed\n";
        return false;
      }
    }
    _numCars += static_cast<int>(count);
  }

  entry[TRAIN_NUM_CARS] = _numCars;

  peaksInfo.numCars = _numCars;
  peaksInfo.numPeaks = peaksInfo.positions.size();

  return true;
}


bool TrainDB::correct(
  const CorrectionDB& correctionDB,
  const Entity& entry,
  PeaksInfo& peaksInfo)
{
  // At this point the official name has not been adorned with _N / _R.
  vector<int> const * corrptr = correctionDB.getIntVector(
    entry.getString(TRAIN_OFFICIAL_NAME));

  if (! corrptr)
    return false;

  if (peaksInfo.positions.size() != corrptr->size())
  {
    cout << "Correction of " << entry.getString(TRAIN_OFFICIAL_NAME) <<
      " attempted with " << corrptr->size() << " axles" << endl;
    return false;
  }

  for (unsigned i = 0; i < peaksInfo.positions.size(); i++)
    peaksInfo.positions[i] += ((* corrptr)[i] - (* corrptr)[0]) / 1000.f;

  return true;
}


bool TrainDB::readFile(
  const CarDB& carDB,
  const CorrectionDB& correctionDB,
  const string& fname)
{
  Entity entry;
  if (! entry.readTagFile(fname, fields, fieldCounts))
    return false;

  PeaksInfo peaksInfo;
  if (! TrainDB::complete(carDB, entry, peaksInfo))
    return false;

  TrainDB::correct(correctionDB, entry, peaksInfo);

  // The train is first stored in the normal direction, and the name
  // is suffixed with _N.

  const string officialName = entry.getString(TRAIN_OFFICIAL_NAME);

  entry.getString(TRAIN_OFFICIAL_NAME) += "_N";
  entry.setBool(TRAIN_REVERSED, false);
  offTrainMap[entry.getString(TRAIN_OFFICIAL_NAME)] = entries.size();
  entries.push_back(entry);

  trainInfo.push_back(peaksInfo);

  // Then we store the reverse direction.

  if (! entry.getBool(TRAIN_SYMMETRY))
  {
    entry.getString(TRAIN_OFFICIAL_NAME) = officialName + "_R";
    entry.setBool(TRAIN_REVERSED, true);
    offTrainMap[entry.getString(TRAIN_OFFICIAL_NAME)] = entries.size();
    entries.push_back(entry);

    peaksInfo.reversePositions();
    peaksInfo.reverseCarNumbers();
    peaksInfo.reversePeakNumbersInCar();
    trainInfo.push_back(peaksInfo);
  }

  return true;
}


void TrainDB::logEquivalences(const vector<string>& equiv)
{
  for (unsigned i = 0; i < equiv.size(); i++)
  {
    for (unsigned j = i+1; j < equiv.size(); j++)
    {
      equivalences[equiv[i]].push_back(equiv[j]);
      equivalences[equiv[j]].push_back(equiv[i]);
    }
  }
}


unsigned TrainDB::size() const
{
   return trainInfo.size();
}


unsigned TrainDB::numAxles(const unsigned trainNo) const
{
  assert(trainNo < trainInfo.size());
  return trainInfo[trainNo].numPeaks;
}


unsigned TrainDB::numCars(const unsigned trainNo) const
{
  assert(trainNo < trainInfo.size());
  return trainInfo[trainNo].numCars;
}


int TrainDB::lookupNumber(const string& offName) const
{
  auto it = offTrainMap.find(offName);
  if (it == offTrainMap.end())
    return -1;
  else
    return it->second;
}


string TrainDB::lookupName(const unsigned trainNo) const
{
  assert(trainNo < entries.size());
  return entries[trainNo].getString(TRAIN_OFFICIAL_NAME);
}


bool TrainDB::reversed(const unsigned trainNo) const
{
  assert(trainNo < entries.size());
  return entries[trainNo].getBool(TRAIN_REVERSED);
}


bool TrainDB::equivalent(
  const string& train1,
  const string& train2) const
{
  if (train1 == train2)
    return true;

  auto it = equivalences.find(train1);
  if (it == equivalences.end())
    return false;

  for (const string& s: it->second)
  {
    if (train2 == s)
      return true;
  }
  return false;
}


bool TrainDB::isInCountry(
  const unsigned trainNo,
  const string& country) const
{
  assert(trainNo < entries.size());
  const vector<string>& sv = 
    entries[trainNo].getStringVector(TRAIN_COUNTRIES);

  for (const auto& c: sv)
  {
    if (c == country)
      return true;
  }
  return false;
}


const PeaksInfo& TrainDB::getRefInfo(const unsigned trainNo) const
{
  assert(trainNo < trainInfo.size());
  return trainInfo[trainNo];
}


bool TrainDB::selectByAxles(
  const list<string>& countries,
  const unsigned minAxles,
  const unsigned maxAxles)
{
  // Used in preparation for looping using begin() and end().

  selected.clear();
  if (countries.empty())
    return false;

  const bool allFlag = 
    (countries.size() == 1 && countries.front() == "ALL");

  map<string, int> countryMap;
  if (! allFlag)
  {
    // Make a map of country strings
    for (auto& c: countries)
      countryMap[c] = 1;
  }

  for (unsigned i = 0; i < entries.size(); i++)
  {
    const string offName = entries[i].getString(TRAIN_OFFICIAL_NAME);
    if (! allFlag && countryMap.find(offName) == countryMap.end())
      continue;

    const unsigned l = trainInfo[i].numPeaks;
    if (l < minAxles || l > maxAxles)
      continue;

    selected.push_back(offName);
  }

  return (! selected.empty());
}


bool TrainDB::selectByCars(
  const list<string>& countries,
  const unsigned minCars,
  const unsigned maxCars)
{
  // Used in preparation for looping using begin() and end().

  selected.clear();
  if (countries.empty())
    return false;

  const bool allFlag = 
    (countries.size() == 1 && countries.front() == "ALL");

  map<string, int> countryMap;
  if (! allFlag)
  {
    // Make a map of country strings
    for (auto& c: countries)
      countryMap[c] = 1;
  }

  for (unsigned i = 0; i < entries.size(); i++)
  {
    const string offName = entries[i].getString(TRAIN_OFFICIAL_NAME);
    if (! allFlag && countryMap.find(offName) == countryMap.end())
      continue;

    const unsigned l = trainInfo[i].numCars;
    if (l < minCars || l > maxCars)
      continue;

    selected.push_back(offName);
  }

  return (! selected.empty());
}

