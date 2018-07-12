#ifndef TRAIN_STATS_H
#define TRAIN_STATS_H

#include <vector>
#include <string>

#include "StatCross.h"
#include "StatEntry.h"

using namespace std;


class Stats
{
  private:

    StatCross statCross;

    map<string, StatEntry> trainMap;
    vector<StatEntry> speedMap;
    vector<StatEntry> accelMap;
    map<string, vector<StatEntry>> trainSpeedMap;
    map<string, vector<StatEntry>> trainAccelMap;

    void printSpeed(
      ofstream& fout,
      const vector<StatEntry>& sMap) const;

    void printAccel(
      ofstream& fout,
      const vector<StatEntry>& sMap) const;

  public:

    Stats();

    ~Stats();

    void log(
      const string& trainActual,
      const vector<double>& motionActual,
      const string& trainEstimate,
      const vector<double>& motionEstimate,
      const double residuals);

    void printCrossCountCSV(const string& fname) const;

    void printCrossPercentCSV(const string& fname) const;

    void printOverviewCSV(const string& fname) const;

    void printDetailsCSV(const string& fname) const;
};

#endif
