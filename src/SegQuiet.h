#ifndef TRAIN_SEGQUIET_H
#define TRAIN_SEGQUIET_H

#include <vector>
#include <string>

#include "struct.h"

using namespace std;


class SegQuiet
{
  private:

    struct QuietStats
    {
      double mean;
      double sdev;
      double lower;
      double upper;
      unsigned len;
      unsigned numOutliers;
      double valOutliers;
      unsigned numRuns;
    };

    vector<Interval> quiet;

    vector<float> synth;

    unsigned offset;

    void makeStats(
      const vector<double>& samples,
      const unsigned first,
      const unsigned len,
      QuietStats& qstats) const;

    unsigned countRuns(
      const vector<Run>& runs,
      const unsigned first,
      const unsigned len) const;

    bool isQuiet(const QuietStats& qstats) const;

    void makeSynth(const unsigned l);

    void makeActive(
      vector<Interval>& active,
      const unsigned firstSampleNo,
      const unsigned l) const;

    void printStats(
      const QuietStats& qstats,
      const unsigned first) const;

    void printShortStats(
      const QuietStats& qstats,
      const unsigned first,
      const bool flag) const;

  public:

    SegQuiet();

    ~SegQuiet();

    void reset();

    bool detect(
      const vector<double>& samples, // TODO: Should use times[]
      const vector<Run>& runs,
      const unsigned firstSampleNo,
      vector<Interval>& active);

    void writeBinary(const string& origname) const;

    string headerCSV() const;

    string strCSV() const;
};

#endif
