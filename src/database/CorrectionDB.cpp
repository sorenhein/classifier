#include <iostream>
#include <fstream>

#include "CorrectionDB.h"

// trace name, 5 old fields, 5 new fields
#define CORR_FIELDS 11


CorrectionDB::CorrectionDB()
{
  CorrectionDB::reset();
}


CorrectionDB::~CorrectionDB()
{
}


void CorrectionDB::reset()
{
  fieldCounts =
  {
    CORR_FIELDS, 
    0,
    0,
    0,
    0,
    0,
    0,
    0
  };

  corrections.clear();
}



bool CorrectionDB::readFile(const string& fname)
{
  Entity entry;
  entry.init(fieldCounts);

  bool errFlag;
  ifstream fin;

  fin.open(fname);
  // If we wanted to read more than two equivalences, this would
  // be the place to do it.
  while (entry.readCommaLine(fin, errFlag, CORR_FIELDS))
  {
    corrections.push_back(vector<string>());
    vector<string>& corr = corrections.back();
    for (unsigned i = 0; i < CORR_FIELDS; i++)
      corr.push_back(entry.getString(i));
  }
  fin.close();

  if (errFlag)
  {
    cout << "Could not read correction file " << fname << endl;
    return false;
  }
  else
    return true;
}

