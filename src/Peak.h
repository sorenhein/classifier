#ifndef TRAIN_PEAK_H
#define TRAIN_PEAK_H

#include <vector>
#include <string>

#include "struct.h"

using namespace std;


class Peak
{
  private:

    // We mainly quantify the left flank of the peaks.
    // There are dummy peaks on each end of the peak list.

    // Absolute quantities associated with the peak.
    unsigned index;
    double time;
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

    float measureVal;
    bool measureFlag;

    unsigned clusterNo;
    bool selectFlag;
    int match; // Number of matched peak in real list (or -1)
    PeakType ptype;

    // TODO Maybe tmp
    unsigned indexZC; // Location of *next* zero-crossing
    float valueZC; // Size of *own* maximum within this zero-crossing
    bool quietBegin;
    bool quietEnd;
    bool sharpFlag;

    const Peak * origPtr; // Pointer to origin in more complete peak list


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

    void logCluster(const unsigned cno);
    void logType(PeakType ptypeIn);
    void logMatch(const int m);
    void logZeroCrossing(
      const unsigned indexIn,
      const float valueIn);

    void logOrigPointer(const Peak * const);

    void logSharp();

    void logQuietBegin();
    void logQuietEnd();

    void update(
      Peak * peakPrev,
      const Peak * peakNext);

    void annotate(
      const Peak * peakPrev,
      const Peak * peakNext);

    float distance(
      const Peak& p2,
      const Peak& scale) const;

    unsigned getIndex() const;
    double getTime() const;
    bool getMaxFlag() const;
    float getValue() const;
    float getAreaCum() const;
    float getLength() const;
    float getRange() const;
    float getArea() const;
    float getArea(const Peak& p2) const;
    float getFill() const;
    float getGradient() const;
    float getSymmetry() const;
    unsigned getCluster() const;
    int getMatch() const;
    PeakType getType() const;
    float getValueZC() const;
    unsigned getIndexNextZC() const;
    const Peak * const getOrigPointer() const;
    bool getQuietBegin() const;
    bool getQuietEnd() const;
    bool getSharp() const;

    bool isCluster(const unsigned cno) const;

    bool isCandidate() const;

    float measure(const Peak& scale);
    float measure() const;

    void select();
    bool isSelected() const;

    bool check(
      const Peak& p2,
      const unsigned offset) const;

    void operator += (const Peak& p2);

    void operator /= (const unsigned no);

    string strHeader() const;

    string str(const unsigned offset = 0) const;
};

#endif
