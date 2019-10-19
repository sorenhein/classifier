#ifndef TRAIN_TRANS_H
#define TRAIN_TRANS_H

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

using namespace std;


enum TransientStatus
{
  TSTATUS_TOO_FEW_RUNS = 0,
  TSTATUS_NO_CANDIDATE_RUN = 1,
  TSTATUS_NO_FULL_DECLINE = 2,
  TSTATUS_NO_MID_DECLINE = 3,
  TSTATUS_BAD_FIT = 4,
  TSTATUS_SIZE = 5
};

enum TransientType
{
  TRANSIENT_NONE = 0,
  TRANSIENT_FOUND = 1,
  TRANSIENT_SIZE = 6
};

struct Run
{
  unsigned first;
  unsigned len;
  bool posFlag;
  float cum;
};

enum QuietGrade
{
  GRADE_GREEN = 0,
  GRADE_AMBER = 1,
  GRADE_RED = 2,
  GRADE_SIZE = 3
};

struct QuietInterval
{
  unsigned first;
  unsigned len;
  float mean;
  float sdev;
  QuietGrade grade;
  bool _meanIsQuiet;

  void setGrade(
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
  };

  bool meanIsQuiet() const
  {
    return _meanIsQuiet;
  };

  string str() const
  {
    stringstream ss;
    ss <<
      setw(6) << right << first <<
      setw(6) << len <<
      setw(10) << fixed << setprecision(2) << mean <<
      setw(10) << fixed << setprecision(2) << sdev <<
      setw(6) << grade << "\n";
  return ss.str();

  };
};

#endif
