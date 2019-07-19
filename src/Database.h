/*
    A collection of information about trains and cars that we'll classify.
 */

#ifndef TRAIN_DATABASE_H
#define TRAIN_DATABASE_H

#include <vector>
#include <list>
#include <map>

#include "database/CarDB.h"
#include "database/TrainDB.h"

#include "struct.h"

using namespace std;


struct TrainEntry
{
  string officialName; // Example: "X74_SWE_28" (not really official...)
  string name; // Example: "ICE1 Refurbishment"
  int introduction; // Year
  int retirement; // Year
  vector<string> countries;
  bool symmetryFlag;
  bool reverseFlag;
  vector<int> carNumbers;
  vector<int> axles; // In mm starting from the first wheel
  bool fourWheelFlag; // Some cars "share" axles, look like 3-wheelers
};


class Database
{
  private:

    struct DatabaseSensor
    {
      string country;
      string type;
    };

    CarDB carDB;
   
    vector<TrainEntry> trainEntries;

TrainDB trainDB;

    map<string, DatabaseSensor> sensors;

    list<string> selectedTrains;

    map<string, int> offCarMap;

    map<string, unsigned> offTrainMap;

    void setCorrespondences();
    void resetCar(Entity& carEntry) const;


  public:

    Database();

    ~Database();

    void readCarFile(const string& fname);

    bool appendAxles(
      const int carNo,
      int& posRunning,
      vector<int>& axles) const;

    void logTrain(const TrainEntry& train);

    bool logSensor(
      const string& name,
      const string& country,
      const string& stype);

    bool select(
      const list<string>& countries,
      const unsigned minAxles,
      const unsigned maxAxles);

    list<string>::const_iterator begin() const {return selectedTrains.begin(); }

    list<string>::const_iterator end() const {return selectedTrains.end(); }

    unsigned axleCount(const unsigned trainNo) const;

    bool getPerfectPeaks(
      const string& trainName,
      vector<PeakPos>& peaks) const; // In m

    bool getPerfectPeaks(
      const unsigned trainNo,
      vector<PeakPos>& peaks) const; // In m

    int lookupCarNumber(const string& offName) const;

    int lookupTrainNumber(const string& offName) const;

    string lookupTrainName(const unsigned trainNo) const;

    string lookupSensorCountry(const string& sensor) const;

    bool trainIsInCountry(
      const unsigned trainNo,
      const string& country) const;

    bool trainIsReversed(const unsigned trainNo) const;
};

#endif

