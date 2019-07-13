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
#include "Target.h"


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
      const TargetData& tdata,
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

    const vector<unsigned>& indices();

    void registerPeaks(vector<Peak *> peaksClose);

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

    void setData(const Target& target);

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
