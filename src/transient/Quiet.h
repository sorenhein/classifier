#ifndef TRAIN_QUIET_H
#define TRAIN_QUIET_H

#include <vector>
#include <string>

#include "trans.h"

using namespace std;


class Quiet
{
  private:

    unsigned durationCoarse;
    unsigned durationFine;

    struct QuietStats
    {
      unsigned start;
      unsigned len;
      float mean;
      float sdev;
      QuietGrade grade;
    };

    vector<Interval> quiet;

    vector<float> synth;

    Interval writeInterval;
    Interval activeInterval;


    void makeStarts(
      const Interval& interval,
      const bool fromBackFlag,
      const unsigned duration,
      vector<QuietStats>& startList) const;

    void makeStats(
      const vector<float>& samples,
      QuietStats& qentry,
      QuietStats& qstats) const;

    QuietGrade isQuiet(const QuietStats& qstats) const;

    void addQuiet(
      const unsigned start,
      const unsigned len,
      const QuietGrade grade,
      const float mean);

    unsigned curate(const vector<QuietStats>& qstats) const;

    void getFinetuneStatistics(
      const vector<float>& samples,
      vector<QuietStats>& fineStarts,
      vector<QuietStats>& fineList,
      float& sdevThreshold) const;

    void adjustIntervals(
      const bool fromBackFlag,
      QuietStats& qstatsCoarse,
      const unsigned index);

    void setFineInterval(
      const QuietStats& qstatsCoarse,
      const bool fromBackFlag,
      const unsigned sampleSize,
      Interval& intervalFine) const;

    void finetune(
      const vector<float>& samples,
      const bool fromBackFlag,
      QuietStats& qstats);

    void adjustOutputIntervals(
      const vector<QuietStats>& startList,
      const Interval& avail,
      const bool fromBackFlag);

    void makeSynth(const vector<QuietStats>& startList);

    void printStats(
      const QuietStats& qstats,
      const unsigned first,
      const bool flag) const;

  public:

    Quiet();

    ~Quiet();

    bool detect(
      const vector<float>& samples,
      const double sampleRate,
      const Interval& available,
      const bool fromBackFlag,
      Interval& active);

    void writeFile(const string& filename) const;
};

#endif
