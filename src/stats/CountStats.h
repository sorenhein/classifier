/*
    A simple class for counter-type statistics.
 */

#ifndef TRAIN_COUNTSTATS_H
#define TRAIN_COUNTSTATS_H

#include <map>
#include <vector>
#include <string>

using namespace std;


class CountStats
{
  private:

    string title;

    map<string, unsigned> counts;

    unsigned fieldLength;


  public:

    CountStats();

    ~CountStats();

    void reset();

    void init(
      const string& titleIn,
      const vector<string>& tagsIn);

    bool log(
      const string& tag,
      const unsigned incr = 1);

    string str() const;
};

#endif
