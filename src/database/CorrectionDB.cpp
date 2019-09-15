#include <iostream>
#include <fstream>

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
  fieldCounts =
  {
    3, // trace name, old train, new train
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
  while (entry.readCommaLine(fin, errFlag, 3))
  {
    corrections.push_back(vector<string>());
    vector<string>& corr = corrections.back();
    corr.push_back(entry.getString(0));
    corr.push_back(entry.getString(1));
    corr.push_back(entry.getString(2));
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

