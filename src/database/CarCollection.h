#ifndef TRAIN_CARCOLLECTION_H
#define TRAIN_CARCOLLECTION_H

#include "Collection.h"

using namespace std;


enum CarFieldStrings
{
  CAR_OFFICIAL_NAME = 0,
  CAR_NAME = 1,
  CAR_CONFIGURATION_UIC = 2,
  CAR_COMMENT = 3,
  CAR_STRINGS_SIZE = 4
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
  CAR_CLASS = 2,
  CAR_WEIGHT = 3,
  CAR_WHEEL_LOAD = 4,
  CAR_SPEED = 5,
  CAR_LENGTH = 6,
  CAR_DIST_WHEELS = 7,
  CAR_DIST_WHEELS1 = 8,
  CAR_DIST_WHEELS2 = 9,
  CAR_DIST_MIDDLES = 10,
  CAR_DIST_PAIR = 11,
  CAR_DIST_FRONT_TO_WHEEL = 12,
  CAR_DIST_WHEEL_TO_BACK = 13,
  CAR_DIST_FRONT_TO_MID1 = 14,
  CAR_DIST_BACK_TO_MID2 = 15,
  CAR_INTS_SIZE = 16
};

enum CarFieldBools
{
  CAR_POWER = 0,
  CAR_RESTAURANT = 1,
  CAR_SYMMETRY = 2,
  CAR_FOURWHEEL_FLAG = 3,
  CAR_BOOLS_SIZE = 4
};


class CarCollection: public Collection
{
  private:

    void complete(Entity& entry);

    void configure();

    bool fillInEquation(
      int &lhs,
      vector<int>& rhs,
      const unsigned len) const;

    bool fillInDistances(Entity& entry) const;


  public:

    CarCollection();

    void reset();

    bool appendAxles(
      const unsigned index,
      const bool reverseFlag,
      int& posRunning,
      vector<int>& axles) const;
};

#endif
