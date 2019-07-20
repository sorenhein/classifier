/*
 * For some train types it seems that the stated axle positions are
 * not optimal for detection.  The most necessary corrections are
 * stored here.
 */

#ifndef TRAIN_CORRECTIONDB_H
#define TRAIN_CORRECTIONDB_H

#include <map>

#include "Entity.h"

using namespace std;


enum CorrectionFieldStrings
{
  CORR_OFFICIAL_NAME = 0,
  CORR_STRINGS_SIZE = 1
};

enum CorrectionFieldStringVectors
{
  CORR_STRING_VECTORS_SIZE = 0
};

enum CorrectionFieldIntVectors
{
  CORR_DELTAS = 0,
  CORR_INT_VECTORS_SIZE = 1
};

enum CorrectionFieldInts
{
  CORR_INTS_SIZE = 0
};

enum CorrectionFieldBools
{
  CORR_BOOLS_SIZE = 0
};


class CorrectionDB
{
  private:

    list<CorrespondenceEntry> fields;

    vector<unsigned> fieldCounts;

    vector<Entity> entries;

    map<string, int> correctionMap;


    void configure();


  public:

    CorrectionDB();

    ~CorrectionDB();

    void reset();

    bool readFile(const string& fname);

    // The name is Without the _N / _R
    vector<int> const * getIntVector(const string& officialName) const; 
};

#endif
