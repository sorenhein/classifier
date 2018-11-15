#ifndef TRAIN_PEAK_H
#define TRAIN_PEAK_H

#include <list>
#include <string>

#include "struct.h"

using namespace std;


enum WheelType
{
  WHEEL_LEFT = 0,
  WHEEL_RIGHT = 1,
  WHEEL_ONLY = 2,
  WHEEL_SIZE = 3
};

enum BogeyType
{
  BOGEY_LEFT = 0,
  BOGEY_RIGHT = 1,
  BOGEY_SIZE = 2
};


class Peak
{
  private:

    struct Flank
    {
      // These quantities are non-negative
      float len; // In samples
      float range;
      float area; // From the lowest of the peaks
      float gradient; // range/len
      float fill; // area / (0.5 * range * len)

      void reset()
      {
        len = 0.f; range = 0.f; area = 0.f; gradient = 0.f; fill = 0.f;
      };

      void operator += (const Flank& f2)
      {
        len += f2.len; range += f2.range; area += f2.area;
        gradient += f2.gradient; fill += f2.fill;
      };

      void operator /= (const unsigned no)
      {
        len /= no; range /= no; area /= no; gradient /= no; fill /= no;
      };
    };

    // We mainly quantify the left flank of the peaks.
    // There are dummy peaks on each end of the peak list.

    // Absolute quantities associated with the peak.
    unsigned index;
    double time;
    float value; // Signed
    float areaCum; // Integral from beginning of trace, signed
    bool maxFlag;

    // We start with the left flank and later copy in the right flank.
    // Sometimes we need the data here, so it's not enough to store
    // a pointer to the neighboring peak.

    Flank left, right;

    float rangeRatio; // Our range / next range
    float gradRatio;

    float qualityPeak;
    float qualityShape;

    bool selectFlag;
    bool seedFlag;

    bool wheelFlag;
    WheelType wheelSide;


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

    float calcQualityPeak(const Peak& scale) const;
    float calcQualityShape(const Peak& scale) const;

    float penalty(const float val) const;

  public:

    Peak();

    ~Peak();

    void reset();

    void logSentinel(
      const float valueIn,
      const bool maxFlagIn);

    void log(
      const unsigned indexIn,
      const float valueIn,
      const float areaCumIn,
      const bool maxFlagIn);

    void logNextPeak(Peak const * nextPtrIn);

    void update(const Peak * peakPrev);

    void annotate(const Peak * peakPrev);

    void calcQualities(const Peak& scale);
    void calcQualities(const list<Peak>& scales);

    float distance(
      const Peak& p2,
      const Peak& scale) const;

    float distToScaleQ(const Peak& scale) const;

    unsigned getIndex() const;
    double getTime() const;
    bool getMaxFlag() const;
    float getValue() const;
    float getAreaCum() const;
    float getRange() const;
    float getArea() const;
    float getArea(const Peak& p2) const;
    float getGradient() const;

    float getQualityPeak() const;
    float getQualityShape() const;

    bool isCandidate() const;

    void select();
    bool isSelected() const;

    void setSeed();
    void unsetSeed();
    bool isSeed() const;

    void markWheel(const WheelType wheelType);

    bool isLeftWheel() const;
    bool isRightWheel() const;

    bool greatQuality() const;

    bool similarGradient(
      const Peak& p1,
      const Peak& p2) const;

    bool check(
      const Peak& p2,
      const unsigned offset) const;

    void operator += (const Peak& p2);

    void operator /= (const unsigned no);

    string strHeader() const;
    string strHeaderSum() const;
    string strHeaderQuality() const;

    string str(const unsigned offset = 0) const;
    string strSum(const unsigned offset = 0) const;
    string strQuality(const unsigned offset = 0) const;
};

#endif
