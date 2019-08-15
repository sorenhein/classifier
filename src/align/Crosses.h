/*
    Keeps track of correlations between true trains.
 */

#ifndef TRAIN_CROSSES_H
#define TRAIN_CROSSES_H

#include <vector>
#include <string>

#include "Alignment.h"

using namespace std;

class TrainDB;


class Crosses
{
  private:

    vector<vector<Alignment>> crosses;



  public:

    Crosses();

    ~Crosses();

    void reset();

    void log(const TrainDB& trainDB);

    string strTrue() const;
};

#endif
