#ifndef TRAIN_PEAKSTATS_H
#define TRAIN_PEAKSTATS_H

#include <vector>
#include <string>

#include "struct.h"


using namespace std;


class PeakStats
{
  private:

    struct Entry
    {
      unsigned no;
      unsigned count;
    };

    // Number of true (resp. seen) peaks of a given number (0, 1, 2, ...)
    // that have matched among the seen peaks (resp. true peaks).
    vector<Entry> matchedTrue;
    vector<Entry> matchedSeen;

    unsigned countTrue;
    unsigned countSeen;

    // Number of peaks of each type that are true.
    vector<Entry> statsSeen;

    // Number of true peaks of each type.
    vector<unsigned> statsTrue;

    vector<string> typeNames;


    string percent(
      const unsigned num,
      const unsigned denom) const;

    void printDetailTable(
      ofstream& fout,
      const string& title,
      const bool indexFlag,
      const vector<Entry>& matched) const;


  public:

    PeakStats();

    ~PeakStats();

    void reset();

    void log(
      const int matchNoTrue,
      const int matchNoSeen,
      const PeakType type);

    void print(const string& fname) const;

    void printDetail(const string& fname) const;

};

#endif
