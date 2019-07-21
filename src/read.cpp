#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "Database.h"
#include "TraceDB.h"
#include "read.h"

#include "util/parse.h"
#include "util/misc.h"

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


void readCarFiles(
  Database& db,
  const string& dir)
{
  vector<string> textfiles;
  getFilenames(dir, textfiles);

  for (auto &f: textfiles)
    db.readCarFile(f);
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
      if ( ! parseDouble(rest, c.speedMin)) 
      {
        cout << err << endl;
        break;
      }
    }
    else if (field == "SPEED_MAX")
    {
      if ( ! parseDouble(rest, c.speedMax)) 
      {
        cout << err << endl;
        break;
      }
    }
    else if (field == "SPEED_STEP")
    {
      if ( ! parseDouble(rest, c.speedStep)) 
      {
        cout << err << endl;
        break;
      }
    }
    else if (field == "ACCELERATION_MIN")
    {
      if ( ! parseDouble(rest, c.accelMin)) 
      {
        cout << err << endl;
        break;
      }
    }
    else if (field == "ACCELERATION_MAX")
    {
      if ( ! parseDouble(rest, c.accelMax)) 
      {
        cout << err << endl;
        break;
      }
    }
    else if (field == "ACCELERATION_STEP")
    {
      if ( ! parseDouble(rest, c.accelStep)) 
      {
        cout << err << endl;
        break;
      }
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


void readTrainFiles(
  Database& db,
  const string& dir,
  const string& correctionDir)
{
  vector<string> correctionFiles;
  getFilenames(correctionDir, correctionFiles);

  for (auto &f: correctionFiles)
    db.readCorrectionFile(f);

  vector<string> textfiles;
  getFilenames(dir, textfiles);

  for (auto &f: textfiles)
    db.readTrainFile(f);
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

    if (! parseDouble(v[4], truth.speed))
    {
      cout << err << endl;
      fin.close();
      return false;
    }

    if (! parseDouble(v[5], truth.accel))
    {
      cout << err << endl;
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

