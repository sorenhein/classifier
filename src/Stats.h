#ifndef TRAIN_STATS_H
#define TRAIN_STATS_H

#include <map>

using namespace std;


class Stats
{
  private:

    map<string, map<string, int>> stats;


  public:

    Stats();

    ~Stats();

    void log(
      const string& trainReal,
      const string& trainClassified);

    void print(const string& fname) const;
};

#endif
