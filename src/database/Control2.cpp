#include <iostream>
#include <iomanip>
#include <sstream>

#include "Control2.h"


Control2::Control2()
{
  Control2::reset();
}


Control2::~Control2()
{
}


void Control2::reset()
{
  fields.clear();
  fieldCounts.clear();

  Control2::configure();
}


void Control2::configure()
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
    // Example: ../../../mini_dataset_v012/data/sensors/062493/raw
    { "TRACE_DIRECTORY", CORRESPONDENCE_STRING, CTRL_TRACE_DIRECTORY },
    // Example: ../../../mini_dataset_v012/labels.csv
    { "TRUTH_FILE", CORRESPONDENCE_STRING, CTRL_TRUTH_FILE },
    // Example: output/overview.txt
    { "OVERVIEW_FILE", CORRESPONDENCE_STRING, CTRL_OVERVIEW_FILE },
    // Example: output/details.txt
    { "DETAIL_FILE", CORRESPONDENCE_STRING, CTRL_DETAIL_FILE }
  };

  fieldCounts =
  {
    CTRL_STRINGS_SIZE,
    0,
    0,
    CTRL_INT_VECTORS_SIZE,
    0,
    CTRL_BOOLS_SIZE,
    0
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
    { "-w", "--writing", CORRESPONDENCE_BIT_VECTOR, CTRL_WRITE, "0x20",
      "Binary output files (default: 0x20).  Bits:\n"
      "0x01: transient\n" 
      "0x02: back\n" 
      "0x04: front\n"
      "0x08: speed\n"
      "0x10: pos\n"
      "0x20: peak\n"
      "0x40: outline\n" },
    { "-v", "--verbose", CORRESPONDENCE_BIT_VECTOR, CTRL_VERBOSE, "0x1b",
      "Verbosity (default: 0x1b).  Bits:\n"
      "0x01: Transient match\n"
      "0x02: Align matches\n"
      "0x04: Align peaks\n"
      "0x08: Regress match\n"
      "0x10: Regress motion\n"
      "0x20: Reduce peaks\n" }
  };

  entry.init(fieldCounts);
}


bool Control2::parseCommandLine(
  int argc, 
  char * argv[])
{
  if (! entry.setCommandLineDefaults(commands))
  {
    cout << "Could not parse defaults\n";
    return false;
  }

  const string basename = argv[0];

  if (argc <= 1)
  {
    entry.usage(basename, commands);
    return false;
  }

  vector<string> commandLine;
  for (unsigned i = 1; i < static_cast<unsigned>(argc); i++)
    commandLine.push_back(string(argv[i]));

  if (! entry.parseCommandLine(commandLine, commands))
  {
    entry.usage(basename, commands);
    return false;
  }

  if (! Control2::readFile(entry.getString(CTRL_CONTROL_FILE)))
  {
    entry.usage(basename, commands);
    return false;
  }

  return true;
}


bool Control2::readFile(const string& fname)
{
  if (! entry.readTagFile(fname, fields, fieldCounts))
    return false;

  return true;
}


const string& Control2::carDir() const
{
  return entry.getString(CTRL_CAR_DIRECTORY);
}


const string& Control2::trainDir() const
{
  return entry.getString(CTRL_TRAIN_DIRECTORY);
}


const string& Control2::correctionDir() const
{
  return entry.getString(CTRL_CORRECTION_DIRECTORY);
}


const string& Control2::sensorFile() const
{
  return entry.getString(CTRL_SENSOR_FILE);
}


const string& Control2::sensorCountry() const
{
  return entry.getString(CTRL_SENSOR_COUNTRY);
}


const string& Control2::traceDir() const
{
  return entry.getString(CTRL_TRACE_DIRECTORY);
}


const string& Control2::truthFile() const
{
  return entry.getString(CTRL_TRUTH_FILE);
}


const string& Control2::overviewFile() const
{
  return entry.getString(CTRL_OVERVIEW_FILE);
}


const string& Control2::detailFile() const
{
  return entry.getString(CTRL_DETAIL_FILE);
}


const string& Control2::pickFirst() const
{
  return entry.getString(CTRL_PICK_FIRST);
}


const string& Control2::pickAny() const
{
  return entry.getString(CTRL_PICK_ANY);
}


const string& Control2::statsFile() const
{
  return entry.getString(CTRL_STATS_FILE);
}


bool Control2::writeTransient() const
{
  return entry.getIntVector(CTRL_WRITE)[CTRL_WRITE_TRANSIENT];
}


bool Control2::writeBack() const
{
  return entry.getIntVector(CTRL_WRITE)[CTRL_WRITE_BACK];
}


bool Control2::writeFront() const
{
  return entry.getIntVector(CTRL_WRITE)[CTRL_WRITE_FRONT];
}


bool Control2::writeSpeed() const
{
  return entry.getIntVector(CTRL_WRITE)[CTRL_WRITE_SPEED];
}


bool Control2::writePos() const
{
  return entry.getIntVector(CTRL_WRITE)[CTRL_WRITE_POS];
}


bool Control2::writePeak() const
{
  return entry.getIntVector(CTRL_WRITE)[CTRL_WRITE_PEAK];
}


bool Control2::writeOutline() const
{
  return entry.getIntVector(CTRL_WRITE)[CTRL_WRITE_OUTLINE];
}


bool Control2::verboseTransient() const
{
  return entry.getIntVector(CTRL_VERBOSE)[CTRL_VERBOSE_TRANSIENT_MATCH];
}


bool Control2::verboseAlignMatches() const
{
  return entry.getIntVector(CTRL_VERBOSE)[CTRL_VERBOSE_ALIGN_MATCHES];
}


bool Control2::verboseAlignPeaks() const
{
  return entry.getIntVector(CTRL_VERBOSE)[CTRL_VERBOSE_ALIGN_PEAKS];
}


bool Control2::verboseRegressMatch() const
{
  return entry.getIntVector(CTRL_VERBOSE)[CTRL_VERBOSE_REGRESS_MATCH];
}


bool Control2::verboseRegressMotion() const
{
  return entry.getIntVector(CTRL_VERBOSE)[CTRL_VERBOSE_REGRESS_MOTION];
}


bool Control2::verbosePeakReduce() const
{
  return entry.getIntVector(CTRL_VERBOSE)[CTRL_VERBOSE_PEAK_REDUCE];
}


