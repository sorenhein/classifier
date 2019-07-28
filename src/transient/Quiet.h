#ifndef TRAIN_QUIET_H
#define TRAIN_QUIET_H

#include <vector>
#include <string>

#include "trans.h"

using namespace std;


class Quiet
{
  private:

    struct QuietStats
    {
      float mean;
      float sdev;
      unsigned len;
    };

    vector<Interval> quiet;

    vector<float> synth;

    Interval writeInterval;
    Interval activeInterval;

    void makeStarts(
      const Interval& interval,
      const bool fromBackFlag,
      vector<unsigned>& startList) const;

    void makeStats(
      const vector<float>& samples,
      const unsigned first,
      const unsigned len,
      QuietStats& qstats) const;

    QuietGrade isQuiet(const QuietStats& qstats) const;

    void addQuiet(
      const unsigned start,
      const unsigned len,
      const QuietGrade grade,
      const float mean);

    unsigned curate() const;

    void setFinetuneRange(
      const vector<float>& samples,
      const bool fromBackFlag,
      const Interval& quietInt,
      vector<unsigned>& fineStarts) const;

    void getFinetuneStatistics(
      const vector<float>& samples,
      vector<unsigned>& fineStarts,
      vector<QuietStats>& fineList,
      float& sdevThreshold) const;

    void adjustIntervals(
      const bool fromBackFlag,
      Interval& quietInt,
      const unsigned index);

    void finetune(
      const vector<float>& samples,
      const bool fromBackFlag,
      Interval& quietInt);

    void adjustOutputIntervals(
      const Interval& avail,
      const bool fromBackFlag);

    void makeSynth();

    void printStats(
      const QuietStats& qstats,
      const unsigned first,
      const bool flag) const;

  public:

    Quiet();

    ~Quiet();

    void reset();

    bool detect(
      const vector<float>& samples,
      const Interval& available,
      const bool fromBackFlag,
      Interval& active);

    void writeFile(const string& filename) const;
};

#endif
