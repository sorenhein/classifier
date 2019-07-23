#include <iostream>
#include <iomanip>
#include <sstream>

#include "misc.h"


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
    ss << fixed << setprecision(decimals) <<
      100. * num / static_cast<double>(denom) << "%";

  return ss.str();
}

