#ifndef TRAIN_STRUCT_H
#define TRAIN_STRUCT_H

#include <string>

using namespace std;


/*
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
*/

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

/*
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
*/

enum FindCarType
{
  FIND_CAR_MATCH = 0,
  FIND_CAR_PARTIAL = 1,
  FIND_CAR_DOWNGRADE = 2,
  FIND_CAR_NO_MATCH = 3,
  FIND_CAR_SIZE = 4
};

#endif
