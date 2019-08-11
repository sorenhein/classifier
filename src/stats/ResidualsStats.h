/*
    Statistics on residuals for several trains.
 */

#ifndef TRAIN_RESIDUALSSTATS_H
#define TRAIN_RESIDUALSSTATS_H

#include <map>
#include <string>

#include "ResidualStats.h"

using namespace std;

struct Alignment;


class ResidualsStats
{
  private:

    map<string, ResidualStats> stats;


  public:

    ResidualsStats();

    ~ResidualsStats();

    void reset();

    void log(
      const string& trainName,
      const Alignment& alignment);

    void calculate();

    string str() const;

    void write(const string& dir) const;
};

#endif
