#ifndef TRAIN_COMPSTATS_H
#define TRAIN_COMPSTATS_H

#include <map>
#include <vector>
#include <string>

using namespace std;


class CompStats
{
  private:

    struct Entry
    {
      unsigned no;
      double sum;

      Entry() { no = 0; sum = 0.; }
    };

    map<string, vector<Entry>> stats;

    void printHeader(
      ofstream& fout,
      const string& tag) const;

    string strEntry(const Entry& entry) const;

    void printHeaderCSV(
      ofstream& fout,
      const string& tag) const;

    string strEntryCSV(const Entry& entry) const;

  public:

    CompStats();

    ~CompStats();

    void log(
      const string& key,
      const unsigned rank,
      const double residuals);

    void print(
      const string& fname,
      const string& tag) const;

    void printCSV(
      const string& fname,
      const string& tag) const;
};

#endif
