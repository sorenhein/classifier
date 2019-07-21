#ifndef TRAIN_STRUCT_H
#define TRAIN_STRUCT_H

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <limits>

using namespace std;


#define G_FORCE 9.8f


struct Control
{
  string carDir; // Directory of car files
  string trainDir; // Directory of train files
  string correctionDir; // Directory of residual correction files
  string sensorFile; // File with sensor data
  string truthFile; // File with trace truth data
  string country; // Country of input traces
  string traceDir; // In 32-bit format
  string overviewFile; // Output file for overview train tables
  string detailFile; // Output file for detailed train tables

  string controlFile;
  string pickFileString;
  string pickTrainString;
  string summaryFile;
  bool summaryAppendFlag;

  bool verboseTransientMatch;
  bool verboseAlignMatches;
  bool verboseAlignPeaks;
  bool verboseRegressMatch;
  bool verboseRegressMotion;
  bool verbosePeakReduce;

  bool writingTransient;
  bool writingBack;
  bool writingFront;
  bool writingSpeed;
  bool writingPos;
  bool writingPeak;
  bool writingOutline;
};

struct Log
{
  bool flag;
  ofstream flog;
};

struct TraceTruth
{
  string filename;
  string trainName;
  int numAxles;
  double speed;
  double accel;
  bool reverseFlag;
};

struct TrainData
{
  string trainName;
  int numAxles;
  double speed;
  double accel;
};

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

enum FileFormat
{
  FILE_TXT = 0,
  FILE_CSV = 1,
  FILE_DAT = 2,
  FILE_SIZE = 3
};

struct InputEntry
{
  int number;
  string tag;
  string date;
  string time;
  vector<PeakTime> actual;
};

struct HistMatch
{
  unsigned trainNo;
  double scale;
};

struct HistWarp
{
  double dist;
  double scale;
};

struct RegrEntry
{
  unsigned index;
  double value;
  double valueSq;
  double frac;

  bool operator < (const RegrEntry& regr2)
  {
    return (valueSq < regr2.valueSq);
  };
};

struct Alignment
{
  unsigned trainNo;
  unsigned numAdd;
  unsigned numDelete;
  vector<int> actualToRef;

  double dist;
  double distMatch;
  vector<RegrEntry> topResiduals;

  Alignment()
  {
    topResiduals.clear();
  };

  bool operator < (const Alignment& a2)
  {
    // Cars have same length?
    if (numDelete + a2.numAdd == a2.numDelete + numAdd)
      return (dist < a2.dist);

    // Only look closely at good matches.
    if (distMatch > 3. || a2.distMatch > 3.)
      return (dist < a2.dist);

    // One match distance is clearly better?
    if (distMatch < 0.7 * a2.distMatch)
      return true;
    else if (a2.distMatch < 0.7 * distMatch)
      return false;

    if (numDelete + a2.numAdd > a2.numDelete + numAdd)
    {
      // This car has more wheels.
      if (distMatch < a2.distMatch && dist < 2. * a2.dist)
        return true;
    }
    else
    {
      // a2 car has more wheels.
      if (a2.distMatch < distMatch && a2.dist < 2. * dist)
        return false;
    }

    // Unclear.
    return (dist < a2.dist);
  }
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

enum PeakSeenType
{
  PEAK_SEEN_TOO_EARLY = 0,
  PEAK_SEEN_EARLY = 1,
  PEAK_SEEN_CORE = 2,
  PEAK_SEEN_LATE = 3,
  PEAK_SEEN_TOO_LATE = 4,
  PEAK_SEEN_SIZE = 5
};

enum PeakTrueType
{
  PEAK_TRUE_TOO_EARLY = 0,
  PEAK_TRUE_MISSED = 1,
  PEAK_TRUE_TOO_LATE = 2,
  PEAK_TRUE_SIZE = 3
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

enum PeakSource
{
  PEAK_SOURCE_FIRST = 1,
  PEAK_SOURCE_INNER = 2,
  PEAK_SOURCE_LAST = 4,
  PEAK_SOURCE_SIZE = 8
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

struct MatchData
{
  float distance;
  unsigned index;
  bool reverseFlag;

  string strIndex() const
  {
    return to_string(index) + (reverseFlag ? "R" : "");
  };
};

enum CarPosition
{
  CARPOSITION_FIRST = 0,
  CARPOSITION_INNER_SINGLE = 1,
  CARPOSITION_INNER_MULTI = 2,
  CARPOSITION_LAST = 3,
  CARPOSITION_SIZE = 4
};

struct DistEntry
{
  unsigned index;
  unsigned indexHi; // Sometimes set to denote a range
  int direction;
  unsigned origin;
  int count;
  int cumul;

  bool operator < (const DistEntry& de2)
  {
    return (index < de2.index);
  };

  string str() const
  {
    stringstream ss;
    ss << "index " << index << "(" << indexHi << ")";
    return ss.str();
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
