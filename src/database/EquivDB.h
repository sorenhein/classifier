/*
 * Simple storage of trains that we consider equivalent (and that are
 * probably identical, in fact).
 * Only does pairwise equivalences.  If we need more, we could always
 * do lots of pairs...
 */

#ifndef TRAIN_EQUIVDB_H
#define TRAIN_EQUIVDB_H

#include <map>
#include <vector>

#include "Entity.h"

using namespace std;




class EquivDB
{
  private:

    vector<unsigned> fieldCounts;

    list<vector<string>> equivalences;


  public:

    EquivDB();

    ~EquivDB();

    void reset();

    bool readFile(const string& fname);

    list<vector<string>>::const_iterator begin() 
      const { return equivalences.begin(); }
    list<vector<string>>::const_iterator end() 
      const { return equivalences.end(); }    

};

#endif
