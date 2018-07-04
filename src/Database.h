#ifndef TRAIN_DATABASE_H
#define TRAIN_DATABASE_H

#include <vector>
#include <list>
#include <map>

#include "const.h"

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
  int distWheels;  // In mm: Distance between wheels in a pair
  int distMiddles; // In mm: Distance between middles of wheel pairs
  int distPair; // In mm: Inner distance between pairs
  int distFrontToWheel; // In mm
  int distWheelToBack; // In mm
  int distFrontToMid1; // In mm
  int distBackToMid2; // In mm
};


struct TrainEntry
{
  string name; // Example: "ICE1 Refurbishment"
  int introduction; // Year
  vector<string> countries;
  vector<int> carNumbers;
};

struct DatabaseCar
{
  float shortGap;
  float longGap;
  int count;
  int countInternal;
};


struct CarGeometry
{
  int distFrontToWheel; // In mm
  int distWheels; // in mm
  int distPair; // In mm
  int distWheelToBack; // In mm
};


class Database
{
  private:

   vector<CarEntry> carEntries;
   
   vector<TrainEntry> trainEntries;

   map<string, unsigned> offNameMap;


  public:

    Database();

    ~Database();

    void logCar(const CarEntry& car);

    void logTrain(const TrainEntry& train);

    void getPerfectPeaks(
      const string& trainName,
      const string& country,
      vector<Peak>& peaks,
      const int speed) const;

    int lookupCarNumber(const string& offName) const;

    string lookupTrainName(const unsigned trainNo) const;

    void lookupCar(
      const DatabaseCar& dbCar,
      const string& country,
      const int year,
      list<int>& carList) const;
    
    int lookupTrain(
      const vector<list<int>>& carTypes,
      unsigned& firstCar,
      unsigned& lastCar,
      unsigned& carsMissing) const;
      
};

#endif

