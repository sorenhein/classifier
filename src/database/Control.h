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
  CTRL_SENSOR_FILE = 2,
  CTRL_SENSOR_COUNTRY = 3,

  CTRL_BASE_DIRECTORY = 4,
  CTRL_TRACE_SUBDIR = 5,
  CTRL_TRANSIENT_SUBDIR = 6,
  CTRL_BACK_SUBDIR = 7,
  CTRL_FRONT_SUBDIR = 8,
  CTRL_SPEED_SUBDIR = 9,
  CTRL_POS_SUBDIR = 10,
  CTRL_PEAK_SUBDIR = 11,
  CTRL_MATCH_SUBDIR = 12,
  CTRL_BOX_SUBDIR = 13,

  CTRL_BEST_LEAFDIR = 14,
  CTRL_TRUTH_LEAFDIR = 15,
  CTRL_SECOND_LEAFDIR = 16,

  CTRL_TIMES_NAME = 17,
  CTRL_CARS_NAME = 18,
  CTRL_VALUES_NAME = 19,
  CTRL_REFGRADE_NAME = 20,
  CTRL_PEAKGRADE_NAME = 21,
  CTRL_INFO_NAME = 22,

  CTRL_TRACE_DIRECTORY = 23,
  CTRL_TRANSIENT_DIRECTORY = 24,
  CTRL_BACK_DIRECTORY = 25,
  CTRL_FRONT_DIRECTORY = 26,
  CTRL_SPEED_DIRECTORY = 27,
  CTRL_POS_DIRECTORY = 28,
  CTRL_PEAK_DIRECTORY = 29,
  CTRL_MATCH_DIRECTORY = 30,
  CTRL_BOX_DIRECTORY = 31,

  CTRL_TRUTH_FILE = 32,
  CTRL_EQUIV_FILE = 33,
  CTRL_CORRECTION_FILE = 34,

  CTRL_OUTPUT_DIRECTORY = 35,

  CTRL_CONTROL_FILE = 36,
  CTRL_PICK_FIRST = 37,
  CTRL_PICK_ANY = 38,
  CTRL_STATS_FILE = 39,
  CTRL_STRINGS_SIZE = 40
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
  CTRL_WRITE_PEAK = 5,
  CTRL_WRITE_MATCH = 6,
  CTRL_WRITE_BOX = 7
};

enum ControlVerbose
{
  CTRL_VERBOSE_TRANSIENT_MATCH = 0,
  CTRL_VERBOSE_QUIET = 1,
  CTRL_VERBOSE_ALIGN_MATCHES = 2,
  CTRL_VERBOSE_REGRESS_MATCH = 3,
  CTRL_VERBOSE_REGRESS_MOTION = 4,
  CTRL_VERBOSE_REGRESS_TOPS = 5,
  CTRL_VERBOSE_PEAK_REDUCE = 6,
  CTRL_VERBOSE_SENSOR_STATS = 7,
  CTRL_VERBOSE_CROSS_STATS = 8,
  CTRL_VERBOSE_TRAIN_STATS = 9,
  CTRL_VERBOSE_TIMER_STATS = 10

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
    const string& matchDir() const;
    const string& boxDir() const;

    const string& bestLeafDir() const;
    const string& truthLeafDir() const;
    const string& secondLeafDir() const;

    const string& timesName() const;
    const string& carsName() const;
    const string& valuesName() const;
    const string& refGradeName() const;
    const string& peakGradeName() const;
    const string& infoName() const;

    const string& truthFile() const;
    const string& equivFile() const;
    const string& correctionFile() const;

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
    bool writeMatch() const;
    bool writeBox() const;

    bool verboseTransient() const;
    bool verboseQuiet() const;
    bool verboseAlignMatches() const;
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
