#ifndef TRAIN_METRICS_H
#define TRAIN_METRICS_H

#include <vector>

#include "struct.h"

using namespace std;


struct MetricEntry
{
  double dist1;
  double dist2;
};


using namespace std;


class Metrics
{
  private:

    vector<MetricEntry> entriesCorrect;
    vector<MetricEntry> entriesIncorrect;


  public:

    Metrics();

    ~Metrics();

    double distanceAlignment(
      const double sumsq,
      const unsigned numPeaksUsed,
      const unsigned numAdd,
      const unsigned numDelete,
      const bool correctFlag);

    void printCSV(const string& fname) const;
};

#endif
