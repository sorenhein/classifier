#ifndef TRAIN_STRUCT_H
#define TRAIN_STRUCT_H

#include <string>

using namespace std;


struct Control
{
  string carDir; // Directory of car files
  string trainDir; // Directory of train files
  string country; // Country of input traces
  int year; // Year of input traces
  string disturbFile; // File with data for random disturbance
  int simCount; // Number of simulations for a given train, v and a
  double speedMin; // Lower speed for simulation loop, in m/s
  double speedMax; // Upper speed for simulation loop, in m/s
  double speedStep; // Step size, in m/s
  double accelMin; // Lower acceleration for simulation loop, in m/s^2
  double accelMax; // Upperacceleration for simulation loop, in m/s^2
  double accelStep; // Step size, in m/s^2
  string crossCountFile; // Output file for cross-table of counts
  string crossPercentFile; // Output file for cross-table of percentages
  string overviewFile; // Output file for overview train tables
  string detailFile; // Output file for detailed train tables
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
  FILE_SIZE = 2
};

struct Cluster
{
  double center;
  double median;
  double sdev;
  double lower;
  double upper;
  int count;
  int tag;

  bool operator < (const Cluster& c2)
  {
    return (median < c2.median);
  }
};


#endif
