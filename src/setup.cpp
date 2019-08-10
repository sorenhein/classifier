#include <iostream>
#include <string>

#include "database/SensorDB.h"
#include "database/CarDB.h"
#include "database/CorrectionDB.h"
#include "database/TrainDB.h"
#include "database/TraceDB.h"
#include "database/Control.h"

#include "stats/CountStats.h"

#include "setup.h"

#include "util/io.h"


using namespace std;

extern CountStats overallStats;
extern CountStats warnStats;
extern CountStats deviationStats;
extern CountStats partialStats;
extern CountStats carMethodStats;
extern CountStats modelCountStats;
extern CountStats alignStats;
extern CountStats exceptStats;


void setupControl(
  int argc,
  char * argv[],
  Control& control);

void setupCars(
  const Control& control,
  CarDB& carDB);

void setupCorrections(
  const Control& control,
  CorrectionDB& correctionDB);

void setupTrains(
  const Control& control,
  const CarDB& carDB,
  const CorrectionDB& correctionDB,
  TrainDB& trainDB);

void setupSensors(
  const Control& control,
  SensorDB& sensorDB);

void setupTraces(
  const Control& control,
  const TrainDB& trainDB,
  const SensorDB& sensorDB,
  TraceDB& traceDB);


void setupControl(
  int argc,
  char * argv[],
  Control& control)
{
  if (! control.parseCommandLine(argc, argv))
  {
    cout << "Bad command line" << endl;
    exit(0);
  }

  if (control.traceDir() == "")
  {
    // This was once use to generate synthetic, noisy peaks and
    // to classify them.
    cout << "No traces specified.\n";
    exit(0);
  }
}


void setupCars(
  const Control& control,
  CarDB& carDB)
{
  vector<string> textfiles;
  if (! getFilenames(control.carDir(), textfiles))
  {
    cout << "Bad directory " << control.carDir() << endl;
    exit(0);
  }

  for (auto& fname: textfiles)
    carDB.readFile(fname);
}


void setupCorrections(
  const Control& control,
  CorrectionDB& correctionDB)
{
  vector<string> correctionFiles;
  if (! getFilenames(control.correctionDir(), correctionFiles))
  {
    cout << "Bad directory " << control.correctionDir() << endl;
    exit(0);
  }

  for (auto& fname: correctionFiles)
    correctionDB.readFile(fname);
}


void setupTrains(
  const Control& control,
  const CarDB& carDB,
  const CorrectionDB& correctionDB,
  TrainDB& trainDB)
{
  vector<string> textfiles;
  textfiles.clear();
  if (! getFilenames(control.trainDir(), textfiles))
  {
    cout << "Bad directory " << control.trainDir() << endl;
    exit(0);
  }

  for (auto& fname: textfiles)
    trainDB.readFile(carDB, correctionDB, fname);

  if (! trainDB.selectByAxles({"ALL"}, 0, 100))
  {
    cout << "No trains selected" << endl;
    exit(0);
  }
}


void setupSensors(
  const Control& control,
  SensorDB& sensorDB)
{
  if (control.sensorFile() != "")
    sensorDB.readFile(control.sensorFile());
}


void setupTraces(
  const Control& control,
  const TrainDB& trainDB,
  const SensorDB& sensorDB,
  TraceDB& traceDB)
{
  traceDB.readFile(control.truthFile(), trainDB, sensorDB);

  vector<string> filenames;
  if (! getFilenames(control.traceDir(), filenames, control.pickFirst()))
  {
    cout << "Bad directory " << control.traceDir() << endl;
    exit(0);
  }

  traceDB.pickFilenames(filenames, control.pickAny());
}


void setup(
  int argc, 
  char * argv[],
  Control& control,
  TrainDB& trainDB,
  TraceDB& traceDB)
{
  setupControl(argc, argv, control);

  CarDB carDB;
  setupCars(control, carDB);

  CorrectionDB correctionDB;
  setupCorrections(control, correctionDB);

  setupTrains(control, carDB, correctionDB, trainDB);

  SensorDB sensorDB;
  setupSensors(control, sensorDB);

  setupTraces(control, trainDB, sensorDB, traceDB);
}


void setupStats()
{
  overallStats.init("Overall",
    {
      "count",
      "good",
      "error",
      "except",
      "warn"
    } );

  warnStats.init("Warnings",
    {
      "first, 1-4",
      "first, 5+",
      "mid, 1-4",
      "mid, 5+",
      "last, 1-4",
      "last, 5+"
    } );

  deviationStats.init("Deviations",
    {
      "<= 1",
      "1-3",
      "3-10",
      "10-100",
      "100+"
    } );

  partialStats.init("Partials",
    {
      "Trains with warnings",
      "Trains with partials",
      "Cars with partials",
      "Partial peak count"
    } );

  modelCountStats.init("Model counts",
    {
      "0",
      "1",
      "2",
      "3",
      "4",
      "5+"
    } );

  carMethodStats.init("Recognizers",
    { 
      "by 1234 order",
      "by great quality",
      "by emptiness",
      "by pattern",
      "by spacing"
    } );

  alignStats.init("Alignment",
    {
      "match",
      "miss early",
      "miss within",
      "miss late",
      "spurious early",
      "spurious within",
      "spurious late"
    } );

  exceptStats.init("Exceptions",
    {
      "Unknown sample rate",
      "Trace file not read",
      "Truth train not known",
      "No alignment matches",
      "No peaks in structure",
      "No nested intervals",
      "No peak scale found when labeling",
      "Short gap by new method empty",
      "New short gap fails",
      "Long gap is very long",
      "Three-wheeler",
      "Not all cars detected"
    } );
}

