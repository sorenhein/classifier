#ifndef TRAIN_CONTROL_H
#define TRAIN_CONTROL_H

#include <map>
#include <list>
#include <vector>
#include <string>

#include "Entity.h"

using namespace std;


enum ControlFieldStrings
{
  CTRL_CAR_DIRECTORY = 0,
  CTRL_TRAIN_DIRECTORY = 1,
  CTRL_CORRECTION_DIRECTORY = 2,
  CTRL_SENSOR_FILE = 3,
  CTRL_SENSOR_COUNTRY = 4,

  CTRL_BASE_DIRECTORY = 5,
  CTRL_TRACE_SUBDIR = 6,
  CTRL_TRANSIENT_SUBDIR = 7,
  CTRL_BACK_SUBDIR = 8,
  CTRL_FRONT_SUBDIR = 9,
  CTRL_SPEED_SUBDIR = 10,
  CTRL_POS_SUBDIR = 11,
  CTRL_PEAK_SUBDIR = 12,

  CTRL_TRACE_DIRECTORY = 13,
  CTRL_TRANSIENT_DIRECTORY = 14,
  CTRL_BACK_DIRECTORY = 15,
  CTRL_FRONT_DIRECTORY = 16,
  CTRL_SPEED_DIRECTORY = 17,
  CTRL_POS_DIRECTORY = 18,
  CTRL_PEAK_DIRECTORY = 19,

  CTRL_TRUTH_FILE = 20,

  CTRL_OUTPUT_DIRECTORY = 21,
  CTRL_SENSORSTATS_FILE = 22,
  CTRL_TRAINSTATS_FILE = 23,
  CTRL_PEAKSTATS_FILE = 24,
  CTRL_SENSORSTATS_FILEFULL = 25,
  CTRL_TRAINSTATS_FILEFULL = 26,
  CTRL_PEAKSTATS_FILEFULL = 27,

  CTRL_CONTROL_FILE = 28,
  CTRL_PICK_FIRST = 29,
  CTRL_PICK_ANY = 30,
  CTRL_STATS_FILE = 31,
  CTRL_STRINGS_SIZE = 32
};

enum ControlIntVectors
{
  CTRL_WRITE = 0,
  CTRL_VERBOSE = 1,
  CTRL_INT_VECTORS_SIZE = 2
};

enum ControlBools
{
  CTRL_APPEND = 0,
  CTRL_BOOLS_SIZE = 1
};

enum ControlInts
{
  CTRL_THREADS = 0,
  CTRL_INTS_SIZE = 1
};

enum ControlWrite
{
  CTRL_WRITE_TRANSIENT = 0,
  CTRL_WRITE_BACK = 1,
  CTRL_WRITE_FRONT = 2,
  CTRL_WRITE_SPEED = 3,
  CTRL_WRITE_POS = 4,
  CTRL_WRITE_PEAK = 5
};

enum ControlVerbose
{
  CTRL_VERBOSE_TRANSIENT_MATCH = 0,
  CTRL_VERBOSE_QUIET = 1,
  CTRL_VERBOSE_ALIGN_MATCHES = 2,
  CTRL_VERBOSE_ALIGN_PEAKS = 3,
  CTRL_VERBOSE_REGRESS_MATCH = 4,
  CTRL_VERBOSE_REGRESS_MOTION = 5,
  CTRL_VERBOSE_REGRESS_TOPS = 6,
  CTRL_VERBOSE_PEAK_REDUCE = 7,
  CTRL_VERBOSE_SENSOR_STATS = 8,
  CTRL_VERBOSE_CROSS_STATS = 9,
  CTRL_VERBOSE_TRAIN_STATS = 10,
  CTRL_VERBOSE_TIMER_STATS = 11

};


class Control
{
  private:

    struct Completion
    {
      ControlWrite flag;
      ControlFieldStrings base;
      ControlFieldStrings tail;
      ControlFieldStrings result;
    };


    list<CorrespondenceEntry> fields;

    vector<unsigned> fieldCounts;

    list<CommandLineEntry> commands;

    Entity entry;

    list<Completion> completions;


    void configure();

    void complete();


  public:

    Control();

    ~Control();

    void reset();

    bool parseCommandLine(
      int argc,
      char * argv[]);

    bool readFile(const string& fname);

    const string& carDir() const;
    const string& trainDir() const;
    const string& correctionDir() const;
    const string& sensorFile() const;
    const string& sensorCountry() const;

    const string& baseDir() const;
    const string& traceDir() const;
    const string& transientDir() const;
    const string& backDir() const;
    const string& frontDir() const;
    const string& speedDir() const;
    const string& posDir() const;
    const string& peakDir() const;

    const string& truthFile() const;

    const string& sensorstatsFile() const;
    const string& trainstatsFile() const;
    const string& peakstatsFile() const;

    const string& pickFirst() const;
    const string& pickAny() const;
    const string& statsFile() const;

    unsigned numThreads() const;

    bool writeTransient() const;
    bool writeBack() const;
    bool writeFront() const;
    bool writeSpeed() const;
    bool writePos() const;
    bool writePeak() const;

    bool verboseTransient() const;
    bool verboseQuiet() const;
    bool verboseAlignMatches() const;
    bool verboseAlignPeaks() const;
    bool verboseRegressMatch() const;
    bool verboseRegressMotion() const;
    bool verboseRegressTopResiduals() const;
    bool verbosePeakReduce() const;
    bool verboseSensorStats() const;
    bool verboseCrossStats() const;
    bool verboseTrainStats() const;
    bool verboseTimerStats() const;
};

#endif
