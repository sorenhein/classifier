#ifndef TRAIN_PEAKSTATS_H
#define TRAIN_PEAKSTATS_H

#include <vector>
#include <string>

using namespace std;

enum PeakSeenType
{
  PEAK_SEEN_TOO_EARLY = 0,
  PEAK_SEEN_EARLY = 1,
  PEAK_SEEN_CORE = 2,
  PEAK_SEEN_LATE = 3,
  PEAK_SEEN_TOO_LATE = 4,
  PEAK_SEEN_SIZE = 5
};

enum PeakTrueType
{
  PEAK_TRUE_TOO_EARLY = 0,
  PEAK_TRUE_MISSED = 1,
  PEAK_TRUE_TOO_LATE = 2,
  PEAK_TRUE_SIZE = 3
};

#define PEAKSTATS_END_COUNT 4


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

    void writeTrueHeader(ofstream& fout) const;

    void writeTrueLine(
      ofstream& fout,
      const string& text,
      const Entry& e,
      const vector<unsigned>& v,
      Entry& ecum,
      vector<unsigned>& vcum) const;

    void writeTrueTable(ofstream& fout) const;

    void writeSeenHeader(ofstream& fout) const;

    void writeSeenLine(
      ofstream& fout,
      const string& text,
      const Entry& e) const;

    void writeSeenTable(ofstream& fout) const;


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

    void write(const string& fname) const;

};

#endif
