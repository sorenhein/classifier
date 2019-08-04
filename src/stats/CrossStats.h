#ifndef TRAIN_CROSSSTATS_H
#define TRAIN_CROSSSTATS_H

#include <vector>
#include <list>
#include <map>
#include <string>

using namespace std;


class CrossStats
{
  private:

    struct ListEntry
    {
      string name;
      unsigned index;
      vector<bool> connected;
    };


    map<string, unsigned> nameMap;

    vector<string> numberMap;

    vector<int> countEntries;

    unsigned numEntries;

    vector<vector<int>> countCross;


    void resize(const unsigned n);

    unsigned nameToNumber(const string& name);

    void setConnectivity(
      list<ListEntry>& segments,
      vector<unsigned>& connectCount) const;

    void condense(
      list<ListEntry>& segments,
      vector<unsigned>& connectCount) const;

    void segment(
      list<string>& orphans,
      list<string>& singletons,
      list<map<string, bool>>& segmentMaps) const;

    string percent(
      const int num,
      const int denom) const;

    string strHeader(const map<string, bool>& selectMap) const;

    string strLines(const map<string, bool>& selectMap) const;

    string strSimple(
      const list<string>& simple,
      const string& text) const;


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
