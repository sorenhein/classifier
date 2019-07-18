#ifndef TRAIN_CARCOLLECTION_H
#define TRAIN_CARCOLLECTION_H

#include <map>

#include "Entity.h"

using namespace std;


enum CarFieldStrings
{
  CAR_OFFICIAL_NAME = 0,
  CAR_NAME = 1,
  CAR_CONFIGURATION_UIC = 2,
  CAR_CLASS = 3,
  CAR_COMMENT = 4,
  CAR_STRINGS_SIZE = 5
};

enum CarFieldVectors
{
  CAR_USAGE = 0,
  CAR_COUNTRIES = 1,
  CAR_VECTORS_SIZE = 2
};

enum CarFieldInts
{
  CAR_INTRODUCTION = 0,
  CAR_CAPACITY = 1,
  CAR_WEIGHT = 2,
  CAR_WHEEL_LOAD = 3,
  CAR_SPEED = 4,
  CAR_LENGTH = 5,
  CAR_DIST_WHEELS = 6,
  CAR_DIST_WHEELS1 = 7,
  CAR_DIST_WHEELS2 = 8,
  CAR_DIST_MIDDLES = 9,
  CAR_DIST_PAIR = 10,
  CAR_DIST_FRONT_TO_WHEEL = 11,
  CAR_DIST_WHEEL_TO_BACK = 12,
  CAR_DIST_FRONT_TO_MID1 = 13,
  CAR_DIST_BACK_TO_MID2 = 14,
  CAR_INTS_SIZE = 15
};

enum CarFieldBools
{
  CAR_POWER = 0,
  CAR_RESTAURANT = 1,
  CAR_SYMMETRY = 2,
  CAR_FOURWHEEL_FLAG = 3,
  CAR_BOOLS_SIZE = 4
};


class CarCollection
{
  private:

    list<CorrespondenceEntry> fields;

    vector<unsigned> fieldCounts;

    vector<Entity> entries;

    map<string, int> offCarMap;


    bool complete(Entity& entry);

    void configure();

    bool fillInEquation(
      int &lhs,
      vector<int>& rhs,
      bool& inconsistentFlag,
      const unsigned len) const;

    bool fillInDistances(Entity& entry) const;


  public:

    CarCollection();

    ~CarCollection();

    void reset();

    bool readFile(const string& fname);

    bool appendAxles(
      const int carNo,
      int& posRunning,
      vector<int>& axles) const;

    int lookupCarNumber(const string& offName) const;

    string strDistances(const Entity& entry) const;
};

#endif
