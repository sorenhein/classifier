#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>

#include "Database.h"
#include "TraceDB.h"
#include "read.h"
#include "misc.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


void resetCar(CarEntry& c);

void resetTrain(TrainEntry& t);

bool readOrder(
  const Database& db,
  const string& text,
  vector<int>& carNumbers,
  const string& err);

bool readLevel(
  const string& text,
  TrainLevel& value,
  const string& err);

bool fillInEquation(
  int &lhs,
  vector<int *>& rhs,
  const unsigned len);

bool fillInDistances(CarEntry& c);

void printDistances(const CarEntry& c);

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


void resetCar(CarEntry& c)
{
  c.officialName = "";
  c.name = "";
  c.usage.clear();
  c.countries.clear();
  c.introduction = 0;
  c.powerFlag = false;
  c.capacity = 0;
  c.level = TRAIN_NONE;
  c.restaurantFlag = false;
  c.weight = 0;
  c.wheelLoad = 0;
  c.speed = 1; // Something non-zero in case we divide by it
  c.configurationUIC = "";
  c.length = 4; // Something non-zero in case we divide by it
  c.symmetryFlag = false;
  c.fourWheelFlag = true;

  c.distWheels = -1;
  c.distWheels1 = -1;
  c.distWheels2 = -1;
  c.distMiddles = -1;
  c.distPair = -1;
  c.distFrontToWheel = -1;
  c.distWheelToBack = -1;
  c.distFrontToMid1 = -1;
  c.distBackToMid2 = -1;
}


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
      if (! parseInt(bracketed, count, err))
        return false;
      
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


bool readLevel(
  const string& text,
  TrainLevel& level,
  const string& err)
{
  if (text == "none")
    level = TRAIN_NONE;
  else if (text == "first")
    level = TRAIN_FIRST;
  else if (text == "second")
    level = TRAIN_SECOND;
  else
  {
    cout << err << endl;
    return false;
  }

  return true;
}


bool fillInEquation(
  int &lhs,
  vector<int *>& rhs,
  const unsigned len)
{
  // Equation: lhs = sum(rhs).
  // If all are given, check consistency.
  // If one is missing, calculate it.
  // Otherwise, return error.
  
  if (len <= 1)
    return false;

  unsigned countLHS = 0;
  if (lhs >= 0)
  countLHS++;

  unsigned countRHS = 0;
  int sum = 0;
  int miss = -1;
  for (unsigned i = 0; i < len; i++)
  {
    const int r = * rhs[i];
    if (r >= 0)
      countRHS++;
    else
      miss = static_cast<int>(i);

    sum += r;
  }


  if (countRHS == len)
  {
    if (countLHS == 0)
      lhs = sum;
    else if (lhs != sum)
      return false;
  }
  else if (countRHS == len-1)
  {
    if (countLHS == 0)
      return false;
    else
    {
      const unsigned m = static_cast<unsigned>(miss);
      * rhs[m] = lhs - (sum - * rhs[m]);
    }
  }
  else
    return false;

  return true;
}


bool fillInDistances(CarEntry& c)
{
  // Calculate missing distances.  Check that not over-specified.
  // Less than some small number means that the value was not set.
  // There are probably ways that the data can be consistent and still 
  // be flagged below.  This is just a quick hack (Gaussian elimination!).
  //
  // Equations:
  //
  // 1. distMiddles = distPair + (1/2) * (distWheels1 + distWheels2), i.e.
  // 2*distMiddles = 2*distPair + distWheels1 + distWheels2
  //
  // 2. length = distFrontToWheel + distWheels1 + distPair + 
  //    distWheels2 + distWheelToBack
  //
  // distFrontToMid1 = distFrontToWheel + (1/2) * distWheels1, i.e.
  // 3. 2*distFrontToMid1 = 2*distFrontToWheel + distWheels1.
  //
  // distBackToMid2 = distWheelToBack + (1/2) * distWheels2, i.e.
  // 4. 2*distBackToMid2 = 2*distWheelToBack + distWheels2.

  vector<int *> rhs(5);

  bool done = false;
  int lhs;
  int rhsv;
  for (unsigned iter = 0; iter < 3 && ! done; iter++)
  {
    done = true;

    // Equation 1
    lhs = 2 * c.distMiddles;
    rhsv = 2 * c.distPair;
    rhs[0] = &rhsv;
    rhs[1] = &c.distWheels1;
    rhs[2] = &c.distWheels2;
    if (! fillInEquation(lhs, rhs, 3))
      done = false;
    else
    {
      c.distMiddles = lhs/2;
      c.distPair = rhsv/2;
    }

    // Equation 2
    rhs[0] = &c.distFrontToWheel;
    rhs[1] = &c.distWheels1;
    rhs[2] = &c.distPair;
    rhs[3] = &c.distWheels2;
    rhs[4] = &c.distWheelToBack;
    if (! fillInEquation(c.length, rhs, 5))
      done = false;

    // Equation 3
    lhs = 2 * c.distFrontToMid1;
    rhsv = 2 * c.distFrontToWheel;
    rhs[0] = &rhsv;
    rhs[1] = &c.distWheels1;
    if (! fillInEquation(lhs, rhs, 2))
      done = false;
    else
    {
      c.distFrontToMid1 = lhs/2;
      c.distFrontToWheel = rhsv/2;
    }

    // Equation 4
    lhs = 2 * c.distBackToMid2;
    rhsv = 2 * c.distWheelToBack;
    rhs[0] = &rhsv;
    rhs[1] = &c.distWheels2;
    if (! fillInEquation(lhs, rhs, 2))
      done = false;
    else
    {
      c.distBackToMid2 = lhs/2;
      c.distWheelToBack = rhsv/2;
    }
  }

  return done;
}


void printDistances(const CarEntry& c)
{
  cout << setw(18) << left << "distWheels" << c.distWheels << "\n";
  cout << setw(18) << left << "distWheels1" << c.distWheels1 << "\n";
  cout << setw(18) << left << "distWheels2" << c.distWheels2 << "\n";
  cout << setw(18) << left << "distMiddles" << c.distMiddles << "\n";
  cout << setw(18) << left << "distPair" << c.distPair << "\n";
  cout << setw(18) << left << "distFrontToWheel" << c.distFrontToWheel << "\n";
  cout << setw(18) << left << "distWheelToBack" << c.distWheelToBack << "\n";
  cout << setw(18) << left << "distFrontToMid1" << c.distFrontToMid1 << "\n";
  cout << setw(18) << left << "distBackToMid2" << c.distBackToMid2 << "\n";
  cout << endl;
}


void readCarFile(
  Database& db,
  const string& fname)
{
  CarEntry c;
  resetCar(c);

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
      c.officialName = rest;
    else if (field == "NAME")
      c.name = rest;
    else if (field == "USAGE")
      parseCommaString(rest, c.usage);
    else if (field == "COUNTRIES")
      parseCommaString(rest, c.countries);
    else if (field == "INTRODUCTION")
    {
      if (! parseInt(rest, c.introduction, err)) break;
    }
    else if (field == "POWER")
    {
      if (! parseBool(rest, c.powerFlag, err)) break;
    }
    else if (field == "CAPACITY")
    {
      if (! parseInt(rest, c.capacity, err)) break;
    }
    else if (field == "CLASS")
    {
      if (! readLevel(rest, c.level, err)) break;
    }
    else if (field == "RESTAURANT")
    {
      if (! parseBool(rest, c.restaurantFlag, err)) break;
    }
    else if (field == "WEIGHT")
    {
      if (! parseInt(rest, c.weight, err)) break;
    }
    else if (field == "WHEEL_LOAD")
    {
      if (! parseInt(rest, c.wheelLoad, err)) break;
    }
    else if (field == "SPEED")
    {
      if (! parseInt(rest, c.speed, err)) break;
    }
    else if (field == "CONFIGURATION")
      c.configurationUIC = rest;
    else if (field == "LENGTH")
    {
      if (! parseInt(rest, c.length, err)) break;
    }
    else if (field == "SYMMETRY")
    {
      if (! parseBool(rest, c.symmetryFlag, err)) break;
    }
    else if (field == "LENGTH")
    {
      if (! parseInt(rest, c.length, err)) break;
    }
    else if (field == "DIST_WHEELS")
    {
      if (! parseInt(rest, c.distWheels, err)) break;
      c.distWheels1 = c.distWheels;
      c.distWheels2 = c.distWheels;
    }
    else if (field == "DIST_WHEELS1")
    {
      if (! parseInt(rest, c.distWheels1, err)) break;
    }
    else if (field == "DIST_WHEELS2")
    {
      if (! parseInt(rest, c.distWheels2, err)) break;
    }
    else if (field == "DIST_MIDDLES")
    {
      if (! parseInt(rest, c.distMiddles, err)) break;
    }
    else if (field == "DIST_PAIR")
    {
      if (! parseInt(rest, c.distPair, err)) break;
    }
    else if (field == "DIST_FRONT_TO_WHEEL")
    {
      if (! parseInt(rest, c.distFrontToWheel, err)) break;
    }
    else if (field == "DIST_WHEEL_TO_BACK")
    {
      if (! parseInt(rest, c.distWheelToBack, err)) break;
    }
    else if (field == "DIST_FRONT_TO_MID1")
    {
      if (! parseInt(rest, c.distFrontToMid1, err)) break;
    }
    else if (field == "DIST_BACK_TO_MID2")
    {
      if (! parseInt(rest, c.distBackToMid2, err)) break;
    }
    else if (field == "COMMENT")
    {
      // Ignore
    }
    else
    {
      cout << err << endl;
      break;
    }
  }
  fin.close();

  if (! fillInDistances(c))
  {
    cout << "File " << fname << ": Could not fill in distances" << endl;
    return;
  }

  if (c.distWheels1 == 0 || c.distWheels2 == 0)
    c.fourWheelFlag = false;

  db.logCar(c);
}


void readCarFiles(
  Database& db,
  const string& dir)
{
  vector<string> textfiles;
  getFilenames(dir, textfiles);

  for (auto &f: textfiles)
    readCarFile(db, f);
}


bool makeTrainAxles(
  const Database& db,
  TrainEntry& t)
{
  int pos = 0;
  for (unsigned i = 0; i < t.carNumbers.size(); i++)
  {
    const int carNo = t.carNumbers[i];
    if (carNo == 0)
    {
      cout << "makeTrainAxles: Bad car number" << endl;
      return false;
    }
    else if (carNo > 0)
    {
      const CarEntry * cPtr = db.lookupCar(t.carNumbers[i]);
      if (cPtr == nullptr)
        return false;
      if (! cPtr -> fourWheelFlag)
        t.fourWheelFlag = false;

      if (i > 0)
        pos += cPtr->distFrontToWheel;

      if (i == 0 || cPtr->distFrontToWheel > 0)
      t.axles.push_back(pos); // First pair, first wheel

      if (cPtr->distWheels1 > 0)
      {
        pos += cPtr->distWheels1;
        t.axles.push_back(pos); // First pair, second wheel
      }

      pos += cPtr->distPair;
      t.axles.push_back(pos); // Second pair, first wheel

      if (cPtr->distWheels2 > 0)
      {
        pos += cPtr->distWheels2;
        t.axles.push_back(pos); // Second pair, second wheel
      }

      pos += cPtr->distWheelToBack;
    }
    else
    {
      // Car is reversed.
      const CarEntry * cPtr = db.lookupCar(-t.carNumbers[i]);
      if (! cPtr -> fourWheelFlag)
        t.fourWheelFlag = false;

      if (i > 0)
        pos += cPtr->distWheelToBack;

      if (i == 0 || cPtr->distWheelToBack > 0)
        t.axles.push_back(pos); // Second pair, second wheel

      if (cPtr->distWheels2 > 0)
      {
        pos += cPtr->distWheels2;
        t.axles.push_back(pos); // Second pair, first wheel
      }

      pos += cPtr->distPair;
      t.axles.push_back(pos); // First pair, second wheel

      if (cPtr->distWheels1 > 0)
      {
        pos += cPtr->distWheels1;
        t.axles.push_back(pos); // First pair, first wheel
      }

      pos += cPtr->distFrontToWheel;
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
      if ( ! parseInt(rest, c.year, err)) break;
    }
    else if (field == "TRACE_DIRECTORY")
      c.traceDir = rest;
    else if (field == "SIM_COUNT")
    {
      if ( ! parseInt(rest, c.simCount, err)) break;
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

    if (! parseInt(line, value, "Bad integer"))
      break;

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
      if ( ! parseInt(rest, t.introduction, err)) break;
    }
    else if (field == "RETIREMENT")
    {
      if ( ! parseInt(rest, t.retirement, err)) break;
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
    readTrainFile(db, f, corrections);
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

    if (! parseInt(v[3], truth.numAxles, err))
    {
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


#define SAMPLE_RATE 2000.

bool readInputFile(
  const string& fname,
  vector<InputEntry>& actualList)
{
  ifstream fin;
  fin.open(fname);
  string line;
  InputEntry * entryp = nullptr;
  vector<string> v;
  int prevNo = -1;
  int offset = 0, number, i;
  PeakTime peak;

  while (getline(fin, line))
  {
    line.erase(remove(line.begin(), line.end(), '\r'), line.end());
    if (line == "" || line.front() == '#')
      continue;

    const string err = "File " + fname + ": Bad line '" + line + "'";
    const size_t c = countDelimiters(line, ",");
    if (c != 5)
    {
      cout << err << endl;
      fin.close();
      return false;
    }

    v.clear();
    tokenize(line, v, ",");

    if (! parseInt(v[0], number, err))
    {
      fin.close();
      return false;;
    }

    if (number != prevNo)
    {
      prevNo = number;
      actualList.push_back(InputEntry());
      entryp = &actualList.back();

      entryp->number = number;
      entryp->tag = v[1];
      entryp->date = v[2];
      entryp->time = v[3];

      if (! parseInt(v[4], offset, err))
      {
        fin.close();
        return false;;
      }
    }

    if (! parseInt(v[4], i, err))
    {
      fin.close();
      return false;;
    }
    peak.time = (i - offset) / SAMPLE_RATE;

    if (! parseDouble(v[5], peak.value, err))
    {
      fin.close();
      return false;;
    }

    entryp->actual.push_back(peak);
  }

  fin.close();
  return true;
}

