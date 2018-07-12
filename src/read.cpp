#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#pragma warning(push)
#pragma warning(disable: 4365 4571 4625 4626 4774 5026 5027)
#if defined(__CYGWIN__)
  #include "dirent/dirent.h"
#elif defined(_WIN32)
  #include "dirent/dirent.h"
  #include <windows.h>
  #include "Shlwapi.h"
#else
  #include <dirent.h>
#endif
#pragma warning(pop)


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
  vector<string>& value);

bool readBool(
  const string& text,
  bool& value,
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
  c.capacity = 0;
  c.level = TRAIN_NONE;
  c.restaurantFlag = false;
  c.weight = 0;
  c.wheelLoad = 0;
  c.speed = 1; // Something non-zero in case we divide by it
  c.configurationUIC = "";
  c.length = 4; // Something non-zero in case we divide by it
  c.symmetryFlag = false;

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

      string bracketed = s.substr(pos+1);
      bracketed.pop_back();
      if (! readInt(bracketed, count, err))
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
  if (text == "")
    return false;

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


bool readDouble(
  const string& text,
  double& value,
  const string& err)
{
  if (text == "")
    return false;

  double f;
  size_t pos;
  try
  {
    f = stod(text, &pos);
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
      miss = i;

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
      * rhs[miss] = lhs - (sum - * rhs[miss]);
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
    if (line == "" || line.front() == '#')
      continue;
    
    const string err = "File " + fname + ": Bad line '" + line + "'";

    unsigned sp = line.find(" ");
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
      readUsage(rest, c.usage);
    else if (field == "COUNTRIES")
      readCountries(rest, c.countries);
    else if (field == "INTRODUCTION")
    {
      if (! readInt(rest, c.introduction, err)) break;
    }
    else if (field == "POWER")
    {
      if (! readBool(rest, c.powerFlag, err)) break;
    }
    else if (field == "CAPACITY")
    {
      if (! readInt(rest, c.capacity, err)) break;
    }
    else if (field == "CLASS")
    {
      if (! readLevel(rest, c.level, err)) break;
    }
    else if (field == "RESTAURANT")
    {
      if (! readBool(rest, c.restaurantFlag, err)) break;
    }
    else if (field == "WEIGHT")
    {
      if (! readInt(rest, c.weight, err)) break;
    }
    else if (field == "WHEEL_LOAD")
    {
      if (! readInt(rest, c.wheelLoad, err)) break;
    }
    else if (field == "SPEED")
    {
      if (! readInt(rest, c.speed, err)) break;
    }
    else if (field == "CONFIGURATION")
      c.configurationUIC = rest;
    else if (field == "LENGTH")
    {
      if (! readInt(rest, c.length, err)) break;
    }
    else if (field == "SYMMETRY")
    {
      if (! readBool(rest, c.symmetryFlag, err)) break;
    }
    else if (field == "LENGTH")
    {
      if (! readInt(rest, c.length, err)) break;
    }
    else if (field == "DIST_WHEELS")
    {
      if (! readInt(rest, c.distWheels, err)) break;
      c.distWheels1 = c.distWheels;
      c.distWheels2 = c.distWheels;
    }
    else if (field == "DIST_WHEELS1")
    {
      if (! readInt(rest, c.distWheels1, err)) break;
    }
    else if (field == "DIST_WHEELS2")
    {
      if (! readInt(rest, c.distWheels2, err)) break;
    }
    else if (field == "DIST_MIDDLES")
    {
      if (! readInt(rest, c.distMiddles, err)) break;
    }
    else if (field == "DIST_PAIR")
    {
      if (! readInt(rest, c.distPair, err)) break;
    }
    else if (field == "DIST_FRONT_TO_WHEEL")
    {
      if (! readInt(rest, c.distFrontToWheel, err)) break;
    }
    else if (field == "DIST_WHEEL_TO_BACK")
    {
      if (! readInt(rest, c.distWheelToBack, err)) break;
    }
    else if (field == "DIST_FRONT_TO_MID1")
    {
      if (! readInt(rest, c.distFrontToMid1, err)) break;
    }
    else if (field == "DIST_BACK_TO_MID2")
    {
      if (! readInt(rest, c.distBackToMid2, err)) break;
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

    const string& field = line.substr(0, sp);
    const string& rest = line.substr(sp+1);

    if (field == "OFFICIAL_NAME")
      t.officialName = rest;
    else if (field == "NAME")
      t.name = rest;
    else if (field == "INTRODUCTION")
    {
      if ( ! readInt(rest, t.introduction, err)) break;
    }
    else if (field == "RETIREMENT")
    {
      if ( ! readInt(rest, t.retirement, err)) break;
    }
    else if (field == "COUNTRIES")
      readCountries(rest, t.countries);
    else if (field == "SYMMETRY")
    {
      if (! readBool(rest, t.symmetryFlag, err)) break;
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

