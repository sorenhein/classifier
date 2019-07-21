#ifndef TRAIN_TRACEDB_H
#define TRAIN_TRACEDB_H

#include <vector>
#include <string>
#include <map>

#include "struct.h"

using namespace std;

class SensorDB;
class TrainDB;


class TraceDB
{
  private:

    struct TraceEntry
    {
      string date;
      string time;
      string sensor;
      string country;
      TrainData trainTruth;
      bool hasData;
      unsigned numPeaksDetected;
      vector<Alignment> align;
    };

    map<string, TraceEntry> entries;

    bool deriveComponents(
      const string& fname,
      const SensorDB& sensorDB,
      TraceEntry& entry);

    string deriveName(
      const string& country,
      const TraceTruth& truth);
    
    void printCSVHeader(ofstream& fout) const;

    string basename(const string& fname) const;


  public:

    TraceDB();

    ~TraceDB();

    bool log(
      const TraceTruth& truth,
      const SensorDB& sensorDB,
      const TrainDB& trainDB);

    bool log(
      const string& fname,
      const vector<Alignment>& align,
      const unsigned peakCount);

    string lookupSensor(const string& fname) const;
    string lookupTime(const string& fname) const;

    string lookupTrueTrain(const string& fname) const;
    double lookupTrueSpeed(const string& fname) const;

    void printCSV(
      const string& fname,
      const bool appendFlag,
      const TrainDB& trainDB) const;
};

#endif
