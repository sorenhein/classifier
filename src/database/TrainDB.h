#ifndef TRAIN_TRAINDB_H
#define TRAIN_TRAINDB_H

#include <map>

#include "Entity.h"

using namespace std;

class CarDB;
class CorrectionDB;


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
  TRAIN_CARS = 2, // TODO Needed?
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
  TRAIN_NUM_CARS = 2,
  TRAIN_INTS_SIZE = 3
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

    list<string> selected;


    bool correct(
      const CorrectionDB& correctionDB,
      Entity& entry);

    bool complete(
      const CarDB& carDB,
      Entity& entry);

    void configure();


  public:

    TrainDB();

    ~TrainDB();

    void reset();

    bool readFile(
      const CarDB& carDB,
      const CorrectionDB& correctionDB,
      const string& fname);

    unsigned numAxles(const unsigned trainNo) const;
    unsigned numCars(const unsigned trainNo) const;

    int lookupNumber(const string& offName) const;
    string lookupName(const unsigned trainNo) const;

    bool reversed(const unsigned trainNo) const;

    bool isInCountry(
      const unsigned trainNo,
      const string& country) const;

    void getPeakPositions(
      const unsigned trainNo,
      vector<double>& peakPositions) const; // In m

    bool selectByAxles(
      const list<string>& countries,
      const unsigned minAxles,
      const unsigned maxAxles);

    bool selectByCars(
      const list<string>& countries,
      const unsigned minCars,
      const unsigned maxCars);

    list<string>::const_iterator begin() const { return selected.begin(); }
    list<string>::const_iterator end() const { return selected.end(); }

};

#endif
