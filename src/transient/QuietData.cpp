#include <iostream>
#include <iomanip>
#include <sstream>

#include "QuietData.h"

#include "../const.h"


void QuietData::setGrade(
  const float& meanSomewhatQuiet,
  const float& meanQuietLimit,
  const float& SdevMeanFactor)
{
  const float absmean = abs(mean);
  const float sdevMeans = sdev / SdevMeanFactor;

  if (absmean < meanSomewhatQuiet && sdevMeans < meanSomewhatQuiet)
    grade = GRADE_GREEN;
  else if (absmean < meanQuietLimit && sdevMeans < meanSomewhatQuiet)
    grade = GRADE_AMBER;
  else if (absmean < meanSomewhatQuiet && sdevMeans < SdevMeanFactor)
    grade = GRADE_AMBER;
  else
    grade = GRADE_RED;

  _meanIsQuiet = (absmean < meanSomewhatQuiet);
}


bool QuietData::meanIsQuiet() const
{
  return _meanIsQuiet;
}


string QuietData::str() const
{
  stringstream ss;
  ss <<
    setw(6) << right << start <<
    setw(6) << len <<
    setw(10) << fixed << setprecision(2) << mean <<
    setw(10) << fixed << setprecision(2) << sdev <<
    setw(6) << grade << "\n";
  return ss.str();
}
