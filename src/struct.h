#ifndef TRAIN_STRUCT_H
#define TRAIN_STRUCT_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;


#define G_FORCE 9.8


struct Control
{
  string carDir; // Directory of car files
  string trainDir; // Directory of train files
  string sensorFile; // File with sensor data
  string truthFile; // File with trace truth data
  string country; // Country of input traces
  int year; // Year of input traces
  string disturbFile; // File with data for random disturbance
  string inputFile; // In Chris' chosen format
  string traceDir; // In 32-bit format
  int simCount; // Number of simulations for a given train, v and a
  double speedMin; // Lower speed for simulation loop, in m/s
  double speedMax; // Upper speed for simulation loop, in m/s
  double speedStep; // Step size, in m/s
  double accelMin; // Lower acceleration for simulation loop, in m/s^2
  double accelMax; // Upper acceleration for simulation loop, in m/s^2
  double accelStep; // Step size, in m/s^2
  string crossCountFile; // Output file for cross-table of counts
  string crossPercentFile; // Output file for cross-table of percentages
  string overviewFile; // Output file for overview train tables
  string detailFile; // Output file for detailed train tables

  string controlFile;
  string pickFileString;
  string summaryFile;
  bool summaryAppendFlag;

  bool verboseTransientMatch;
  bool verboseAlignMatches;
  bool verboseAlignPeaks;
  bool verboseRegressMatch;
  bool verboseRegressMotion;

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

struct SensorData
{
  string name;
  string country;
  string type;
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

struct Cluster
{
  double center;
  double median;
  double sdev;
  double lower;
  double upper;
  unsigned count;
  int tag;

  bool operator < (const Cluster& c2)
  {
    return (median < c2.median);
  }
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

struct Alignment
{
  unsigned trainNo;
  double dist;
  double distMatch;
  unsigned numAdd;
  unsigned numDelete;
  vector<int> actualToRef;

  bool operator < (const Alignment& a2)
  {
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

enum BogeyType
{
  BOGEY_LEFT = 0,
  BOGEY_RIGHT = 1,
  BOGEY_SIZE = 2
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
  PEAK_QUALITY_GREAT = 0,
  PEAK_QUALITY_GOOD = 1,
  PEAK_QUALITY_ACCEPTABLE = 2,
  PEAK_QUALITY_POOR = 3,
  PEAK_QUALITY_SIZE = 4
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

#endif
