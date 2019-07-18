#ifndef TRAIN_TRAINCOLLECTION_H
#define TRAIN_TRAINCOLLECTION_H

#include <map>

#include "Entity.h"

using namespace std;


enum TrainFieldStrings
{
  TRAIN_OFFICIAL_NAME = 0,
  TRAIN_NAME = 1,
  TRAIN_STRINGS_SIZE = 2
};

enum TrainFieldVectors
{
  TRAIN_COUNTRIES = 0,
  TRAIN_CAR_ORDER = 1,
  TRAIN_VECTORS_SIZE = 2
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
  TRAIN_BOOLS_SIZE = 1
};


class TrainCollection
{
  private:

    list<CorrespondenceEntry> fields;

    vector<unsigned> fieldCounts;

    vector<Entity> entries;

    map<string, int> offTrainMap;


    bool complete(Entity& entry);

    void configure();


  public:

    TrainCollection();

    ~TrainCollection();

    void reset();

    bool readFile(const string& fname);

    int lookupTrainNumber(const string& offName) const;
};

#endif
