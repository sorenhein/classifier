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

struct FlankData
{
  float len;
  float range;
  float area;

  void reset() {len = 0; range = 0.f; area = 0.f;}

  void operator += (const FlankData& fd2)
  {
    len += fd2.len; range += fd2.range; area += fd2.area;
  }

  void operator -= (const FlankData& fd2)
  {
    len += fd2.len; range -= fd2.range; area -= fd2.area;
  }

  void operator /= (const unsigned no)
  {
    len /= no;
    range /= no;
    area /= no;
  }
};

struct PeakData
{
  unsigned index;
  float value;
  bool maxFlag;
  FlankData left;
  FlankData right;
  bool activeFlag;

  void reset()
  {
    index = 0;
    value = 0.f;
    maxFlag = false;
    left.reset();
    right.reset();
    activeFlag = false;
  };

  void operator += (const PeakData& pd2)
  {
    value += pd2.value;
    left += pd2.left;
    right += pd2.right;
  }

  void operator /= (const unsigned no)
  {
    if (no > 0)
    {
      value /= no;
      left /= no;
      right /= no;
    }
  }
};

enum PeakType
{
  PEAK_TENTATIVE = 0,
  PEAK_POSSIBLE = 1,
  PEAK_CONCEIVABLE = 2,
  PEAK_REJECTED = 3,
  PEAK_TRANS_FRONT = 4,
  PEAK_TRANS_BACK = 5,
  PEAK_TRUE_TOO_EARLY = 6,
  PEAK_TRUE_TOO_LATE = 7,
  PEAK_TRUE_MISSING = 8,
  PEAK_TYPE_SIZE = 9
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
  PEAK_TRUE2_TOO_EARLY = 0,
  PEAK_TRUE2_MISSED = 1,
  PEAK_TRUE2_TOO_LATE = 2,
  PEAK_TRUE2_SIZE = 3
};

#endif
