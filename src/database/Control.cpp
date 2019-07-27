#include <iostream>
#include <iomanip>
#include <sstream>

#if defined(__CYGWIN__) && (__GNUC__ < 8)
  #include <experimental/filesystem>
  using namespace std::experimental::filesystem;
#else
  #include <filesystem>
  using namespace std::filesystem;
#endif


#include "Control.h"

// Undocumented expansion on my system.
#define CONTROL_EXPANSION "../data/control/sensors/sensor"


Control::Control()
{
  Control::reset();
}


Control::~Control()
{
}


void Control::reset()
{
  fields.clear();
  fieldCounts.clear();
  completions.clear();

  Control::configure();
}


void Control::configure()
{
  fields =
  {
    // Example: ../data/cars
    { "CAR_DIRECTORY", CORRESPONDENCE_STRING, CTRL_CAR_DIRECTORY },
    // Example: ../data/trains
    { "TRAIN_DIRECTORY", CORRESPONDENCE_STRING, CTRL_TRAIN_DIRECTORY },
    // Example: ../data/residuals
    { "CORRECTION_DIRECTORY", CORRESPONDENCE_STRING, 
      CTRL_CORRECTION_DIRECTORY },
    // Example: ../../../mini_dataset_v012/sensors.txt
    { "SENSOR_FILE", CORRESPONDENCE_STRING, CTRL_SENSOR_FILE },
    // Example: DEU
    { "COUNTRY", CORRESPONDENCE_STRING, CTRL_SENSOR_COUNTRY },

    // Example: ../../../mini_dataset_v012/data/sensors/062493
    { "BASE_DIRECTORY", CORRESPONDENCE_STRING, CTRL_BASE_DIRECTORY },
    // Example: raw
    { "TRACE_SUBDIR", CORRESPONDENCE_STRING, CTRL_TRACE_SUBDIR },
    // Example: transient
    { "TRANSIENT_SUBDIR", CORRESPONDENCE_STRING, CTRL_TRANSIENT_SUBDIR },
    // Example: back
    { "BACK_SUBDIR", CORRESPONDENCE_STRING, CTRL_BACK_SUBDIR },
    // Example: front
    { "FRONT_SUBDIR", CORRESPONDENCE_STRING, CTRL_FRONT_SUBDIR },
    // Example: speed
    { "SPEED_SUBDIR", CORRESPONDENCE_STRING, CTRL_SPEED_SUBDIR },
    // Example: pos
    { "POS_SUBDIR", CORRESPONDENCE_STRING, CTRL_POS_SUBDIR },
    // Example: peak
    { "PEAK_SUBDIR", CORRESPONDENCE_STRING, CTRL_PEAK_SUBDIR },

    // Example: ../../../mini_dataset_v012/labels.csv
    { "TRUTH_FILE", CORRESPONDENCE_STRING, CTRL_TRUTH_FILE },

    // Example: output
    { "OUTPUT_DIRECTORY", CORRESPONDENCE_STRING, CTRL_OUTPUT_DIRECTORY },
    // Example: sensorstats.txt (goes within output)
    { "SENSORSTATS_FILE", CORRESPONDENCE_STRING, CTRL_SENSORSTATS_FILE },
    // Example: trainstats.txt (goes within output)
    { "TRAINSTATS_FILE", CORRESPONDENCE_STRING, CTRL_TRAINSTATS_FILE },
    // Example: peakstats.txt (goes within output)
    { "PEAKSTATS_FILE", CORRESPONDENCE_STRING, CTRL_PEAKSTATS_FILE }
  };

  fieldCounts =
  {
    CTRL_STRINGS_SIZE,
    0,
    0,
    CTRL_INT_VECTORS_SIZE,
    CTRL_INTS_SIZE,
    CTRL_BOOLS_SIZE,
    0
  };

  completions =
  {
    { CTRL_WRITE_TRANSIENT, CTRL_BASE_DIRECTORY, 
      CTRL_TRANSIENT_SUBDIR, CTRL_TRANSIENT_DIRECTORY },

    { CTRL_WRITE_BACK, CTRL_BASE_DIRECTORY, 
      CTRL_BACK_SUBDIR, CTRL_BACK_DIRECTORY },

    { CTRL_WRITE_FRONT, CTRL_BASE_DIRECTORY, 
      CTRL_FRONT_SUBDIR, CTRL_FRONT_DIRECTORY },

    { CTRL_WRITE_SPEED, CTRL_BASE_DIRECTORY, 
      CTRL_SPEED_SUBDIR, CTRL_SPEED_DIRECTORY },

    { CTRL_WRITE_POS, CTRL_BASE_DIRECTORY, 
      CTRL_POS_SUBDIR, CTRL_POS_DIRECTORY },

    { CTRL_WRITE_PEAK, CTRL_BASE_DIRECTORY, 
      CTRL_PEAK_SUBDIR, CTRL_PEAK_DIRECTORY }
  };

  commands =
  {
    { "-c", "--control", CORRESPONDENCE_STRING, CTRL_CONTROL_FILE, "", 
      "Input control file." },
    { "-p", "--pick", CORRESPONDENCE_STRING, CTRL_PICK_FIRST, "", 
      "If set, pick the first file among those set in\n"
      "the input control file that contains 's'." },
    { "-x", "--train", CORRESPONDENCE_STRING, CTRL_PICK_ANY, "",
      "If set, pick reference trains containing s." },
    { "-s", "--stats", CORRESPONDENCE_STRING, CTRL_STATS_FILE, "",
      "Stats output file." },
    { "-a", "--append", CORRESPONDENCE_BOOL, CTRL_APPEND, "no",
      "If present, stats file is not rewritten." },
    { "-t", "--threads", CORRESPONDENCE_INT, CTRL_THREADS, "1",
      "Number of threads (default: 1)." },
    { "-w", "--writing", CORRESPONDENCE_BIT_VECTOR, CTRL_WRITE, "0x20",
      "Binary output files (default: 0x20).  Bits:\n"
      "0x01: transient\n" 
      "0x02: back\n" 
      "0x04: front\n"
      "0x08: speed\n"
      "0x10: pos\n"
      "0x20: peak\n"
      "0x40: outline" },
    { "-v", "--verbose", CORRESPONDENCE_BIT_VECTOR, CTRL_VERBOSE, "0x1b",
      "Verbosity (default: 0x1b).  Bits:\n"
      "0x01: Transient match\n"
      "0x02: Align matches\n"
      "0x04: Align peaks\n"
      "0x08: Regress match\n"
      "0x10: Regress motion\n"
      "0x20: Reduce peaks" }
  };

  entry.init(fieldCounts);
}


bool Control::parseCommandLine(
  int argc, 
  char * argv[])
{
  if (! entry.setCommandLineDefaults(commands))
  {
    cout << "Could not parse defaults\n";
    return false;
  }

  const string fullname = argv[0];
  // filesystem::path pathObj(fullname);
  path pathObj(fullname);
  const string basename = pathObj.filename().string();

  if (argc <= 1)
  {
    cout << entry.usage(basename, commands);
    return false;
  }

  vector<string> commandLine;
  for (unsigned i = 1; i < static_cast<unsigned>(argc); i++)
    commandLine.push_back(string(argv[i]));

  if (! entry.parseCommandLine(commandLine, commands))
  {
    cout << entry.usage(basename, commands);
    return false;
  }

  // Undocumented expansion of control file argument.
  const string cfile = entry.getString(CTRL_CONTROL_FILE);
  if (cfile.size() == 2)
  {
    entry.getString(CTRL_CONTROL_FILE) = CONTROL_EXPANSION +
      cfile + ".txt";
  }

  if (! Control::readFile(entry.getString(CTRL_CONTROL_FILE)))
  {
    cout << entry.usage(basename, commands);
    return false;
  }

  return true;
}


void Control::complete()
{
  entry.setString(CTRL_TRACE_DIRECTORY,
    entry.getString(CTRL_BASE_DIRECTORY) + "/" +
    entry.getString(CTRL_TRACE_SUBDIR));

  for (auto& comp: completions)
  {
    if (entry.getIntVector(CTRL_WRITE)[comp.flag])
      entry.setString(comp.result,
        entry.getString(comp.base) + "/" + entry.getString(comp.tail));
  }

  entry.setString(CTRL_SENSORSTATS_FILEFULL,
    entry.getString(CTRL_OUTPUT_DIRECTORY) + "/" +
    entry.getString(CTRL_SENSORSTATS_FILE));

  entry.setString(CTRL_TRAINSTATS_FILEFULL,
    entry.getString(CTRL_OUTPUT_DIRECTORY) + "/" +
    entry.getString(CTRL_TRAINSTATS_FILE));

  entry.setString(CTRL_PEAKSTATS_FILEFULL,
    entry.getString(CTRL_OUTPUT_DIRECTORY) + "/" +
    entry.getString(CTRL_PEAKSTATS_FILE));
}


bool Control::readFile(const string& fname)
{
  if (! entry.readTagFile(fname, fields, fieldCounts))
    return false;

  Control::complete();
  return true;
}


const string& Control::carDir() const
{
  return entry.getString(CTRL_CAR_DIRECTORY);
}


const string& Control::trainDir() const
{
  return entry.getString(CTRL_TRAIN_DIRECTORY);
}


const string& Control::correctionDir() const
{
  return entry.getString(CTRL_CORRECTION_DIRECTORY);
}


const string& Control::sensorFile() const
{
  return entry.getString(CTRL_SENSOR_FILE);
}


const string& Control::sensorCountry() const
{
  return entry.getString(CTRL_SENSOR_COUNTRY);
}


const string& Control::baseDir() const
{
  return entry.getString(CTRL_BASE_DIRECTORY);
}


const string& Control::traceDir() const
{
  return entry.getString(CTRL_TRACE_DIRECTORY);
}


const string& Control::transientDir() const
{
  return entry.getString(CTRL_TRANSIENT_DIRECTORY);
}


const string& Control::backDir() const
{
  return entry.getString(CTRL_BACK_DIRECTORY);
}


const string& Control::frontDir() const
{
  return entry.getString(CTRL_FRONT_DIRECTORY);
}


const string& Control::speedDir() const
{
  return entry.getString(CTRL_SPEED_DIRECTORY);
}


const string& Control::posDir() const
{
  return entry.getString(CTRL_POS_DIRECTORY);
}


const string& Control::peakDir() const
{
  return entry.getString(CTRL_PEAK_DIRECTORY);
}


const string& Control::truthFile() const
{
  return entry.getString(CTRL_TRUTH_FILE);
}


const string& Control::sensorstatsFile() const
{
  return entry.getString(CTRL_SENSORSTATS_FILEFULL);
}


const string& Control::trainstatsFile() const
{
  return entry.getString(CTRL_TRAINSTATS_FILEFULL);
}


const string& Control::peakstatsFile() const
{
  return entry.getString(CTRL_PEAKSTATS_FILEFULL);
}


const string& Control::pickFirst() const
{
  return entry.getString(CTRL_PICK_FIRST);
}


const string& Control::pickAny() const
{
  return entry.getString(CTRL_PICK_ANY);
}


const string& Control::statsFile() const
{
  return entry.getString(CTRL_STATS_FILE);
}


unsigned Control::numThreads() const
{
  return static_cast<unsigned>(entry[CTRL_THREADS]);
}


bool Control::writeTransient() const
{
  return (entry.getIntVector(CTRL_WRITE)[CTRL_WRITE_TRANSIENT] != 0);
}


bool Control::writeBack() const
{
  return (entry.getIntVector(CTRL_WRITE)[CTRL_WRITE_BACK] != 0);
}


bool Control::writeFront() const
{
  return (entry.getIntVector(CTRL_WRITE)[CTRL_WRITE_FRONT] != 0);
}


bool Control::writeSpeed() const
{
  return (entry.getIntVector(CTRL_WRITE)[CTRL_WRITE_SPEED] != 0);
}


bool Control::writePos() const
{
  return (entry.getIntVector(CTRL_WRITE)[CTRL_WRITE_POS] != 0);
}


bool Control::writePeak() const
{
  return (entry.getIntVector(CTRL_WRITE)[CTRL_WRITE_PEAK] != 0);
}


bool Control::verboseTransient() const
{
  return (entry.getIntVector(CTRL_VERBOSE)[CTRL_VERBOSE_TRANSIENT_MATCH] != 0);
}


bool Control::verboseAlignMatches() const
{
  return (entry.getIntVector(CTRL_VERBOSE)[CTRL_VERBOSE_ALIGN_MATCHES] != 0);
}


bool Control::verboseAlignPeaks() const
{
  return (entry.getIntVector(CTRL_VERBOSE)[CTRL_VERBOSE_ALIGN_PEAKS] != 0);
}


bool Control::verboseRegressMatch() const
{
  return (entry.getIntVector(CTRL_VERBOSE)[CTRL_VERBOSE_REGRESS_MATCH] != 0);
}


bool Control::verboseRegressMotion() const
{
  return (entry.getIntVector(CTRL_VERBOSE)[CTRL_VERBOSE_REGRESS_MOTION] != 0);
}


bool Control::verbosePeakReduce() const
{
  return (entry.getIntVector(CTRL_VERBOSE)[CTRL_VERBOSE_PEAK_REDUCE] != 0);
}


