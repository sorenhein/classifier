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


typedef list<PeakCompletion>::iterator Miterator;


class CarCompletion
{
  private:

    list<PeakCompletion> peakCompletions;

    unsigned weight;
    unsigned distanceSquared;
    bool _forceFlag;
    list<TargetData> data;

    unsigned _limitLower;
    unsigned _limitUpper;

    list<PeakCompletion *> repairables;
    list<PeakCompletion *>::iterator itRep;


    unsigned filled() const;

    bool samePeaks(CarCompletion& carCompl2);

    void updateOverallFrom(CarCompletion& carCompl2);

    void mergeFrom(CarCompletion& carCompl2);


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

    void setData(const Target& target);

    bool operator < (const CarCompletion& comp2) const;

    void sort();

    void getMatch(
      vector<Peak const *>& closestPtrs,
      unsigned& limitLowerOut,
      unsigned& limitUpperOut);

    bool forceFlag() const;

    bool condense(CarCompletion& carCompl2); 

    void makeRepairables();

    bool nextRepairable(
      Peak& peak,
      bool& forceFlag);

    void pruneRepairables(PeakCompletion& pc);

    void makeShift();

    void calcDistanceSquared();

    string str(const unsigned offset = 0) const;
};

#endif
