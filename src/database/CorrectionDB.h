/*
 * For some train types it seems that the stated truth trains are
 * not correct.  The manual corrections are stored here.
 */

#ifndef TRAIN_CORRECTIONDB_H
#define TRAIN_CORRECTIONDB_H

#include <map>
#include <vector>

#include "Entity.h"

using namespace std;




class CorrectionDB
{
  private:

    vector<unsigned> fieldCounts;

    list<vector<string>> corrections;


  public:

    CorrectionDB();

    ~CorrectionDB();

    void reset();

    bool readFile(const string& fname);

    list<vector<string>>::const_iterator begin() 
      const { return corrections.begin(); }
    list<vector<string>>::const_iterator end() 
      const { return corrections.end(); }    

};

#endif
