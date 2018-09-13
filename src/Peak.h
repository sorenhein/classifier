#ifndef TRAIN_PEAK_H
#define TRAIN_PEAK_H

#include <vector>
#include <string>

using namespace std;


class Peak
{
  private:

    // We mainly quantify the left flank of the peaks.
    // There are dummy peaks on each end of the peak list.

    // Absolute quantities associated with the peak.
    unsigned index;
    bool maxFlag;
    float value; // Signed
    float areaCum; // Integral from beginning of trace, signed

    // The following quantities are measured relative to other
    // peaks and are always non-negative.

    float len; // To the left, in samples
    float range; // To the left (always positive)
    float area; // To the left, from the lowest of the peaks (positive)
    float gradient; // range/len
    float fill; // area / (0.5 * range * len)
    float symmetry; // area relative to the area to the *right*

    unsigned clusterNo;


    void deviation(
      const unsigned v1,
      const unsigned v2,
      unsigned& issue,
      bool& flag) const;

    void deviation(
      const bool v1,
      const bool v2,
      unsigned& issue,
      bool& flag) const;

    void deviation(
      const float v1,
      const float v2,
      unsigned& issue,
      bool& flag) const;

  public:

    Peak();

    ~Peak();

    void reset();

    void logSentinel(const float valueIn);

    void log(
      const unsigned indexIn,
      const float valueIn,
      const float areaCumIn,
      const bool maxFlagIn);

    void logCluster(const unsigned cno);

    void update(
      Peak * peakPrev,
      const Peak * peakFirst,
      const Peak * peakNext);

    void annotate(
      const Peak * peakPrev,
      const Peak * peakNext);

    float distance(
      const Peak& p2,
      const Peak& scale) const;

    unsigned getIndex() const;
    bool getMaxFlag() const;
    float getValue() const;
    float getAreaCum() const;
    float getLength() const;
    float getRange() const;
    float getArea() const;
    float getArea(const Peak& p2) const;

    bool check(
      const Peak& p2,
      const unsigned offset) const;

    void operator += (const Peak& p2);

    void operator /= (const unsigned no);

    string strHeader() const;

    string str(const unsigned offset = 0) const;
};

#endif
