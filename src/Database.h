#ifndef TRAIN_DATABASE_H
#define TRAIN_DATABASE_H

#include <vector>
#include <list>
#include <map>

#include "Clusters.h"
#include "struct.h"

using namespace std;


enum TrainLevel
{
  TRAIN_NONE = 0,
  TRAIN_FIRST = 1,
  TRAIN_SECOND = 2
};


struct CarEntry
{
  string officialName; // Example: Avmz 801
  string name; // Example: "DB Class 801 First Class Intermediate Car"
  vector<string> usage; // ICE1
  vector<string> countries; // Three-letter codes, e.g. DEU
  int introduction; // Year
  bool powerFlag;
  int capacity;
  TrainLevel level;
  bool restaurantFlag;
  int weight; // In kg
  int wheelLoad; // In kg
  int speed; // In km/h
  string configurationUIC; // E.g. 2'2' or Bo'Bo'
  int length; // In mm
  bool symmetryFlag; // True if wheel geometry is symmetric

  // Not all of these have to be specified simultaneously.
  // At the moment we manage to calculate the missing ones for the
  // combinations that occur...

  int distWheels;  // In mm: Distance between wheels in a pair
  int distWheels1;  // In mm: Distance between wheels in front pair
  int distWheels2;  // In mm: Distance between wheels in back pair
  int distMiddles; // In mm: Distance between middles of wheel pairs
  int distPair; // In mm: Inner distance between pairs
  int distFrontToWheel; // In mm
  int distWheelToBack; // In mm
  int distFrontToMid1; // In mm
  int distBackToMid2; // In mm
};


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
  vector<Clusters> clusterList;  // First one is for 2 clusters, etc.
};

struct DatabaseCar
{
  double shortGap;
  double longGap;
  int count;
  int countInternal;
};


class Database
{
  private:

   vector<CarEntry> carEntries;
   
   vector<TrainEntry> trainEntries;

   list<string> selectedTrains;

   map<string, unsigned> offCarMap;

   map<string, unsigned> offTrainMap;

   void printAxlesCSV(const TrainEntry& t) const;


  public:

    Database();

    ~Database();

    void logCar(const CarEntry& car);

    void logTrain(const TrainEntry& train);

    bool select(
      const string& countries,
      const unsigned minAxles,
      const unsigned maxAxles);

    list<string>::iterator begin() {return selectedTrains.begin(); }

    list<string>::iterator end() {return selectedTrains.end(); }

    bool getPerfectPeaks(
      const string& trainName,
      vector<PeakPos>& peaks) const; // In mm

    bool getPerfectPeaks(
      const int trainNo,
      vector<PeakPos>& peaks) const; // In mm

    const Clusters * getClusters(
      const int trainNo,
      const unsigned clusterSize) const;

    const CarEntry * lookupCar(const int carNo) const;

    int lookupCarNumber(const string& offName) const;

    int lookupTrainNumber(const string& offName) const;

    string lookupTrainName(const unsigned trainNo) const;

    bool trainsShareCountry(
      const int trainNo1,
      const int trainNo2) const;

    bool trainIsReversed(const int trainNo) const;

};

#endif

