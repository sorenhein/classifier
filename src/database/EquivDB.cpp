#include <iostream>
#include <fstream>

#include "EquivDB.h"


EquivDB::EquivDB()
{
  EquivDB::reset();
}


EquivDB::~EquivDB()
{
}


void EquivDB::reset()
{
  fieldCounts =
  {
    2, // Equivalences
    0,
    0,
    0,
    0,
    0,
    0,
    0
  };

  equivalences.clear();
}



bool EquivDB::readFile(
  const string& fname)
{
  Entity entry;
  entry.init(fieldCounts);

  bool errFlag;
  ifstream fin;

  fin.open(fname);
  // If we wanted to read more than two equivalences, this would
  // be the place to do it.
  while (entry.readCommaLine(fin, errFlag, 2))
  {
    equivalences.push_back(vector<string>());
    vector<string>& eq = equivalences.back();
    eq.push_back(entry.getString(0));
    eq.push_back(entry.getString(1));
  }
  fin.close();

  if (errFlag)
  {
    cout << "Could not read equivalence file " << fname << endl;
    return false;
  }
  else
    return true;
}

