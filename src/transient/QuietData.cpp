#include <iostream>
#include <iomanip>
#include <sstream>

#include "QuietData.h"

#include "../const.h"


void QuietData::setGrade()
{
  const float absmean = abs(mean);

  if (absmean < MEAN_VERY_QUIET && sdev < SDEV_VERY_QUIET)
    grade = GRADE_GREEN;
  else if (absmean < MEAN_SOMEWHAT_QUIET && sdev < SDEV_SOMEWHAT_QUIET)
    grade = GRADE_AMBER;
  else if (absmean < MEAN_QUIET_LIMIT && sdev < SDEV_SOMEWHAT_QUIET)
    grade = GRADE_RED;
  else if (absmean < MEAN_SOMEWHAT_QUIET && sdev < SDEV_QUIET_LIMIT)
    grade = GRADE_RED;
  else
    grade = GRADE_DEEP_RED;
}


bool QuietData::isVeryQuiet() const
{
  return (abs(mean) < MEAN_VERY_QUIET);
}


float QuietData::writeLevel() const
{
  // Values are pretty random, but these values tend to show up
  // in plots at sensible levels.
  if (grade == GRADE_GREEN)
    return 3.;
  else if (grade == GRADE_AMBER)
    return 1.5;
  else
    return 0.;
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
