#ifndef TRAIN_QUIET_H
#define TRAIN_QUIET_H

#include <vector>
#include <list>
#include <string>

#include "QuietData.h"
#include "trans.h"

using namespace std;


class Quiet
{
  private:

    unsigned durationCoarse;
    unsigned durationFine;

    /*
    struct QuietStats
    {
      unsigned start;
      unsigned len;
      float mean;
      float sdev;
      QuietGrade grade;
    };
    */

    list<QuietData> quietCoarse;


    vector<float> synth;

    Interval writeInterval;
    Interval activeInterval;


    void makeStarts(
      const Interval& interval,
      const bool fromBackFlag,
      const unsigned duration,
      list<QuietData>& quietList) const;

    void makeStats(
      const vector<float>& samples,
      QuietData& qentry) const;

    void annotateList(
      const vector<float>& samples,
      list<QuietData>& quietList) const;

    unsigned curate(const list<QuietData>& quietList) const;

    void getFinetuneStatistics(
      const vector<float>& samples,
      list<QuietData>& fineStarts,
      float& sdevThreshold) const;

    void adjustIntervals(
      const bool fromBackFlag,
      QuietData& qstatsCoarse,
      const unsigned index);

    void setFineInterval(
      const QuietData& qstatsCoarse,
      const bool fromBackFlag,
      const unsigned sampleSize,
      Interval& intervalFine) const;

    void finetune(
      const vector<float>& samples,
      const bool fromBackFlag,
      QuietData& qstats);

    void adjustOutputIntervals(
      const list<QuietData>& quietList,
      const Interval& avail,
      const bool fromBackFlag);

    void makeSynth(const list<QuietData>& quietList);


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
