#ifndef TRAIN_CONTROL2_H
#define TRAIN_CONTROL2_H
// TODO Change guard name back

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
  CTRL_TRACE_DIRECTORY = 5,
  CTRL_TRUTH_FILE = 6,
  CTRL_OVERVIEW_FILE = 7,
  CTRL_DETAIL_FILE = 8,
  CTRL_CONTROL_FILE = 9,
  CTRL_PICK_FIRST = 10,
  CTRL_PICK_ANY = 11,
  CTRL_STATS_FILE = 12,
  CTRL_STRINGS_SIZE = 13
};

enum ControlIntVectors
{
  CTRL_WRITE = 0,
  CTRL_VERBOSE = 1,
  CTRL_INT_VECTORS_SIZE = 2
};

enum ControlBoolStrings
{
  CTRL_APPEND = 0,
  CTRL_BOOLS_SIZE = 1
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


class Control2
{
  private:

    list<CorrespondenceEntry> fields;

    vector<unsigned> fieldCounts;

    list<CommandLineEntry> commands;

    Entity entry;

    void configure();


  public:

    Control2();

    ~Control2();

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
