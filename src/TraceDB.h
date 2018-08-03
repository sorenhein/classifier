#ifndef TRAIN_TRACEDB_H
#define TRAIN_TRACEDB_H

#include <vector>
#include <string>
#include <map>

#include "struct.h"

using namespace std;


class TraceDB
{
  private:

    struct TraceEntry
    {
      string date;
      string time;
      string sensor;
      TrainData trainTruth;
      vector<Alignment> align;
    };

    map<string, TraceEntry> entries;

    bool deriveComponents(
      const string& fname,
      TraceEntry& entry);

    string deriveName(const TraceTruth& truth);


  public:

    TraceDB();

    ~TraceDB();

    bool log(const TraceTruth& truth);

    bool log(
      const string& fname,
      const vector<Alignment>& align);

    void printCSV(const string& fname) const;
};

#endif
