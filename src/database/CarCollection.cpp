#include "CarCollection.h"


#include <iostream>


CarCollection::CarCollection()
{
  CarCollection::reset();
}


void CarCollection::reset()
{
  fields.clear();
  fieldCounts.clear();
  entries.clear();

  CarCollection::configure();
}


void CarCollection::configure()
{
cout << "Arring in CarCollection::configure" << endl;
  fields =
    {
    { "OFFICIAL_NAME", CORRESPONDENCE_STRING, CAR_OFFICIAL_NAME },
    { "NAME", CORRESPONDENCE_STRING, CAR_NAME },
    { "CONFIGURATION", CORRESPONDENCE_STRING, CAR_CONFIGURATION_UIC },
    { "COMMENT", CORRESPONDENCE_STRING, CAR_COMMENT },

    { "USAGE", CORRESPONDENCE_STRING_VECTOR, CAR_USAGE },
    { "COUNTRIES", CORRESPONDENCE_STRING_VECTOR, CAR_COUNTRIES },

    { "INTRODUCTION", CORRESPONDENCE_INT, CAR_INTRODUCTION },
    { "CAPACITY", CORRESPONDENCE_INT, CAR_CAPACITY },
    { "CLASS", CORRESPONDENCE_INT, CAR_CLASS },
    { "WEIGHT", CORRESPONDENCE_INT, CAR_WEIGHT },
    { "WHEEL_LOAD", CORRESPONDENCE_INT, CAR_WHEEL_LOAD },
    { "SPEED", CORRESPONDENCE_INT, CAR_SPEED },
    { "LENGTH", CORRESPONDENCE_INT, CAR_LENGTH },
    { "DIST_WHEELS", CORRESPONDENCE_INT, CAR_DIST_WHEELS },
    { "DIST_WHEELS1", CORRESPONDENCE_INT, CAR_DIST_WHEELS1 },
    { "DIST_WHEELS2", CORRESPONDENCE_INT, CAR_DIST_WHEELS2 },
    { "DIST_MIDDLES", CORRESPONDENCE_INT, CAR_DIST_MIDDLES },
    { "DIST_PAIR", CORRESPONDENCE_INT, CAR_DIST_PAIR },
    { "DIST_FRONT_TO_WHEEL", CORRESPONDENCE_INT, CAR_DIST_FRONT_TO_WHEEL },
    { "DIST_WHEEL_TO_BACK", CORRESPONDENCE_INT, CAR_DIST_WHEEL_TO_BACK },
    { "DIST_FRONT_TO_MID1", CORRESPONDENCE_INT, CAR_DIST_FRONT_TO_MID1 },
    { "DIST_BACK_TO_MID2", CORRESPONDENCE_INT, CAR_DIST_BACK_TO_MID2 },

    { "POWER", CORRESPONDENCE_BOOL, CAR_POWER },
    { "RESTAURANT", CORRESPONDENCE_BOOL, CAR_RESTAURANT },
    { "SYMMETRY", CORRESPONDENCE_BOOL, CAR_SYMMETRY }
  };

  fieldCounts =
  {
    CAR_STRINGS_SIZE,
    CAR_VECTORS_SIZE,
    CAR_INTS_SIZE,
    CAR_BOOLS_SIZE
  };
}


bool CarCollection::fillInEquation(
  int &lhs,
  vector<int>& rhs,
  const unsigned len) const
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
    if (rhs[i] >= 0)
      countRHS++;
    else
      miss = static_cast<int>(i);

    sum += rhs[i];
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
      rhs[m] += lhs - sum;
    }
  }
  else
    return false;

  return true;
}


bool CarCollection::fillInDistances(Entity& entry) const
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

  bool done = false;
  int lhs;
  for (unsigned iter = 0; iter < 3 && ! done; iter++)
  {
    done = true;

    // Equation 1
    lhs = 2 * entry[CAR_DIST_MIDDLES];
    rhs[0] = 2 * entry[CAR_DIST_PAIR];
    rhs[1] = entry[CAR_DIST_WHEELS1];
    rhs[2] = entry[CAR_DIST_WHEELS2];

    if (fillInEquation(lhs, rhs, 3))
    {
      entry[CAR_DIST_MIDDLES] = lhs/2;
      entry[CAR_DIST_PAIR]  = rhs[0]/2;
      entry[CAR_DIST_WHEELS1] = rhs[1];
      entry[CAR_DIST_WHEELS1] = rhs[2];
    }
    else
      done = false;

    // Equation 2
    lhs = entry[CAR_LENGTH];
    rhs[0] = entry[CAR_DIST_FRONT_TO_WHEEL];
    rhs[1] = entry[CAR_DIST_WHEELS1];
    rhs[2] = entry[CAR_DIST_PAIR];
    rhs[3] = entry[CAR_DIST_WHEELS2];
    rhs[4] = entry[CAR_DIST_WHEEL_TO_BACK];

    if (fillInEquation(lhs, rhs, 5))
    {
      entry[CAR_LENGTH] = lhs;
      entry[CAR_DIST_FRONT_TO_WHEEL] = rhs[0];
      entry[CAR_DIST_WHEELS1] = rhs[1];
      entry[CAR_DIST_PAIR] = rhs[2];
      entry[CAR_DIST_WHEELS2] = rhs[3];
      entry[CAR_DIST_WHEEL_TO_BACK] = rhs[4];
    }
    else
      done = false;

    // Equation 3
    lhs = 2 * entry[CAR_DIST_FRONT_TO_MID1];
    rhs[0] = 2 * entry[CAR_DIST_FRONT_TO_WHEEL];
    rhs[1] = entry[CAR_DIST_WHEELS1];

    if (fillInEquation(lhs, rhs, 2))
    {
      entry[CAR_DIST_FRONT_TO_MID1] = lhs/2;
      entry[CAR_DIST_WHEELS1] = rhs[1];
      entry[CAR_DIST_FRONT_TO_WHEEL] = rhs[0]/2;
    }
    else
      done = false;

    // Equation 4
    lhs = 2 * entry[CAR_DIST_BACK_TO_MID2];
    rhs[0] = 2 * entry[CAR_DIST_WHEEL_TO_BACK];
    rhs[1] = entry[CAR_DIST_WHEELS2];

    if (fillInEquation(lhs, rhs, 2))
    {
      entry[CAR_DIST_BACK_TO_MID2] = lhs/2;
      entry[CAR_DIST_WHEELS2] = rhs[1];
      entry[CAR_DIST_WHEEL_TO_BACK] = rhs[0]/2;
    }
    else
      done = false;
  }

  return done;
}



void CarCollection::complete(Entity& entry)
{
  const unsigned dw = entry[CAR_DIST_WHEELS];
  if (dw > 0)
  {
    if (entry[CAR_DIST_WHEELS1] == -1)
      entry[CAR_DIST_WHEELS1] = dw;
    if (entry[CAR_DIST_WHEELS1] == -1)
      entry[CAR_DIST_WHEELS2] = dw;
  }

  entry.setBool(CAR_FOURWHEEL_FLAG,
    entry[CAR_DIST_WHEELS1] > 0 && entry[CAR_DIST_WHEELS2] > 0);

  CarCollection::fillInDistances(entry);
}


bool CarCollection::addAxles(
  const unsigned index,
  const bool reverseFlag,
  int& posRunning,
  vector<int>& axles) const
{
  if (index >= entries.size())
    return false;

  const Entity& entry = entries[index];
  const int dw1 = entry[CAR_DIST_WHEELS1];
  const int dw2 = entry[CAR_DIST_WHEELS2];

  if (! reverseFlag)
  {
    if (! axles.empty())
      posRunning += entry[CAR_DIST_FRONT_TO_WHEEL];

    axles.push_back(posRunning); // First pair, first wheel
  
    if (dw1 > 0)
    {
      posRunning += dw1;
      axles.push_back(posRunning);  // First pair, second wheel
    }

    posRunning += entry[CAR_DIST_PAIR];
    axles.push_back(posRunning); // Second pair, first wheel

    if (dw2 > 0)
    {
      posRunning += dw2;
      axles.push_back(posRunning); // Second pair, second wheel
    }

    posRunning += entry[CAR_DIST_WHEEL_TO_BACK];
  }
  else
  {
    // Car is reversed.
    if (! axles.empty())
      posRunning += entry[CAR_DIST_WHEEL_TO_BACK];

    axles.push_back(posRunning); // Second pair, second wheel

    if (dw2 > 0)
    {
      posRunning += dw2;
      axles.push_back(posRunning); // Second pair, first wheel
    }

    posRunning += entry[CAR_DIST_PAIR];
    axles.push_back(posRunning); // First pair, second wheel

    if (dw1 > 0)
    {
      posRunning += dw1;
      axles.push_back(posRunning);  // First pair, first wheel
    }

    posRunning += entry[CAR_DIST_FRONT_TO_WHEEL];
  }
  return true;
}

