#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>

#include "Database.h"
#include "TraceDB.h"
#include "read.h"

#include "util/parse.h"
#include "util/misc.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


void resetTrain(TrainEntry& t);

bool readOrder(
  const Database& db,
  const string& text,
  vector<int>& carNumbers,
  const string& err);

bool makeTrainAxles(
  const Database& db,
  TrainEntry& t);

void readCarFile(
  Database& db,
  const string& fname);

void readCorrectionFile(
  const string& fname,
  map<string, vector<int>>& corrections);

void correctTrain(
  TrainEntry& t,
  const vector<int>& corr);

void readTrainFile(
  Database& db,
  const string& fname,
  const map<string, vector<int>>& corrections);


void resetTrain(TrainEntry& t)
{
  t.name = "";
  t.introduction = 0;
  t.countries.clear();
  t.carNumbers.clear();
  t.fourWheelFlag = true;
}



bool readOrder(
  const Database& db,
  const string& text,
  vector<int>& carNumbers,
  const string& err)
{
  // Split on comma to get groups of cars.
  const size_t c = countDelimiters(text, ",");
  vector<string> carStrings(c+1);
  carStrings.clear();
  tokenize(text, carStrings, ",");

  // Expand notation "802(7)" into 7 cars of type 802.
  carNumbers.clear();
  for (unsigned i = 0; i < carStrings.size(); i++)
  {
    string s = carStrings[i];
    int count = 1;
    if (s.back() == ')')
    {
      auto pos = s.find("(");
      if (pos == string::npos)
        return false;

      string bracketed = s.substr(pos+1);
      bracketed.pop_back();
      if (! parseInt(bracketed, count))
      {
        cout << err << endl;
        return false;
      }
      
      s = s.substr(0, pos);
    }

    // Car is "backwards".
    int reverseMultiplier = 1;
    if (s.back() == ']')
    {
      auto pos = s.find("[");
      if (pos == string::npos)
        return false;

      string bracketed = s.substr(pos+1);
      if (bracketed != "R]")
        return false;
      
      s = s.substr(0, pos);
      reverseMultiplier = -1;
    }

    // A reversed car has the negative number.
    const int carNo = reverseMultiplier * db.lookupCarNumber(s);

    for (unsigned j = 0; j < static_cast<unsigned>(count); j++)
      carNumbers.push_back(carNo);
  }
  return true;
}


void readCarFiles(
  Database& db,
  const string& dir)
{
  vector<string> textfiles;
  getFilenames(dir, textfiles);

  for (auto &f: textfiles)
    db.readCarFile(f);
}


bool makeTrainAxles(
  const Database& db,
  TrainEntry& t)
{
  int pos = 0;
  for (int carNo: t.carNumbers)
  {
    if (! db.appendAxles(carNo, pos, t.axles))
    {
      cout << "makeTrainAxles: appendAxles failed\n";
      return false;
    }
  }
  return true;
}


bool readControlFile(
  Control& c,
  const string& fname)
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
      fin.close();
      return false;
    }

    const string& field = line.substr(0, sp);
    const string& rest = line.substr(sp+1);

    if (field == "CAR_DIRECTORY")
      c.carDir = rest;
    else if (field == "TRAIN_DIRECTORY")
      c.trainDir = rest;
    else if (field == "CORRECTION_DIRECTORY")
      c.correctionDir = rest;
    else if (field == "SENSOR_FILE")
      c.sensorFile = rest;
    else if (field == "TRUTH_FILE")
      c.truthFile = rest;
    else if (field == "COUNTRY")
      c.country = rest;
    else if (field == "YEAR")
    {
      if ( ! parseInt(rest, c.year)) 
      {
        cout << err << endl;
        break;
      }
    }
    else if (field == "TRACE_DIRECTORY")
      c.traceDir = rest;
    else if (field == "SIM_COUNT")
    {
      if ( ! parseInt(rest, c.simCount)) 
      {
        cout << err << endl;
        break;
      }
    }
    else if (field == "SPEED_MIN")
    {
      if ( ! parseDouble(rest, c.speedMin, err)) break;
    }
    else if (field == "SPEED_MAX")
    {
      if ( ! parseDouble(rest, c.speedMax, err)) break;
    }
    else if (field == "SPEED_STEP")
    {
      if ( ! parseDouble(rest, c.speedStep, err)) break;
    }
    else if (field == "ACCELERATION_MIN")
    {
      if ( ! parseDouble(rest, c.accelMin, err)) break;
    }
    else if (field == "ACCELERATION_MAX")
    {
      if ( ! parseDouble(rest, c.accelMax, err)) break;
    }
    else if (field == "ACCELERATION_STEP")
    {
      if ( ! parseDouble(rest, c.accelStep, err)) break;
    }
    else if (field == "CROSSCOUNT_FILE")
      c.crossCountFile = rest;
    else if (field == "CROSSPERCENT_FILE")
      c.crossPercentFile = rest;
    else if (field == "OVERVIEW_FILE")
      c.overviewFile = rest;
    else if (field == "DETAIL_FILE")
      c.detailFile = rest;
    else
    {
      cout << err << endl;
      break;
    }
  }

  fin.close();
  return true;
}


void readCorrectionFile(
  const string& fname,
  map<string, vector<int>>& corrections)
{
  ifstream fin;
  fin.open(fname);
  string line;
  string officialName;

  getline(fin, line);
  line.erase(remove(line.begin(), line.end(), '\r'), line.end());

  if (line.empty())
  {
    fin.close();
    cout << "Error reading correction file: '" << line << "'" << endl;
    return;
  }

  const auto sp = line.find(" ");
  const string& field = line.substr(0, sp);
  const string& rest = line.substr(sp+1);

  if (field == "OFFICIAL_NAME")
    officialName = rest;
  else
  {
    fin.close();
    cout << "Error reading correction file: '" << line << "'" << endl;
    return;
  }

  int value;
  while (getline(fin, line))
  {
    line.erase(remove(line.begin(), line.end(), '\r'), line.end());
    if (line.empty())
      break;

    if (! parseInt(line, value))
    {
      cout << "Bad integer" << endl;
      break;
    }

    corrections[officialName].push_back(value);
  }

  fin.close();
}


void correctTrain(
  TrainEntry& t,
  const vector<int>& corr)
{
  if (t.axles.size() != corr.size())
  {
    cout << "Correction attempted with " << corr.size() << " vs. " <<
      t.axles.size() << " axles\n";
    return;
  }

  for (unsigned i = 0; i < corr.size(); i++)
    t.axles[i] += corr[i] - corr[0];

}


void readTrainFile(
  Database& db,
  const string& fname,
  const map<string, vector<int>>& corrections)
{

  TrainEntry t;
  resetTrain(t);

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

    if (field == "OFFICIAL_NAME")
      t.officialName = rest;
    else if (field == "NAME")
      t.name = rest;
    else if (field == "INTRODUCTION")
    {
      if ( ! parseInt(rest, t.introduction)) 
      {
        cout << err << endl;
        break;
      }
    }
    else if (field == "RETIREMENT")
    {
      if ( ! parseInt(rest, t.retirement)) 
      {
        cout << err << endl;
        break;
      }
    }
    else if (field == "COUNTRIES")
      parseCommaString(rest, t.countries);
    else if (field == "SYMMETRY")
    {
      if (! parseBool(rest, t.symmetryFlag, err)) break;
    }
    else if (field == "ORDER")
    {
      if (! readOrder(db, rest, t.carNumbers, err)) break;
    }
    else
    {
      cout << err << endl;
      break;
    }
  }
  fin.close();

  if (! makeTrainAxles(db, t))
    cout << "File " + fname + ": Could not make axles" << endl;

  auto v = corrections.find(t.officialName);
  if (v != corrections.end())
    correctTrain(t, v->second);

  db.logTrain(t);
}


void readTrainFiles(
  Database& db,
  const string& dir,
  const string& correctionDir)
{
  vector<string> correctionFiles;
  getFilenames(correctionDir, correctionFiles);

  map<string, vector<int>> corrections;
  for (auto &f: correctionFiles)
    readCorrectionFile(f, corrections);

  vector<string> textfiles;
  getFilenames(dir, textfiles);

  for (auto &f: textfiles)
  {
    readTrainFile(db, f, corrections);
    db.readTrainFile(f);
  }
}


void readSensorFile(
  Database& db,
  const string& fname)
{
  ifstream fin;
  fin.open(fname);
  string line;
  vector<string> v;

  while (getline(fin, line))
  {
    line.erase(remove(line.begin(), line.end(), '\r'), line.end());
    if (line == "" || line.front() == '#')
      continue;

    const string err = "File " + fname + ": Bad line '" + line + "'";

    const size_t c = countDelimiters(line, ",");
    if (c != 2)
    {
      cout << err << endl;
      fin.close();
      return;
    }

    v.clear();
    tokenize(line, v, ",");

    if (! db.logSensor(v[0], v[1], v[2]))
    {
      cout << err << endl;
      fin.close();
      return;
    }
  }
  fin.close();
}


bool readTraceTruth(
  const string& fname,
  const Database& db,
  TraceDB& tdb)
{
  ifstream fin;
  fin.open(fname);
  string line;
  vector<string> v;
  TraceTruth truth;

  if (! getline(fin, line))
  {
    cout << "File " << fname << ": no first line\n";
    return false;
  }

  while (getline(fin, line))
  {
    line.erase(remove(line.begin(), line.end(), '\r'), line.end());
    if (line == "" || line.front() == '#')
      continue;

    const string err = "File " + fname + ": Bad line '" + line + "'";

    const size_t c = countDelimiters(line, ",");
    if (c != 16)
    {
      cout << err << endl;
      fin.close();
      return false;
    }

    v.clear();
    tokenize(line, v, ",");

    truth.filename = v[0];
    truth.trainName = v[2];

    if (! parseInt(v[3], truth.numAxles))
    {
      cout << err << endl;
      fin.close();
      return false;
    }

    if (! parseDouble(v[4], truth.speed, err))
    {
      fin.close();
      return false;
    }

    if (! parseDouble(v[5], truth.accel, err))
    {
      fin.close();
      return false;
    }

    if (v[6] == "1")
      truth.reverseFlag = false;
    else if (v[6] == "-1")
      truth.reverseFlag = true;
    else
      return false;

    tdb.log(truth, db);
  }
  fin.close();
  return true;
}

