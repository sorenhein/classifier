/*
    A simple class to keep track of peaks that collectively would
    complete a car.
 */

#ifndef TRAIN_CARCOMPLETION_H
#define TRAIN_CARCOMPLETION_H

#include <list>
#include <vector>
#include <string>

#include "PeakCompletion.h"


enum BordersType
{
  BORDERS_NONE = 0,
  BORDERS_SINGLE_SIDED_LEFT = 1,
  BORDERS_SINGLE_SIDED_RIGHT = 2,
  BORDERS_DOUBLE_SIDED_SINGLE = 3,
  BORDERS_DOUBLE_SIDED_SINGLE_SHORT = 4,
  BORDERS_DOUBLE_SIDED_DOUBLE = 5,
  BORDERS_SIZE = 6
};

enum RangeType
{
  RANGE_UNBOUNDED = 0,
  RANGE_BOUNDED_LEFT = 1,
  RANGE_BOUNDED_RIGHT = 2,
  RANGE_BOUNDED_BOTH = 3
};

struct TargetData
{
  unsigned modelNo;
  bool reverseFlag;
  unsigned weight;
  bool forceFlag;
  BordersType borders;
  RangeType range;
  unsigned distanceSquared;

  bool operator < (TargetData& tdata2) const
  {
    if (modelNo < tdata2.modelNo)
      return true;
    else if (modelNo > tdata2.modelNo)
      return false;
    else if (reverseFlag == tdata2.reverseFlag)
      return true; // To have something
    else
      return (! reverseFlag);
  };

  string strModel() const
  {
    return to_string(modelNo) + (reverseFlag ? "R" : "");
  };

  string strSource() const
  {
    string s = " ";
    if (borders == BORDERS_NONE)
      s += "none";
    else if (borders == BORDERS_SINGLE_SIDED_LEFT)
      s += "left";
    else if (borders == BORDERS_SINGLE_SIDED_RIGHT)
      s += "right";
    else if (borders == BORDERS_DOUBLE_SIDED_SINGLE)
      s += "single";
    else if (borders == BORDERS_DOUBLE_SIDED_SINGLE_SHORT)
      s += "single-short";
    else if (borders == BORDERS_DOUBLE_SIDED_DOUBLE)
      s += "single-double";
    
    s += " (" + to_string(weight) + ", dsq " + 
      to_string(distanceSquared) + ")";
    return s;
  };
};

enum CondenseType
{
  CONDENSE_SAME = 0,
  CONDENSE_BETTER = 1,
  CONDENSE_WORSE = 2,
  CONDENSE_DIFFERENT = 3
};

typedef list<PeakCompletion>::iterator Miterator;


class CarCompletion
{
  private:

    list<PeakCompletion> peakCompletions;

    // Related to target.
    list<TargetData> data;
    unsigned limitLower;
    unsigned limitUpper;
    bool abutLeftFlag;
    bool abutRightFlag;
    unsigned _start;
    unsigned _end;
    vector<unsigned> _indices;

    // Related to completion.
    unsigned weight;
    unsigned distanceSquared;
    float qualShapeSum;
    float qualPeakSum;
    bool _forceFlag;


    list<PeakCompletion *> repairables;
    list<PeakCompletion *>::iterator itRep;


    bool fillPoints(
      const bool reverseFlag,
      const list<unsigned>& carPoints,
      const unsigned indexBase);

    void setLimits(const TargetData& tdata);

    bool samePeaks(CarCompletion& comp2);
    bool samePartialPeaks(CarCompletion& comp2);
    bool contains(CarCompletion& comp2);
    
    bool dominates(
      const float dRatio,
      const float qsRatio,
      const float qpRatio,
      const unsigned dSq) const;

    void combineSameCount(CarCompletion& carCompl2);

    void updateOverallFrom(CarCompletion& carCompl2);

    void mergeFrom(CarCompletion& carCompl2);

    void calcDistanceSquared();


  public:

    CarCompletion();

    ~CarCompletion();

    void reset();

    bool fill(
      const TargetData& tdata,
      const unsigned indexRangeLeft,
      const unsigned indexRangeRight,
      const list<unsigned>& carPoints);

    void registerPeaks(vector<Peak *>& peaksClose);

    const vector<unsigned>& indices();

    void addMiss(
      const unsigned target,
      const unsigned tolerance);

    void addMatch(
      const unsigned target,
      Peak * pptr);

    Miterator begin();
    Miterator end();

    void markWith(
      Peak& peak,
      const CompletionType type,
      const bool forceFlag);

    bool complete() const;
    bool partial() const;

    bool operator < (const CarCompletion& comp2) const;

    void sort();

    void getMatch(
      vector<Peak const *>& closestPtrs,
      unsigned& limitLowerOut,
      unsigned& limitUpperOut);

    bool forceFlag() const;

    BordersType bestBorders() const;

    unsigned filled() const;

    CondenseType condense(CarCompletion& carCompl2); 

    void makeRepairables();

    bool nextRepairable(
      Peak& peak,
      bool& forceFlag);

    void pruneRepairables(PeakCompletion& pc);

    void makeShift();

    void calcMetrics();

    string str(const unsigned offset = 0) const;
};

#endif
