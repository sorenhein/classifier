#ifndef TRAIN_PEAK_H
#define TRAIN_PEAK_H

#include <vector>
#include <string>

using namespace std;


class Peak
{
  private:

    unsigned index;
    float value;
    bool maxFlag;

    // The flank goes to the left of the peak.
    // There are dummy peaks on each end.
    // The following quantities are measured relative to the
    // previous peak and are always non-negative.

    float len; 
    float range;

    float areaCum; // From beginning of trace, may be negative

    unsigned clusterNo;

    string str(
      const float areaOut,
      const unsigned offset) const;

  public:

    Peak();

    ~Peak();

    void reset();

    void logSentinel(const float valueIn);

    void log(
      const unsigned indexIn,
      const float valueIn,
      const float areaFullIn,
      const Peak& peakPrev);

    void logCluster(const unsigned cno);

    void update(const Peak& peakPrev);

    unsigned getIndex() const;
    bool getMaxFlag() const;
    float getValue() const;
    float getLength() const;
    float getRange() const;
    float getArea() const;
    float getArea(const Peak& peakPrev) const;

    bool check(
      const Peak& p2,
      const unsigned offset) const;

    void operator += (const Peak& p2);

    void operator /= (const unsigned no);

    string strHeader() const;

    string str(const unsigned offset = 0) const;

    string str(
      const Peak& peakPrev,
      const unsigned offset = 0) const;
};

#endif
