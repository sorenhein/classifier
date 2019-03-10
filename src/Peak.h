#ifndef TRAIN_PEAK_H
#define TRAIN_PEAK_H

#include <string>
#include <vector>

#include "struct.h"

using namespace std;


class Peak;
typedef void (Peak::*PeakRunFncPtr)();
typedef bool (Peak::*PeakFncPtr)() const;
typedef vector<Peak *> PeakPtrVector;


class Peak
{
  private:

    struct Flank
    {
      // These quantities are non-negative:
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

    // It can happen that a peak is quite shallow.  In that case we
    // note also the "extent" of the peak, as this influences the
    // gradients.

    unsigned indexLeft;
    unsigned indexRight;
    float valueLeft;
    float valueRight;

    // We start with the left flank and later copy in the right flank.
    // Sometimes we need the data here, so it's not enough to store
    // a pointer to the neighboring peak.

    Flank left, right;

    float rangeRatio; // Our range / next range
    float gradRatio;

    float qualityPeak;
    float qualityShape;

    bool selectFlag;

    bool wheelFlag;
    WheelType wheelSide;
    BogieType bogieSide;


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

    string strExtent() const;

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

    void logExtent(
      const Peak& prevPeak,
      const Peak& nextPeak);

    void logPosition(
      const unsigned indexIn,
      const unsigned indexLeftIn,
      const unsigned indexRightIn);

    void update(const Peak * peakPrev);

    void annotate(const Peak * peakPrev);

    void calcQualities(const Peak& scale);
    void calcQualities(const vector<Peak>& scales);

    unsigned calcQualityPeak(const vector<Peak>& scales);

    unsigned getIndex() const;
    double getTime() const;
    bool getMaxFlag() const;
    bool isMinimum() const;
    float getValue() const;
    float getAreaCum() const;
    float getRange() const;
    float getArea() const;
    float getArea(const Peak& p2) const;

    float getParameter(const PeakParam param) const;
    float getParameter(
      const Peak& p2,
      const PeakParam param) const;

    float getQualityShape() const; // TODO Delete later

    bool isCandidate() const;

    bool fits(const Peak& peak2) const;

    void select();
    void unselect();
    bool isSelected() const;

    bool always() const;

    void markWheel(const WheelType wheelType);
    void markNoWheel();
    bool isLeftWheel() const;
    bool isRightWheel() const;
    bool isWheel() const;

    void markdown();

    void markBogie(const BogieType bogieType);
    void markNoBogie();
    bool isLeftBogie() const;
    bool isRightBogie() const;
    bool isBogie() const;

    bool fitsType(
      const BogieType bogieType,
      const WheelType wheelType) const;

    void markBogieAndWheel(
      const BogieType bogieType,
      const WheelType wheelType);

    bool fantasticQuality() const;
    bool greatQuality() const;
    bool goodQuality() const;
    bool acceptableQuality() const;

    bool goodPeakQuality() const;

    bool similarGradientBest(const Peak& p1) const;
    bool similarGradientForward(const Peak& p1) const;
    bool similarGradientBackward(const Peak& p1) const;

    float matchMeasure(const Peak& p1) const;

    bool similarGradientTwo(
      const Peak& p1,
      const Peak& p2) const;

    bool matchesGap(
      const Peak& peakNext,
      const Gap& gap) const;

    bool check(
      const Peak& p2,
      const unsigned offset) const;

    void operator += (const Peak& p2);

    void operator /= (const unsigned no);

    string strHeader() const;
    string strHeaderQuality() const;

    string str(const unsigned offset = 0) const;
    string strQuality(const unsigned offset = 0) const;
    string strQualityWhole(
      const string& text,
      const unsigned offset) const;

    string stars() const;
};

#endif
