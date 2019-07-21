#include <iostream>

#include "CorrectionDB.h"


CorrectionDB::CorrectionDB()
{
  CorrectionDB::reset();
}


CorrectionDB::~CorrectionDB()
{
}


void CorrectionDB::reset()
{
  fields.clear();
  fieldCounts.clear();
  entries.clear();
  correctionMap.clear();

  CorrectionDB::configure();
}


void CorrectionDB::configure()
{
  fields =
  {
    { "OFFICIAL_NAME", CORRESPONDENCE_STRING, CORR_OFFICIAL_NAME }
  };

  fieldCounts =
  {
    CORR_STRINGS_SIZE,
    CORR_STRING_VECTORS_SIZE,
    0,
    CORR_INT_VECTORS_SIZE,
    CORR_INTS_SIZE,
    CORR_BOOLS_SIZE,
    0
  };
}


bool CorrectionDB::readFile(const string& fname)
{
  Entity entry;
  if (! entry.readSeriesFile(fname, fields, fieldCounts, CORR_DELTAS))
  {
    cout << "Could not read correction file " << fname << endl;
    return false;
  }

  correctionMap[entry.getString(CORR_OFFICIAL_NAME)] = entries.size();

  entries.push_back(entry);
  return true;
}


vector<int> const * CorrectionDB::getIntVector(
  const string& officialName) const // Without the _N / _R
{
  auto c = correctionMap.find(officialName);
  if (c == correctionMap.end())
    return nullptr;
  else
    return &entries[c->second].getIntVector(CORR_DELTAS);
}

