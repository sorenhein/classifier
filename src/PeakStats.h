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
      unsigned good;
      unsigned reverse;
      unsigned len;

      Entry()
      {
        good = 0; reverse = 0; len = 0;
      };

      void operator +=(const Entry& e2)
      {
        good += e2.good;
        reverse += e2.reverse;
        len += e2.len;
      };
    };

    // Number of true peaks of a given number that have matches among 
    // the seen peaks.  There is a vector for the first N and the 
    // last N, and an entry for all others together.
    vector<Entry> matchedTrueFront;
    vector<Entry> matchedTrueMiddle;
    vector<Entry> matchedTrueBack;

    // Number of peaks of each type that are true.
    vector<Entry> statsSeen;

    // Number of true peaks of each type.
    vector<Entry> statsTrue;

    vector<string> typeNames;

    string percent(
      const unsigned num,
      const unsigned denom) const;

    string strDetailLine(
      const string& name,
      const unsigned width,
      const Entry& matched) const;

    void printDetailTable(
      ofstream& fout,
      const string& title,
      const bool indexFlag,
      const vector<Entry>& matched) const;


  public:

    PeakStats();

    ~PeakStats();

    void reset();

    void logSeenMatch(
      const unsigned matchNoTrue,
      const unsigned lenTrue,
      const PeakType type);

    void logSeenMiss(const PeakType type);

    void logTrueReverseMatch(
      const unsigned matchNoTrue,
      const unsigned lenTrue,
      const PeakType type);

    void logTrueReverseMiss(
      const unsigned matchNoTrue,
      const unsigned lenTrue);

    void print(const string& fname) const;

    void printDetail(const string& fname) const;

};

#endif
