#include <iostream>
#include <iomanip>
#include <sstream>

#include "misc.h"

#include "../const.h"


float ratioCappedUnsigned(
  const unsigned num,
  const unsigned denom,
  const float rMax)
{
  return ratioCappedFloat(
    static_cast<float>(num),
    static_cast<float>(denom),
    rMax);
}


float ratioCappedFloat(
  const float num,
  const float denom,
  const float rMax)
{
  if (denom == 0)
    return rMax;

  const float invMax = 1.f / rMax;

  if (num == 0)
    return invMax;

  const float f = num / denom;
  if (f > rMax)
    return rMax;
  else if (f < invMax)
    return invMax;
  else
    return f;
}


string percent(
  const unsigned num,
  const unsigned denom,
  const unsigned decimals)
{
  stringstream ss;

  if (denom == 0 || num == 0)
    ss << "-";
  else
    ss << fixed << setprecision(static_cast<int>(decimals)) <<
      100. * num / static_cast<double>(denom) << "%";

  return ss.str();
}


void printVectorCSV(
  const string& title,
  const vector<double>& peaks,
  const int level)
{
  cout << title << "\n";
  for (unsigned i = 0; i < peaks.size(); i++)
    cout << fixed << setprecision(4) << 
      peaks[i] << SEPARATOR << level << endl;
  cout << endl;
}

