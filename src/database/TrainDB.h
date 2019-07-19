#ifndef TRAIN_TRAINDB_H
#define TRAIN_TRAINDB_H

#include <map>

#include "Entity.h"

using namespace std;

class CarDB;


enum TrainFieldStrings
{
  TRAIN_OFFICIAL_NAME = 0,
  TRAIN_NAME = 1,
  TRAIN_STRINGS_SIZE = 2
};

enum TrainFieldStringVectors
{
  TRAIN_COUNTRIES = 0,
  TRAIN_CAR_ORDER = 1,
  TRAIN_CARS = 2,
  TRAIN_STRING_VECTORS_SIZE = 3
};

enum TrainFieldIntVectors
{
  TRAIN_AXLES = 0,
  TRAIN_INT_VECTORS_SIZE = 1
};

enum TrainFieldInts
{
  TRAIN_INTRODUCTION = 0,
  TRAIN_RETIREMENT = 1,
  TRAIN_INTS_SIZE = 2
};

enum TrainFieldBools
{
  TRAIN_SYMMETRY = 0,
  TRAIN_REVERSED = 1,
  TRAIN_BOOLS_SIZE = 2
};


class TrainDB
{
  private:

    list<CorrespondenceEntry> fields;

    vector<unsigned> fieldCounts;

    vector<Entity> entries;

    map<string, int> offTrainMap;


    bool complete(
      const CarDB& carDB,
      const string& err,
      Entity& entry);

    void configure();


  public:

    TrainDB();

    ~TrainDB();

    void reset();

    bool readFile(
      const CarDB& carDB,
      const string& err,
      const string& fname);

    unsigned numAxles(const unsigned trainNo) const;
    unsigned numCars(const unsigned trainNo) const;

    int lookupTrainNumber(const string& offName) const;
    string lookupTrainName(const unsigned trainNo) const;

    bool reversed(const unsigned trainNo) const;
};

#endif
