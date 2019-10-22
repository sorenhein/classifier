#ifndef TRAIN_QUIET_H
#define TRAIN_QUIET_H

#include <vector>
#include <list>
#include <string>

#include "trans.h"

using namespace std;


class Quiet
{
  private:

    unsigned durationCoarse;
    unsigned durationFine;
    unsigned padSamples;

    list<QuietInterval> quietCoarse;

    vector<float> synth;


    void makeStarts(
      const QuietInterval& interval,
      const bool fromBackFlag,
      const unsigned duration,
      list<QuietInterval>& quietList) const;

    void makeStats(
      const vector<float>& samples,
      QuietInterval& qint) const;

    unsigned annotateList(
      const vector<float>& samples,
      list<QuietInterval>::iterator qbegin,
      list<QuietInterval>& quietList) const;

    void getFinetuneStatistics(
      const vector<float>& samples,
      list<QuietInterval>& fineStarts,
      float& sdevThreshold) const;

    void adjustIntervals(
      const bool fromBackFlag,
      QuietInterval& qintCoarse,
      const unsigned index) const;

    void setFineInterval(
      const QuietInterval& qintCoarse,
      const bool fromBackFlag,
      const unsigned sampleSize,
      QuietInterval& intervalFine) const;

    void finetune(
      const vector<float>& samples,
      const bool fromBackFlag,
      QuietInterval& qint) const;

    void adjustOutputIntervals(
      const QuietInterval& qint,
      const bool fromBackFlag,
      const bool padFlag,
      QuietInterval& avail) const;

    void synthesize(
      const QuietInterval& available,
      const list<QuietInterval>& actives);

    bool findActive(
      const vector<float>& samples,
      list<QuietInterval>& actives,
      QuietInterval& runningInterval,
      QuietInterval& quietInterval,
      list<QuietInterval>::const_iterator& qit) const;


  public:

    Quiet();

    ~Quiet();

    void detect(
      const vector<float>& samples,
      const float sampleRate,
      const bool fromBackFlag,
      QuietInterval& available);

    void detectIntervals(
      const vector<float>& samples,
      const QuietInterval& available,
      list<QuietInterval>& actives);

    void writeFile(
      const string& filename,
      const unsigned offset) const;
};

#endif
