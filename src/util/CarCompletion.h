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

    unsigned weight;
    unsigned distanceSquared;
    float qualShapeSum;
    float qualPeakSum;
    bool _forceFlag;
    list<TargetData> data;

    unsigned _limitLower;
    unsigned _limitUpper;

    list<PeakCompletion *> repairables;
    list<PeakCompletion *>::iterator itRep;


    bool samePeaks(CarCompletion& comp2);
    bool samePartialPeaks(CarCompletion& comp2);
    bool contains(CarCompletion& comp2);

    void updateOverallFrom(CarCompletion& carCompl2);

    void mergeFrom(CarCompletion& carCompl2);

    void calcDistanceSquared();


  public:

    CarCompletion();

    ~CarCompletion();

    void reset();

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
