#include "Database.h"


Database::Database()
{
  carEntries.clear();
  trainEntries.clear();
  offCarMap.clear();
  offTrainMap.clear();

  // Put a dummy car at position 0 which is not used.
  // We're going to put reversed cars by the negative of the
  // actual number, and this only works if we exclude 0.
  CarEntry car;
  car.officialName = "Dummy";
  carEntries.push_back(car);

  carEntriesNew.reset();
}


Database::~Database()
{
}


/*
void Database::resetCar(Entity& carEntry) const
{
  carEntry.strings.clear();
  carEntry.strings.resize(CAR_STRINGS_SIZE);

  carEntry.stringVectors.clear();
  carEntry.stringVectors.resize(CAR_STRING_VECTORS_SIZE);

  carEntry.ints.clear();
  carEntry.ints.resize(CAR_INTS_SIZE, -1);

  carEntry.bools.clear();
  carEntry.bools.resize(CAR_BOOLS_SIZE, false);

}
*/


/*
bool Database::readFile(
  const string& fname,
  const list<CorrespondenceEntry>& fields,
  DatabaseEntry& entries)
{
  ifstream fin;
  fin.open(fname);
  string line;
  while (getline(fin, line))
  {
    line.erase(remove(line.begin(), line.end(), '\r'), line.end());
    if (line == "" || line.front() == '#')
      continue;

    const string err = "File " + fname + ": Bad line '" + line + "'";

    const auto sp = line.find(" ");
    if (sp == string::npos || sp == 0 || sp == line.size()-1)
    {
      cout << err << endl;
      break;
    }

    const string& field = line.substr(0, sp);
    const string& rest = line.substr(sp+1);

    bool seenFlag = false;
    bool errFlag = false;
    for (const auto& fieldEntry: fields)
    {
      if (fieldEntry.tag == field)
      {
        if (fieldEntry.corrType == CORRESPONDENCE_STRING)
        {
          entries.strings[fieldEntry.no] = rest;
        }
        else if (fieldEntry.corrType == CORRESPONDENCE_STRING_VECTOR)
        {
          parseCommaString(rest, entries.stringVectors[fieldEntry.no]);
        }
        else if (fieldEntry.corrType == CORRESPONDENCE_INT)
        {
          if (! parseInt(rest, entries.ints[fieldEntry.no], err))
          {
            errFlag = true;
            break;
          }
        }
        else if (fieldEntry.corrType == CORRESPONDENCE_BOOL)
        {
          if (! parseBool(rest, entries.bools[fieldEntry.no], err))
          {
            errFlag = true;
            break;
          }
        }
        else
        {
          errFlag = true;
          cout << "File " << fname << ": Bad correspondence type " <<
            fieldEntry.corrType << endl;
          break;
        }

        seenFlag = true;
        break;
      }
    }

    if (! seenFlag && ! errFlag)
    {
      cout << err << endl;
      fin.close();
      return false;
      break;
    }
  }
  fin.close();
  return true;
}
*/


/*
void Database::finishCar(DatabaseEntry& carEntry) const
{
  if (carEntry.ints[CAR_DIST_WHEELS] > 0)
  {
    if (carEntry.ints[CAR_DIST_WHEELS1] == -1)
      carEntry.ints[CAR_DIST_WHEELS1] = carEntry.ints[CAR_DIST_WHEELS];
    if (carEntry.ints[CAR_DIST_WHEELS2] == -1)
      carEntry.ints[CAR_DIST_WHEELS2] = carEntry.ints[CAR_DIST_WHEELS];
  }


  carEntry.bools[CAR_FOURWHEEL_FLAG] = 
    (carEntry.ints[CAR_DIST_WHEELS1] > 0 &&
     carEntry.ints[CAR_DIST_WHEELS2] > 0);

}
*/


void Database::readCarFile(const string& fname)
{
  carEntriesNew.readFile(fname);
}


void Database::logCar(const CarEntry& car)
{
  offCarMap[car.officialName] = carEntries.size();
  carEntries.push_back(car);
}


void Database::logTrain(const TrainEntry& train)
{
  const string officialName = train.officialName;

  // Normal direction.
  trainEntries.push_back(train);
  TrainEntry& t = trainEntries.back();
  t.officialName = officialName + "_N";
  t.reverseFlag = false;
  offTrainMap[t.officialName] = trainEntries.size()-1;

  if (! train.symmetryFlag)
  {
    // Reversed as well.
    TrainEntry tr = train;

    const int aLast = tr.axles.back();
    const unsigned l = tr.axles.size();
    const vector<int> axles = tr.axles;
    for (unsigned i = 0; i < l; i++)
      tr.axles[i] = aLast - axles[l-i-1];

    tr.officialName = officialName + "_R";
    tr.reverseFlag = true;
    trainEntries.push_back(tr);
    offTrainMap[tr.officialName] = trainEntries.size()-1;
  }
}


bool Database::logSensor(
  const string& name,
  const string& country,
  const string& stype)
{
  auto it = sensors.find(name);
  if (it != sensors.end())
    return false;

  DatabaseSensor& sensor = sensors[name];
  sensor.country = country;
  sensor.type = stype;
  return true;
}


bool Database::select(
  const list<string>& countries,
  const unsigned minAxles,
  const unsigned maxAxles)
{
  // Used in preparation for looping using begin() and end().

  selectedTrains.clear();
  if (countries.empty())
    return false;

  const bool allFlag = (countries.size() == 1 && countries.front() == "ALL");
  map<string, int> countryMap;
  if (! allFlag)
  {
    // Make a map of country strings
    for (auto& c: countries)
      countryMap[c] = 1;
  }

  for (auto& train: trainEntries)
  {
    if (! allFlag && 
        countryMap.find(train.officialName) == countryMap.end())
      continue;

    const unsigned l = train.axles.size();
    if (l < minAxles || l > maxAxles)
      continue;

    selectedTrains.push_back(train.officialName);
  }

  return (! selectedTrains.empty());
}


unsigned Database::axleCount(const unsigned trainNo) const
{
  if (trainNo >= trainEntries.size())
    return 0;

  return trainEntries[trainNo].axles.size();
}


bool Database::getPerfectPeaks(
  const string& trainName,
  vector<PeakPos>& peaks) const // In m
{
  const int trainNo = Database::lookupTrainNumber(trainName);
  if (trainNo == -1)
    return false;
  else
    return Database::getPerfectPeaks(static_cast<unsigned>(trainNo), peaks);
}


bool Database::getPerfectPeaks(
  const unsigned trainNo,
  vector<PeakPos>& peaks) const // In m
{
  if (trainNo >= trainEntries.size())
    return false;

  PeakPos peak;
  peak.value = 1.f;
  peaks.clear();
  
  for (auto it: trainEntries[trainNo].axles)
  {
    peak.pos = it / 1000.; // Convert from mm to m
    peaks.push_back(peak);
  }

  return true;
}


const CarEntry * Database::lookupCar(const int carNo) const
{
  if (carNo <= 0 || carNo >= static_cast<int>(carEntries.size()))
    return nullptr;
  else
    return &carEntries[static_cast<unsigned>(carNo)];
}


int Database::lookupCarNumber(const string& offName) const
{
  auto it = offCarMap.find(offName);
  if (it == offCarMap.end())
    return 0; // Invalid number
  else
    return it->second;
}


int Database::lookupTrainNumber(const string& offName) const
{
  auto it = offTrainMap.find(offName);
  if (it == offTrainMap.end())
    return -1;
  else
    return static_cast<int>(it->second);
}


string Database::lookupTrainName(const unsigned trainNo) const
{
  if (trainNo >= trainEntries.size())
    return "Bad index";
  else
    return trainEntries[trainNo].officialName;
}


string Database::lookupSensorCountry(const string& sensor) const
{
  auto it = sensors.find(sensor);
  if (it == sensors.end())
    return "Bad sensor name";
  else
    return it->second.country;
}


bool Database::trainIsInCountry(
  const unsigned trainNo,
  const string& country) const
{
  if (trainNo >= trainEntries.size())
    return false;

  for (auto& c: trainEntries[trainNo].countries)
  {
    if (country == c)
      return true;
  }
  return false;
}


bool Database::trainIsReversed(const unsigned trainNo) const
{
  if (trainNo >= trainEntries.size())
    return false;

  return trainEntries[trainNo].reverseFlag;
}

