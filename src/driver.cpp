#include <iostream>
#include <iomanip>
#include <string>
#include <limits>

#include "args.h"
#include "read.h"
#include "Trace.h"

#include "database/SensorDB.h"
#include "database/CarDB.h"
#include "database/CorrectionDB.h"
#include "database/TrainDB.h"
#include "database/TraceDB.h"
#include "database/Control2.h"

#include "Regress.h"
#include "Align.h"
#include "CompStats.h"
#include "PeakStats.h"
#include "Except.h"
#include "geometry.h"
#include "print.h"

#include "util/Timers.h"
#include "util/misc.h"

using namespace std;

Log logger;
Timers timers;


void setup(
  int argc, 
  char * argv[],
  Control& control,
  Control2& control2,
  SensorDB& sensorDB,
  CarDB& carDB,
  TrainDB& trainDB);

unsigned lookupMatchRank(
  const TrainDB& trainDB,
  const vector<Alignment>& matches,
  const string& tag);


int main(int argc, char * argv[])
{
  Control control;
  Control2 control2;

  SensorDB sensorDB;
  CarDB carDB;
  TrainDB trainDB;

  setup(argc, argv, control, control2, sensorDB, carDB, trainDB);

  if (control2.traceDir() == "")
  {
    // This was once use to generate synthetic, noisy peaks and
    // to classify them.
    cout << "No traces specified.\n";
    exit(0);
  }

  // This extracts peaks and then extracts train types from peaks.
  TraceDB traceDB;
  traceDB.readFile(control2.truthFile(), sensorDB);

  vector<string> datfiles;
  getFilenames(control2.traceDir(), datfiles, control2.pickFirst());

  CompStats sensorStats, trainStats;
  PeakStats peakStats;



  Imperfections imperf;
  Align align;
  Regress regress;

  vector<PeakPos> perfectPositions;
  vector<Alignment> matchesAlign;
  Alignment bestAlign;
  
  const unsigned order = 2;
  vector<double> motionActual(order+1);
  vector<double> motionEstimate(order+1);
  motionActual[0] = 0.; // Offset in m

  Trace trace;
  vector<PeakTime> times;
  vector<int> actualToRef;
  unsigned numFrontWheels;

  for (auto& fname: datfiles)
  {
    const unsigned t2no = traceDB.traceNumber(fname);
    const string sensor = traceDB.sensor(t2no);
    const string trainTrue = traceDB.train(t2no);

    const string country = sensorDB.country(sensor);

    if (! control2.pickAny().empty() &&
        ! nameMatch(trainTrue, control2.pickAny()))
      continue;

    cout << "File " << fname << ":\n\n";

    // This is only used for diagnostics in trace.
    const double speedTrue = traceDB.speed(t2no);
    const int trainNoTrue = trainDB.lookupNumber(trainTrue);
    vector<double> posTrue;
      
    try
    {
      if (trainNoTrue == -1)
        THROW(ERR_NO_PEAKS, "True train not known");

      trainDB.getPeakPositions(static_cast<unsigned>(trainNoTrue), posTrue);

      // Refuse trace if sample rate is not 2000, maybe in SegActive

      trace.read(fname);
      trace.detect(control, control2, traceDB.sampleRate(t2no), imperf);
      trace.logPeakStats(posTrue, trainTrue, speedTrue, peakStats);
      trace.write(control);

      bool fullTrainFlag;
      if (trace.getAlignment(times, actualToRef, numFrontWheels) &&
          ! actualToRef.empty())
      {
        cout << "FULLALIGN\n";
        fullTrainFlag = true;
      }
      else
      {
        trace.getTrace(times, numFrontWheels);
        fullTrainFlag = false;
      }

      // TODO Kludge.  Should be in PeakDetect or so.
      if (imperf.numSkipsOfSeen > 0)
        numFrontWheels = 4 - imperf.numSkipsOfSeen;

      align.bestMatches(times, actualToRef, numFrontWheels, fullTrainFlag,
        imperf, trainDB, country, 10, control, matchesAlign);

      if (matchesAlign.size() == 0)
      {
        sensorStats.log(sensor, 10, 1000.);
        trainStats.log(trainTrue, 10, 1000.);
        continue;
      }

      regress.bestMatch(times, trainDB, order, control, matchesAlign,
        bestAlign, motionEstimate);

      if (! control2.pickAny().empty())
      {
        const string s = sensor + "/" + traceDB.time(t2no);
        dumpResiduals(times, trainDB, order, matchesAlign, s, trainTrue, 
          control2.pickAny(), 
          trainDB.numAxles(static_cast<unsigned>(trainNoTrue)));
      }

      const string trainDetected = trainDB.lookupName(bestAlign.trainNo);
      const unsigned rank = lookupMatchRank(trainDB, matchesAlign, 
        trainTrue);

      sensorStats.log(sensor, rank, bestAlign.distMatch);
      trainStats.log(trainTrue, rank, bestAlign.distMatch);

if (trainDetected != trainTrue)
  cout << "DRIVER MISMATCH\n";

    }
    catch (Except& ex)
    {
      ex.print(cout);
      sensorStats.log(sensor, 10, 1000.);
      trainStats.log(trainTrue, 10, 1000.);
    }
    catch(...)
    {
      cout << "Unknown exception" << endl;
    }
  }

  sensorStats.print("sensorstats.txt", "Sensor");
  trainStats.print("trainstats.txt", "Train");
  peakStats.print("peakstats.txt");

  cout << timers.str(2) << endl;
}


void setup(
  int argc, 
  char * argv[],
  Control& control,
  Control2& control2,
  SensorDB& sensorDB,
  CarDB& carDB,
  TrainDB& trainDB)
{
  readArgs(argc, argv, control);

  if (! readControlFile(control, control.controlFile))
  {
    cout << "Bad control file" << control.controlFile << endl;
    exit(0);
  }

  if (! control2.parseCommandLine(argc, argv))
  {
    cout << "Bad command line" << endl;
    exit(0);
  }

if (control.writingTransient != control2.writeTransient())
  cout << "WRIT1\n";
if (control.writingBack != control2.writeBack())
  cout << "WRIT2\n";
if (control.writingFront != control2.writeFront())
  cout << "WRIT3\n";
if (control.writingSpeed != control2.writeSpeed())
  cout << "WRIT4\n";
if (control.writingPos != control2.writePos())
  cout << "WRIT5\n";
if (control.writingPeak != control2.writePeak())
  cout << "WRIT6\n";
if (control.writingOutline != control2.writeOutline())
  cout << "WRIT7\n";

if (control.verboseTransient != control2.verboseTransient())
  cout << "VERB1\n";
if (control.verboseAlignMatches != control2.verboseAlignMatches())
  cout << "VERB2\n";
if (control.verboseAlignPeaks != control2.verboseAlignPeaks())
  cout << "VERB3\n";
if (control.verboseRegressMatch != control2.verboseRegressMatch())
  cout << "VERB4\n";
if (control.verboseRegressMotion != control2.verboseRegressMotion())
  cout << "VERB5\n";
if (control.verbosePeakReduce != control2.verbosePeakReduce())
  cout << "VERB6\n";

  // carDB
  vector<string> textfiles;
  getFilenames(control2.carDir(), textfiles);
  for (auto& fname: textfiles)
    carDB.readFile(fname);

  // correctionDB
  CorrectionDB correctionDB;
  vector<string> correctionFiles;
  getFilenames(control2.correctionDir(), correctionFiles);
  for (auto& fname: correctionFiles)
    correctionDB.readFile(fname);

  // trainDB
  textfiles.clear();
  getFilenames(control2.trainDir(), textfiles);
  for (auto& fname: textfiles)
    trainDB.readFile(carDB, correctionDB, fname);

  // sensorDB
  if (control2.sensorFile() != "")
    sensorDB.readFile(control2.sensorFile());

  if (! trainDB.selectByAxles({"ALL"}, 0, 100))
  {
    cout << "No trains selected" << endl;
    exit(0);
  }
}


unsigned lookupMatchRank(
  const TrainDB& trainDB,
  const vector<Alignment>& matches,
  const string& tag)
{
  const unsigned tno = static_cast<unsigned>(trainDB.lookupNumber(tag));
  for (unsigned i = 0; i < matches.size(); i++)
  {
    if (matches[i].trainNo == tno)
      return i;
  }
  return numeric_limits<unsigned>::max();
}

