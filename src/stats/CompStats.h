/*
 * This class keeps track of the rank (1, 2, 3, ...) of the true
 * solution among the detected ones.  The class can be used by
 * sensor, by train type etc.
 */

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

    string strHeader(const string& tag) const;

    string strEntry(const Entry& entry) const;

    string strHeaderCSV(const string& tag) const;

    string strEntryCSV(const Entry& entry) const;

  public:

    CompStats();

    ~CompStats();

    void log(
      const string& key,
      const unsigned rank,
      const double residuals);

    void fail(const string& key);

    string str(const string& tag) const;

    void write(
      const string& fname,
      const string& tag) const;

    string strCSV(const string& tag) const;

    void writeCSV(
      const string& fname,
      const string& tag) const;
};

#endif
