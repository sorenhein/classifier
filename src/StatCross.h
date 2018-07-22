#ifndef TRAIN_STAT_CROSS_H
#define TRAIN_STAT_CROSS_H

#include <vector>
#include <map>
#include <string>

using namespace std;


class StatCross
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


  public:

    StatCross();

    ~StatCross();

    void log(
      const string& trainActual,
      const string& trainEstimate);

    void printCountCSV(const string& fname) const;

    void printPercentCSV(const string& fname) const;

    void printQuality() const;
};

#endif
