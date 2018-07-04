#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "Database.h"
#include "read.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


void resetCar(CarEntry& c);

void resetTrain(TrainEntry& t);

void readCountries(
  const string& text,
  vector<string>& countries);

bool readOrder(
  const Database& db,
  const string& text,
  vector<int>& carNumbers,
  const string& err);

void readUsage(
  const string& text,
  vector<string>& value,
  const string& err);

bool readBool(
  const string& text,
  bool& value,
  const string& err);

bool readLevel(
  const string& text,
  TrainLevel& value,
  const string& err);

void readCarFile(const string& fname);

void readTrainFile(const string& fname);


void getFilenames(
  const string& dir,
  vector<string>& textfiles)
{
  // TODO
  UNUSED(dir);
  UNUSED(textfiles);
}


void resetCar(CarEntry& c)
{
  c.officialName = "";
  c.name = "";
  c.usage.clear();
  c.countries.clear();
  c.introduction = 0;
  c.powerFlag = false;
  c.passengers = 0;
  c.capacity = 0;
  c.level = TRAIN_NONE;
  c.restaurantFlag = false;
  c.weight = 0;
  c.wheelLoad = 0;
  c.speed = 1; // Something non-zero in case we divide by it
  c.configurationUIC = "";
  c.length = 4; // Something non-zero in case we divide by it
  c.symmetryFlag = false;
  c.distWheels = 1;
  c.distMiddles = 2;
  c.distPair = 1;
  c.distFrontToWheel = 1;
  c.distWheelToBack = 1;
}


void resetTrain(TrainEntry& t)
{
  t.name = "";
  t.introduction = 0;
  t.countries.clear();
  t.carNumbers.clear();
}



void readCountries(
  const string& text,
  vector<string>& countries)
{
  // TODO: Split on comma
  UNUSED(text);
  UNUSED(countries);
}


bool readOrder(
  const Database& db,
  const string& text,
  vector<int>& carNumbers,
  const string& err)
{
  // TODO: Split on comma, then (n), fill in vector
  // Look up dbCarno in db
  UNUSED(db);
  UNUSED(text);
  UNUSED(carNumbers);
  UNUSED(err);
  return true;
}


void readUsage(
  const string& text,
  vector<string>& value,
  const string& err)
{
  UNUSED(text);
  UNUSED(value);
  UNUSED(err);
}


bool readInt(
  const string& text,
  int& value,
  const string& err)
{
  // TODO
  UNUSED(text);
  UNUSED(value);
  UNUSED(err);
  return true;
}


bool readFloat(
  const string& text,
  float& value,
  const string& err)
{
  // TODO
  UNUSED(text);
  UNUSED(value);
  UNUSED(err);
  return true;
}


bool readBool(
  const string& text,
  bool& value,
  const string& err)
{
  // TODO
  UNUSED(text);
  UNUSED(value);
  UNUSED(err);
  return true;
}


bool readLevel(
  const string& text,
  TrainLevel& level,
  const string& err)
{
  // TODO
  UNUSED(text);
  UNUSED(level);
  UNUSED(err);
  return true;
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
    if (line == "" || line.front() == '#')
      continue;
    
    const string err = "File " + fname + ": Bad line '" + line + "'";

    unsigned sp = line.find(" ");
    if (sp == string::npos || sp == 0 || sp == line.size()-1)
    {
      cout << err << endl;
      break;
    }

    const string& field = line.substr(0, sp-1);
    const string& rest = line.substr(sp+1);

    if (field == "OFFICIAL_NAME")
      c.officialName = rest;
    else if (field == "NAME")
      c.name = rest;
    else if (field == "USAGE")
      readUsage(rest, c.usage, err);
    else if (field == "COUNTRIES")
      readCountries(rest, c.countries);
    else if (field == "INTRODUCTION" && 
        ! readInt(rest, c.introduction, err))
      break;
    else if (field == " POWER" && ! readBool(rest, c.powerFlag, err))
      break;
    else if (field == "PASSENGERS" && ! readInt(rest, c.passengers, err))
      break;
    else if (field == "CAPACITY" && ! readInt(rest, c.capacity, err))
      break;
    else if (field == "LEVEL" && ! readLevel(rest, c.level, err))
      break;
    else if (field == " RESTAURANT" && 
        ! readBool(rest, c.restaurantFlag, err))
      break;
    else if (field == "WEIGHT" && ! readInt(rest, c.weight, err))
      break;
    else if (field == "WHEEL_LOAD" && 
        ! readInt(rest, c.wheelLoad, err))
      break;
    else if (field == "SPEED" && ! readInt(rest, c.speed, err))
      break;
    else if (field == "CONFIGURATION")
      c.configurationUIC = rest;
    else if (field == "LENGTH" && ! readInt(rest, c.length, err))
      break;
    else if (field == " SYMMETRY" && ! readBool(rest, c.symmetryFlag, err))
      break;
    else if (field == "LENGTH" && ! readInt(rest, c.length, err))
      break;
    else if (field == "DIST_WHEELS" && ! readInt(rest, c.distWheels, err))
      break;
    else if (field == "DIST_MIDDLES" && 
        ! readInt(rest, c.distMiddles, err))
      break;
    else if (field == "DIST_PAIR" && ! readInt(rest, c.distPair, err))
      break;
    else if (field == "DIST_FRONT_TO_WHEEL" && 
        ! readInt(rest, c.distFrontToWheel, err))
      break;
    else if (field == "DIST_WHEEL_TO_BACK" && 
        ! readInt(rest, c.distWheelToBack, err))
      break;
    else
    {
      cout << err << endl;
      break;
    }
  }
  fin.close();

  // TODO: Calculate missing distances
  // Check that not over-specified

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


void readTrainFile(
  Database& db,
  const string& fname)
{
  TrainEntry t;
  resetTrain(t);

  ifstream fin;
  fin.open(fname);
  string line;
  while (getline(fin, line))
  {
    if (line == "" || line.front() == '#')
      continue;

    const string err = "File " + fname + ": Bad line '" + line + "'";
    
    unsigned sp = line.find(" ");
    if (sp == string::npos || sp == 0 || sp == line.size()-1)
    {
      cout << err << endl;
      break;
    }

    const string& field = line.substr(0, sp-1);
    const string& rest = line.substr(sp+1);

    if (field == "NAME")
      t.name = rest;
    else if (field == "INTRODUCTION" && 
        ! readInt(rest, t.introduction, err))
      break;
    else if (field == "COUNTRIES")
      readCountries(rest, t.countries);
    else if (field == "ORDER" && ! readOrder(db, rest, t.carNumbers, err))
      break;
    else
    {
      cout << err << endl;
      break;
    }
  }
  fin.close();

  db.logTrain(t);
}


void readTrainFiles(
  Database& db,
  const string& dir)
{
  vector<string> textfiles;
  getFilenames(dir, textfiles);

  for (auto &f: textfiles)
    readTrainFile(db, f);
}

