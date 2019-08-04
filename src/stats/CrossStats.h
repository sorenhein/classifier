#ifndef TRAIN_CROSSSTATS_H
#define TRAIN_CROSSSTATS_H

#include <vector>
#include <map>
#include <string>

using namespace std;


class CrossStats
{
  private:

    map<string, unsigned> nameMap;

    vector<string> numberMap;

    vector<int> countEntries;

    unsigned numEntries;

    vector<vector<int>> countCross;

    void resize(const unsigned n);

    unsigned nameToNumber(const string& name);

    string percent(
      const int num,
      const int denom) const;

    string strHeader() const;


  public:

    CrossStats();

    ~CrossStats();

    void log(
      const string& trainActual,
      const string& trainEstimate);

    string str() const;

    string strCSV() const;

    string strPercentCSV() const;
};

#endif
