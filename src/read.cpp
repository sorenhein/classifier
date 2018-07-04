#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#pragma warning(push)
#pragma warning(disable: 4365 4571 4625 4626 4774 5026 5027)
#if defined(__CYGWIN__)
  #include "dirent.h"
#elif defined(_WIN32)
  #include "dirent.h"
  #include <windows.h>
  #include "Shlwapi.h"
#else
  #include <dirent.h>
#endif
#pragma warning(pop)


#include "Database.h"
#include "read.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


void tokenize(
  const string& text,
  vector<string>& tokens,
  const string& delimiters);

unsigned countDelimiters(
  const string& text,
  const string& delimiters);

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
  vector<string>& value);

bool readBool(
  const string& text,
  bool& value,
  const string& err);

bool readLevel(
  const string& text,
  TrainLevel& value,
  const string& err);

bool fillInDistances(CarEntry& c);

void readCarFile(const string& fname);

void readTrainFile(const string& fname);


// tokenize splits a string into tokens separated by delimiter.
// http://stackoverflow.com/questions/236129/split-a-string-in-c

void tokenize(
  const string& text,
  vector<string>& tokens,
  const string& delimiters)
{
  string::size_type pos, lastPos = 0;

  while (true)
  {
    pos = text.find_first_of(delimiters, lastPos);
    if (pos == std::string::npos)
    {
      pos = text.length();
      tokens.push_back(string(text.data()+lastPos,
        static_cast<string::size_type>(pos - lastPos)));
      break;
    }
    else
    {
      tokens.push_back(string(text.data()+lastPos,
        static_cast<string::size_type>(pos - lastPos)));
    }
    lastPos = pos + 1;
  }
}


unsigned countDelimiters(
  const string& text,
  const string& delimiters)
{
  int c = 0;
  for (unsigned i = 0; i < delimiters.length(); i++)
    c += static_cast<int>
      (count(text.begin(), text.end(), delimiters.at(i)));
  return static_cast<unsigned>(c);
}


void getFilenames(
  const string& dirName,
  vector<string>& textfiles)
{
  DIR *dir;
  dirent *ent;

  if ((dir = opendir(dirName.c_str())) == nullptr)
  {
    cout << "Bad directory " << dirName << endl;
    return;
  }

  while ((ent = readdir(dir)) != nullptr)
  {
    if (ent->d_type == DT_REG)
      textfiles.push_back(dirName + "/" + string(ent->d_name));
  }

  closedir(dir);
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
  const size_t c = countDelimiters(text, ",");
  countries.resize(c+1);
  countries.clear();
  tokenize(text, countries, ",");
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

      const string bracketed = s.substr(pos+1, s.size()-pos+2);
      if (! readInt(bracketed, count, err))
        return false;
      
      s = s.substr(0, pos);
    }

    const int carNo = db.lookupCarNumber(s);
    for (unsigned j = 0; j < static_cast<unsigned>(count); j++)
      carNumbers.push_back(carNo);
  }
  return true;
}


void readUsage(
  const string& text,
  vector<string>& value)
{
  const size_t c = countDelimiters(text, ",");
  value.resize(c+1);
  value.clear();
  tokenize(text, value, ",");
}


bool readInt(
  const string& text,
  int& value,
  const string& err)
{
  int i;
  size_t pos;
  try
  {
    i = stoi(text, &pos);
    if (pos != text.size())
    {
      cout << err << endl;
      return false;
    }
  }
  catch (const invalid_argument& ia)
  {
    UNUSED(ia);
    cout << err << endl;
    return false;
  }
  catch (const out_of_range& ia)
  {
    UNUSED(ia);
    cout << err << endl;
    return false;
  }

  value = i;
  return true;
}


bool readFloat(
  const string& text,
  float& value,
  const string& err)
{
  float f;
  size_t pos;
  try
  {
    f = static_cast<float>(stod(text, &pos));
    if (pos != text.size())
    {
      cout << err << endl;
      return false;
    }
  }
  catch (const invalid_argument& ia)
  {
    UNUSED(ia);
    cout << err << endl;
    return false;
  }
  catch (const out_of_range& ia)
  {
    UNUSED(ia);
    cout << err << endl;
    return false;
  }

  value = f;
  return true;
}


bool readBool(
  const string& text,
  bool& value,
  const string& err)
{
  if (text == "yes")
    value = true;
  else if (text == "no")
    value = false;
  else
  {
    cout << err << endl;
    return false;
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


bool fillInDistances(CarEntry& c)
{
  // Calculate missing distances.  Check that not over-specified.
  // Less than some small number means that the value was not set.
  // There are ways that the data can be consistent and still be
  // flagged below.  This is just a quick hack.

  int countEq1 = 0;
  if (c.distFrontToWheel < 10) countEq1++;
  if (c.distWheels < 10) countEq1++;
  if (c.distPair < 10) countEq1++;
  if (c.distWheelToBack < 10) countEq1++;

  if (countEq1 == 4)
  {
    const int s1 = c.distFrontToWheel + c.distWheels + 
      c.distPair + c.distWheelToBack;
    if (s1 != c.length)
      return false;
  }
  else if (countEq1 == 3)
  {
    if (c.distFrontToWheel < 10)
      c.distFrontToWheel = c.length - c.distFrontToWheel;
    else if (c.distWheels < 10)
      c.distWheels = c.length - c.distWheels;
    else if (c.distPair < 10)
      c.distPair = c.length - c.distPair;
    else if (c.distWheelToBack < 10)
      c.distWheelToBack = c.length - c.distWheelToBack;
    else
      return false;
  }
  else 
    return false;


  int countEq2a = 0;
  int countEq2b = 0;
  if (c.distMiddles < 10) countEq2a++;
  if (c.distPair < 10) countEq2b++;
  if (c.distWheels < 10) countEq2b++;

  const int s2 = c.distPair + c.distWheels;
  if (countEq2b == 2)
  {
    if (countEq2a == 0)
      c.distMiddles = s2;
    else if (c.distMiddles != s2)
      return false;
  }
  else if (countEq2b == 1)
  {
    if (countEq2a == 0)
      return false;
    else if (c.distMiddles < 10)
      c.distMiddles = s2;
    else if (c.distMiddles != s2)
      return false;
  }
  else
    return false;

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
      readUsage(rest, c.usage);
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

  if (! fillInDistances(c))
  {
    cout << "File " << fname << ": Could not fill in distances" << endl;
    return;
  }

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

