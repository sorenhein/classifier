#ifndef TRAIN_STRUCT_H
#define TRAIN_STRUCT_H

#include <string>

using namespace std;


struct PeakPos
{
  double pos; // In m
  double value;
};

struct PeakTime
{
  double time; // In s
  double value;
};

enum TransientType
{
  TRANSIENT_NONE = 0,
  TRANSIENT_SMALL = 1,
  TRANSIENT_MEDIUM = 2,
  TRANSIENT_LARGE_POS = 3,
  TRANSIENT_LARGE_NEG = 4,
  TRANSIENT_SIZE = 5
};

struct Run
{
  unsigned first;
  unsigned len;
  bool posFlag;
  double cum;
};

enum QuietGrade
{
  GRADE_GREEN = 0,
  GRADE_AMBER = 1,
  GRADE_RED = 2,
  GRADE_DEEP_RED = 3,
  GRADE_SIZE = 4
};

// Not every field is used all the time.

struct Interval
{
  unsigned first;
  unsigned len;
  QuietGrade grade;
  double mean;
};

enum PeakParam
{
  PEAK_PARAM_AREA = 0,
  PEAK_PARAM_RANGE = 1,
  PEAK_PARAM_SIZE = 2
};

enum WheelType
{
  WHEEL_LEFT = 0,
  WHEEL_RIGHT = 1,
  WHEEL_ONLY = 2,
  WHEEL_SIZE = 3
};

enum BogieType
{
  BOGIE_LEFT = 0,
  BOGIE_RIGHT = 1,
  BOGIE_SIZE = 2
};

struct RecogEntry
{
  bool flag;
  unsigned value;

  bool match(const unsigned actual) const
  {
    return (! flag || value == actual);
  };
};

struct RecogParams
{
  RecogEntry source;
  RecogEntry sumGreat;
  RecogEntry starsGood;
};

enum PeakQuality
{
  PEAK_QUALITY_FANTASTIC = 0,
  PEAK_QUALITY_GREAT = 1,
  PEAK_QUALITY_GOOD = 2,
  PEAK_QUALITY_ACCEPTABLE = 3,
  PEAK_QUALITY_BORDERLINE = 4,
  PEAK_QUALITY_POOR = 5,
  PEAK_QUALITY_SIZE = 6
};


struct Recognizer
{
  RecogParams params;
  unsigned numWheels;
  PeakQuality quality;
  string text;
};

struct Imperfections
{
  // These are estimates and do not make use of the "known" true peaks.
  unsigned numSkipsOfReal;
  unsigned numSkipsOfSeen;

  unsigned numSpuriousFirst;
  unsigned numMissingFirst;

  unsigned numSpuriousLater;
  unsigned numMissingLater;

  void reset()
  {
    numSkipsOfReal = 0;
    numSkipsOfSeen = 0;
    numSpuriousFirst = 0;
    numMissingFirst = 0;
    numSpuriousLater = 0;
    numMissingLater = 0;
  };

  Imperfections()
  {
    Imperfections::reset();
  };
};

enum FindCarType
{
  FIND_CAR_MATCH = 0,
  FIND_CAR_PARTIAL = 1,
  FIND_CAR_DOWNGRADE = 2,
  FIND_CAR_NO_MATCH = 3,
  FIND_CAR_SIZE = 4
};

#endif
