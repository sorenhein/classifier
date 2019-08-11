#include "CarDB.h"

#include <iostream>
#include <iomanip>
#include <sstream>

#include "../PeakGeneral.h"


CarDB::CarDB()
{
  CarDB::reset();
}


CarDB::~CarDB()
{
}


void CarDB::reset()
{
  fields.clear();
  fieldCounts.clear();

  entries.clear();
  entries.emplace_back(Entity());

  offCarMap.clear();

  CarDB::configure();
}


void CarDB::configure()
{
  fields =
  {
    // Example: Avmz 801
    { "OFFICIAL_NAME", CORRESPONDENCE_STRING, CAR_OFFICIAL_NAME },
    // Example: "DB Class 801 First Class Intermediate Car"
    { "NAME", CORRESPONDENCE_STRING, CAR_NAME },
    // Example: 2'2' or Bo'Bo
    { "CONFIGURATION", CORRESPONDENCE_STRING, CAR_CONFIGURATION_UIC },
    // Example: "second"
    { "CLASS", CORRESPONDENCE_STRING, CAR_CLASS },
    { "COMMENT", CORRESPONDENCE_STRING, CAR_COMMENT },

    // Example: "ICE1"
    { "USAGE", CORRESPONDENCE_STRING_VECTOR, CAR_USAGE },
    // List of comma-separated three-letter codes, e.g. "DEU,SWE"
    { "COUNTRIES", CORRESPONDENCE_STRING_VECTOR, CAR_COUNTRIES },

    // Example: 1990
    { "INTRODUCTION", CORRESPONDENCE_INT, CAR_INTRODUCTION },
    // Example: 53 (passengers)
    { "CAPACITY", CORRESPONDENCE_INT, CAR_CAPACITY },
    // Example: 50000 (kg)
    { "WEIGHT", CORRESPONDENCE_INT, CAR_WEIGHT },
    // Example: 20100 (kg)
    { "WHEEL_LOAD", CORRESPONDENCE_INT, CAR_WHEEL_LOAD },
    // Example: 280 (km/h)
    { "SPEED", CORRESPONDENCE_INT, CAR_SPEED },
    // Example: 24780 (mm)
    { "LENGTH", CORRESPONDENCE_INT, CAR_LENGTH },
    // Example: 2500 (mm)
    { "DIST_WHEELS", CORRESPONDENCE_INT, CAR_DIST_WHEELS },
    // Example: 2500 (mm)
    { "DIST_WHEELS1", CORRESPONDENCE_INT, CAR_DIST_WHEELS1 },
    // Example: 2500 (mm)
    { "DIST_WHEELS2", CORRESPONDENCE_INT, CAR_DIST_WHEELS2 },
    // Example: 19000 (mm)
    { "DIST_MIDDLES", CORRESPONDENCE_INT, CAR_DIST_MIDDLES },
    // Example: 14880 (mm)
    { "DIST_PAIR", CORRESPONDENCE_INT, CAR_DIST_PAIR },
    // Example: 2450 (mm)
    { "DIST_FRONT_TO_WHEEL", CORRESPONDENCE_INT, CAR_DIST_FRONT_TO_WHEEL },
    // Example: 1375 (mm)
    { "DIST_WHEEL_TO_BACK", CORRESPONDENCE_INT, CAR_DIST_WHEEL_TO_BACK },
    // Example: 3700 (mm)
    { "DIST_FRONT_TO_MID1", CORRESPONDENCE_INT, CAR_DIST_FRONT_TO_MID1 },
    // Example: 3900 (mm)
    { "DIST_BACK_TO_MID2", CORRESPONDENCE_INT, CAR_DIST_BACK_TO_MID2 },

    // Example: "no"
    { "POWER", CORRESPONDENCE_BOOL, CAR_POWER },
    // Example: "no"
    { "RESTAURANT", CORRESPONDENCE_BOOL, CAR_RESTAURANT },
    // Example: "yes"
    { "SYMMETRY", CORRESPONDENCE_BOOL, CAR_SYMMETRY }
  };

  fieldCounts =
  {
    CAR_STRINGS_SIZE,
    CAR_STRING_VECTORS_SIZE,
    0,
    0,
    0,
    CAR_INTS_SIZE,
    CAR_BOOLS_SIZE,
    0
  };
}


bool CarDB::fillInEquation(
  int &lhs,
  vector<int>& rhs,
  bool& inconsistentFlag,
  const unsigned len) const
{
  // Equation: lhs = sum(rhs).
  // If all are given, check consistency.
  // If one is missing, calculate it.
  // Returns true if an equation could be completed.
  // Otherwise sets inconsistentFlag if there is one.
  
  inconsistentFlag = false;

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
    if (rhs[i] >= 0)
    {
      countRHS++;
      sum += rhs[i];
    }
    else
      miss = static_cast<int>(i);
  }


  if (countRHS == len)
  {
    if (countLHS == 0)
    {
      lhs = sum;
      return true;
    }
    else if (lhs == sum)
      return false;
    else
    {
      inconsistentFlag = true;
      return false;
    }
  }
  else if (countRHS == len-1)
  {
    if (countLHS == 0)
    {
      inconsistentFlag = false;
      return false;
    }
    else
    {
      const unsigned m = static_cast<unsigned>(miss);
      rhs[m] = lhs - sum;
      return true;
    }
  }
  else
    return false;
}


bool CarDB::fillInDistances(Entity& entry) const
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

  vector<int> rhs(5);

  bool changeFlag = true;
  bool inconsistentFlag = false;
  int lhs;
  for (unsigned iter = 0; iter < 3 && changeFlag; iter++)
  {
    changeFlag = false;

    // Equation 1
    lhs = 2 * entry[CAR_DIST_MIDDLES];
    rhs[0] = 2 * entry[CAR_DIST_PAIR];
    rhs[1] = entry[CAR_DIST_WHEELS1];
    rhs[2] = entry[CAR_DIST_WHEELS2];

    if (fillInEquation(lhs, rhs, inconsistentFlag, 3))
    {
      entry[CAR_DIST_MIDDLES] = lhs/2;
      entry[CAR_DIST_PAIR]  = rhs[0]/2;
      entry[CAR_DIST_WHEELS1] = rhs[1];
      entry[CAR_DIST_WHEELS2] = rhs[2];
      changeFlag = true;
    }
    else if (inconsistentFlag)
      return false;

    // Equation 2
    lhs = entry[CAR_LENGTH];
    rhs[0] = entry[CAR_DIST_FRONT_TO_WHEEL];
    rhs[1] = entry[CAR_DIST_WHEELS1];
    rhs[2] = entry[CAR_DIST_PAIR];
    rhs[3] = entry[CAR_DIST_WHEELS2];
    rhs[4] = entry[CAR_DIST_WHEEL_TO_BACK];

    if (fillInEquation(lhs, rhs, inconsistentFlag, 5))
    {
      entry[CAR_LENGTH] = lhs;
      entry[CAR_DIST_FRONT_TO_WHEEL] = rhs[0];
      entry[CAR_DIST_WHEELS1] = rhs[1];
      entry[CAR_DIST_PAIR] = rhs[2];
      entry[CAR_DIST_WHEELS2] = rhs[3];
      entry[CAR_DIST_WHEEL_TO_BACK] = rhs[4];
      changeFlag = true;
    }
    else if (inconsistentFlag)
      return false;

    // Equation 3
    lhs = 2 * entry[CAR_DIST_FRONT_TO_MID1];
    rhs[0] = 2 * entry[CAR_DIST_FRONT_TO_WHEEL];
    rhs[1] = entry[CAR_DIST_WHEELS1];

    if (fillInEquation(lhs, rhs, inconsistentFlag, 2))
    {
      entry[CAR_DIST_FRONT_TO_MID1] = lhs/2;
      entry[CAR_DIST_WHEELS1] = rhs[1];
      entry[CAR_DIST_FRONT_TO_WHEEL] = rhs[0]/2;
      changeFlag = true;
    }
    else if (inconsistentFlag)
      return false;

    // Equation 4
    lhs = 2 * entry[CAR_DIST_BACK_TO_MID2];
    rhs[0] = 2 * entry[CAR_DIST_WHEEL_TO_BACK];
    rhs[1] = entry[CAR_DIST_WHEELS2];

    if (fillInEquation(lhs, rhs, inconsistentFlag, 2))
    {
      entry[CAR_DIST_BACK_TO_MID2] = lhs/2;
      entry[CAR_DIST_WHEELS2] = rhs[1];
      entry[CAR_DIST_WHEEL_TO_BACK] = rhs[0]/2;
      changeFlag = true;
    }
    else if (inconsistentFlag)
      return false;
  }

  return ! changeFlag;
}



bool CarDB::complete(Entity& entry)
{
  const int dw = entry[CAR_DIST_WHEELS];
  if (dw > 0)
  {
    if (entry[CAR_DIST_WHEELS1] == -1)
      entry[CAR_DIST_WHEELS1] = dw;
    if (entry[CAR_DIST_WHEELS2] == -1)
      entry[CAR_DIST_WHEELS2] = dw;
  }

  entry.setBool(CAR_FOURWHEEL_FLAG,
    entry[CAR_DIST_WHEELS1] > 0 && entry[CAR_DIST_WHEELS2] > 0);

  return CarDB::fillInDistances(entry);
}


bool CarDB::readFile(const string& fname)
{
  Entity entry;
  if (! entry.readTagFile(fname, fields, fieldCounts))
    return false;

  if (! CarDB::complete(entry))
    return false;

  offCarMap[entry.getString(CAR_OFFICIAL_NAME)] = entries.size();

  entries.push_back(entry);
  return true;
}


bool CarDB::appendAxles(
  const int carNo,
  int& posRunning,
  int& carRunning,
  PeaksInfo& peaksInfo) const
{
  if (carNo == 0)
    return false;

  unsigned index;
  bool reverseFlag;
  if (carNo > 0)
  {
    index = static_cast<unsigned>(carNo);
    reverseFlag = false;
  }
  else
  {
    index = static_cast<unsigned>(-carNo);
    reverseFlag = true;
  }

  if (index >= entries.size())
    return false;

  const Entity& entry = entries[index];
  const int dw1 = entry[CAR_DIST_WHEELS1];
  const int dw2 = entry[CAR_DIST_WHEELS2];

  int numberInCar = 0;
  if (! reverseFlag)
  {
    if (! peaksInfo.positions.empty())
      posRunning += entry[CAR_DIST_FRONT_TO_WHEEL];

    // First pair, first wheel
    peaksInfo.extend(posRunning, carRunning, numberInCar);
  
    if (dw1 > 0)
    {
      // First pair, second wheel
      posRunning += dw1;
      peaksInfo.extend(posRunning, carRunning, numberInCar);
    }

    // Second pair, first wheel
    posRunning += entry[CAR_DIST_PAIR];
    peaksInfo.extend(posRunning, carRunning, numberInCar);

    if (dw2 > 0)
    {
      // Second pair, second wheel
      posRunning += dw2;
      peaksInfo.extend(posRunning, carRunning, numberInCar);
    }

    posRunning += entry[CAR_DIST_WHEEL_TO_BACK];
  }
  else
  {
    // Car is reversed.
    if (! peaksInfo.positions.empty())
      posRunning += entry[CAR_DIST_WHEEL_TO_BACK];

    // Second pair, second wheel
    peaksInfo.extend(posRunning, carRunning, numberInCar);

    if (dw2 > 0)
    {
      // Second pair, first wheel
      posRunning += dw2;
      peaksInfo.extend(posRunning, carRunning, numberInCar);
    }

    // First pair, second wheel
    posRunning += entry[CAR_DIST_PAIR];
    peaksInfo.extend(posRunning, carRunning, numberInCar);

    if (dw1 > 0)
    {
      // First pair, first wheel
      posRunning += dw1;
      peaksInfo.extend(posRunning, carRunning, numberInCar);
    }

    posRunning += entry[CAR_DIST_FRONT_TO_WHEEL];
  }

  carRunning++;
  return true;
}


int CarDB::lookupNumber(const string& offName) const
{
  auto it = offCarMap.find(offName);
  if (it == offCarMap.end())
    return 0; // Invalid number
  else
    return it->second;
}


string CarDB::strDistances(const Entity& entry) const
{
  stringstream ss;

  ss << 
    setw(22) << left << "LENGTH" << entry[CAR_LENGTH] << "\n" << 
    setw(22) << "DIST_WHEELS" << entry[CAR_DIST_WHEELS] << "\n" <<
    setw(22) << "DIST_WHEELS1" << entry[CAR_DIST_WHEELS1] << "\n" <<
    setw(22) << "DIST_WHEELS2" << entry[CAR_DIST_WHEELS2] << "\n" <<
    setw(22) << "DIST_MIDDLES" << entry[CAR_DIST_MIDDLES] << "\n" <<
    setw(22) << "DIST_PAIR" << entry[CAR_DIST_PAIR] << "\n" <<
    setw(22) << "DIST_FRONT_TO_WHEEL" << 
      entry[CAR_DIST_FRONT_TO_WHEEL] << "\n" <<
    setw(22) << "DIST_WHEEL_TO_BACK" << 
      entry[CAR_DIST_WHEEL_TO_BACK] << "\n" <<
    setw(22) << "DIST_FRONT_TO_MID1" << 
      entry[CAR_DIST_FRONT_TO_MID1] << "\n" <<
    setw(22) << "DIST_BACK_TO_MID2" << 
      entry[CAR_DIST_BACK_TO_MID2] << "\n\n";

  return ss.str();
}

