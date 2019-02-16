#ifndef TRAIN_PEAKPARTIAL_H
#define TRAIN_PEAKPARTIAL_H

#include <vector>
#include <string>

using namespace std;

class Peak;


class PeakPartial
{
  private:

    unsigned modelNo;
    bool reverseFlag;

    bool modelUsedFlag;
    bool aliveFlag;
    bool skippedFlag;
    bool newFlag;
    vector<Peak *> peaks;
    unsigned lastIndex;

    vector<unsigned> lower;
    vector<unsigned> upper;
    vector<unsigned> target;
    vector<unsigned> indexUsed;
    unsigned numUsed;


    bool dominates(const PeakPartial& p2) const;

    bool samePeaks(const PeakPartial& p2) const;

    void extend(const PeakPartial& p2);

    string strIndex(
      const unsigned peakNo,
      const unsigned offset) const;

  public:

    PeakPartial();

    ~PeakPartial();

    void reset();

    void init(
      const unsigned modelNoIn,
      const bool reverseFlagIn,
      const unsigned startIn);

    void setPeak(
      const unsigned peakNo,
      Peak& peak) const;

    bool supersede(const PeakPartial& p2);

    bool hasPeak(const unsigned peakNo) const;
    bool hasRange(const unsigned peakNo) const;

    unsigned numMatches() const;

    string strTarget(
      const unsigned peakNo,
      const unsigned offset) const;

    string strHeader() const;

    string str(const unsigned offset) const;
};

#endif
