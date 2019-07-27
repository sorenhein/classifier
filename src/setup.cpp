#include <iostream>
#include <string>

#include "database/SensorDB.h"
#include "database/CarDB.h"
#include "database/CorrectionDB.h"
#include "database/TrainDB.h"
#include "database/TraceDB.h"
#include "database/Control.h"

#include "util/io.h"


using namespace std;


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
  const SensorDB& sensorDB,
  TraceDB& traceDB)
{
  traceDB.readFile(control.truthFile(), sensorDB);

  auto& datfiles = traceDB.getFilenames();
  if (! getFilenames(control.traceDir(), datfiles, control.pickFirst()))
  {
    cout << "Bad directory " << control.traceDir() << endl;
    exit(0);
  }
}


void setup(
  int argc, 
  char * argv[],
  Control& control,
  SensorDB& sensorDB,
  TrainDB& trainDB,
  TraceDB& traceDB)
{
  setupControl(argc, argv, control);

  CarDB carDB;
  setupCars(control, carDB);

  CorrectionDB correctionDB;
  setupCorrections(control, correctionDB);

  setupTrains(control, carDB, correctionDB, trainDB);

  setupSensors(control, sensorDB);

  setupTraces(control, sensorDB, traceDB);
}
