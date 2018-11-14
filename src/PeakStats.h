#ifndef TRAIN_PEAKSTATS_H
#define TRAIN_PEAKSTATS_H

#include <vector>
#include <string>

#include "struct.h"

#define PEAKSTATS_END_COUNT 4


using namespace std;


class PeakStats
{
  private:

    struct Entry
    {
      unsigned good;
      unsigned len;

      Entry()
      {
        good = 0; 
        len = 0;
      };

      void operator +=(const Entry& e2)
      {
        good += e2.good;
        len += e2.len;
      };
    };

    // Number of seen peaks (total and matched to true).
    vector<Entry> statsSeen;

    // Number of true peaks (total and matched to seen).
    vector<Entry> statsTrueFront;
    Entry statsTrueCore;
    vector<Entry> statsTrueBack;

    // Number of missed true peaks (among seen peaks).
    vector<vector<unsigned>> missedTrueFront;
    vector<unsigned> missedTrueCore;
    vector<vector<unsigned>> missedTrueBack;

    vector<string> typeNamesSeen;
    vector<string> typeNamesTrue;

    string percent(
      const unsigned num,
      const unsigned denom) const;

    void printTrueHeader(ofstream& fout) const;

    void printTrueLine(
      ofstream& fout,
      const string& text,
      const Entry& e,
      const vector<unsigned>& v,
      Entry& ecum,
      vector<unsigned>& vcum) const;

    void printTrueTable(ofstream& fout) const;

    void printSeenHeader(ofstream& fout) const;

    void printSeenLine(
      ofstream& fout,
      const string& text,
      const Entry& e) const;

    void printSeenTable(ofstream& fout) const;


  public:

    PeakStats();

    ~PeakStats();

    void reset();

    void logSeenHit(const PeakSeenType stype);

    void logSeenMiss(const PeakSeenType stype);

    void logTrueHit(
      const unsigned trueNo,
      const unsigned trueLen);

    void logTrueMiss(
      const unsigned trueNo,
      const unsigned trueLen,
      const PeakTrueType ttype);

    void print(const string& fname) const;

};

#endif
