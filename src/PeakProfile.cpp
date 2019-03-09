#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakProfile.h"
#include "Peak.h"
#include "PeakPtrs.h"
#include "Except.h"
#include "errors.h"


PeakProfile::PeakProfile()
{
  PeakProfile::reset();
}


PeakProfile::~PeakProfile()
{
}


void PeakProfile::reset()
{
  bogeyWheels.clear();
  wheels.clear();
  stars.clear();

  bogeyWheels.resize(4);
  wheels.resize(2);
  stars.resize(4);

  sumGreat = 0;
  sumSelected = 0;
  sum = 0;
}


void PeakProfile::make(
  const PeakPtrs& peakPtrs,
  const unsigned sourceIn)
{
  source = sourceIn;

  PeakProfile::reset();
  for (auto it = peakPtrs.cbegin(); it != peakPtrs.cend(); it++)
  {
    Peak * const peakPtr = * it;
    if (peakPtr->isSelected())
      sumSelected++;

    if (peakPtr->isLeftBogey())
    {
      if (peakPtr->isLeftWheel())
        bogeyWheels[0]++;
      else if (peakPtr->isRightWheel())
        bogeyWheels[1]++;
      else
        THROW(ERR_ALGO_PEAK_STRUCTURE, "Left bogey has no wheel mark");

      sumGreat++;
    }
    else if (peakPtr->isRightBogey())
    {
      if (peakPtr->isLeftWheel())
        bogeyWheels[2]++;
      else if (peakPtr->isRightWheel())
        bogeyWheels[3]++;
      else
        THROW(ERR_ALGO_PEAK_STRUCTURE, "Right bogey has no wheel mark");

      sumGreat++;
    }
    else if (peakPtr->isLeftWheel())
    {
      wheels[0]++;
      sumGreat++;
    }
    else if (peakPtr->isRightWheel())
    {
      wheels[1]++;
      sumGreat++;
    }
    else
    {
      const string st = peakPtr->stars();
      if (st == "***")
      {
        stars[0]++;
        sumGreat++;
      }
      else if (st == "**")
        stars[1]++;
      else if (st == "*")
        stars[2]++;
      else
        stars[3]++;
    }
    sum++;
  }
}


unsigned PeakProfile::numGood() const
{
  return sumGreat + stars[1];
}


unsigned PeakProfile::numGreat() const
{
  return sumGreat;
}


unsigned PeakProfile::numSelected() const
{
  return sumSelected;
}


unsigned PeakProfile::num() const
{
  return sum;
}


bool PeakProfile::match(const Recognizer& recog) const
{
  return (recog.params.source.match(source) &&
    recog.params.sumGreat.match(sumGreat) &&
    recog.params.starsGood.match(stars[1]));
}


bool PeakProfile::looksEmpty() const
{
  return (sumGreat == 0 && stars[1] == 0);
}


bool PeakProfile::looksEmptyLast() const
{
  return (sumGreat <= 1 &&
    bogeyWheels[0] == 0 &&
    bogeyWheels[1] == 0 &&
    bogeyWheels[2] == 0 &&
    bogeyWheels[3] == 0 &&
    wheels[0] == 0 &&
    wheels[1] == 0);
}


bool PeakProfile::looksLikeTwoCars() const
{
  return (sumGreat >= 7 && 
     sumGreat + stars[1] == 8);
}


bool PeakProfile::looksLong() const
{
  return (sumGreat >= 5);
}


string PeakProfile::strEntry(const unsigned value) const
{
  if (value == 0)
    return "-";
  else
    return to_string(value);
}


string PeakProfile::str() const
{
  string src = "";
  if (source == PEAK_SOURCE_FIRST)
    src = "first";
  else if (source == PEAK_SOURCE_INNER)
    src = "inner";
  else if (source == PEAK_SOURCE_LAST)
    src = "last ";

  stringstream ss;
  ss << "PROFILE " << src << ": " <<
    PeakProfile::strEntry(bogeyWheels[0]) << " " <<
    PeakProfile::strEntry(bogeyWheels[1]) << " " <<
    PeakProfile::strEntry(bogeyWheels[2]) << " " <<
    PeakProfile::strEntry(bogeyWheels[3]) << ", wh " <<
    PeakProfile::strEntry(wheels[0]) << " " <<
    PeakProfile::strEntry(wheels[1]) << ", q " <<
    PeakProfile::strEntry(stars[0]) << " " <<
    PeakProfile::strEntry(stars[1]) << " " <<
    PeakProfile::strEntry(stars[2]) << " " <<
    PeakProfile::strEntry(stars[3]) << " (" <<
    sumGreat << " of " <<
    sum << ")\n";
  return ss.str();
}

