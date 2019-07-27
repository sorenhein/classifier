#ifndef TRAIN_CONTROL_H
#define TRAIN_CONTROL_H

#include <map>

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
  CTRL_OVERVIEW_FILE = 21,
  CTRL_DETAIL_FILE = 22,
  CTRL_CONTROL_FILE = 23,
  CTRL_PICK_FIRST = 24,
  CTRL_PICK_ANY = 25,
  CTRL_STATS_FILE = 26,
  CTRL_STRINGS_SIZE = 27
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
  CTRL_WRITE_OUTLINE = 6
};

enum ControlVerbose
{
  CTRL_VERBOSE_TRANSIENT_MATCH = 0,
  CTRL_VERBOSE_ALIGN_MATCHES = 1,
  CTRL_VERBOSE_ALIGN_PEAKS = 2,
  CTRL_VERBOSE_REGRESS_MATCH = 3,
  CTRL_VERBOSE_REGRESS_MOTION = 4,
  CTRL_VERBOSE_PEAK_REDUCE = 5
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
    const string& traceDir() const;
    const string& truthFile() const;
    const string& overviewFile() const;
    const string& detailFile() const;
    const string& pickFirst() const;
    const string& pickAny() const;
    const string& statsFile() const;

    bool writeTransient() const;
    bool writeBack() const;
    bool writeFront() const;
    bool writeSpeed() const;
    bool writePos() const;
    bool writePeak() const;
    bool writeOutline() const;

    bool verboseTransient() const;
    bool verboseAlignMatches() const;
    bool verboseAlignPeaks() const;
    bool verboseRegressMatch() const;
    bool verboseRegressMotion() const;
    bool verbosePeakReduce() const;
};

#endif
