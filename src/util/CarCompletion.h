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


typedef list<PeakCompletion>::iterator Miterator;


class CarCompletion
{
  private:

    list<PeakCompletion> peakCompletions;
    unsigned weight;

    vector<Peak const *> _closestPeaks;

    unsigned _limitLower;
    unsigned _limitUpper;

    list<PeakCompletion *> repairables;
    list<PeakCompletion *>::iterator itRep;


  public:

    CarCompletion();

    ~CarCompletion();

    void reset();

    void setWeight(const unsigned weightIn);

    void addMiss(
      const unsigned target,
      const unsigned tolerance);

    void addMatch(
      const unsigned target,
      Peak * pptr);

    Miterator begin();
    Miterator end();

    void addPeak(Peak& peak);

    void markWith(
      Peak& peak,
      const CompletionType type);

    bool complete() const;

    void setLimits(
      const unsigned limitLowerIn,
      const unsigned limitUpperIn);

    void getMatch(
      vector<Peak const *>*& closestPtr,
      unsigned& limitLowerOut,
      unsigned& limitUpperOut);

    bool condense(CarCompletion& carCompl2); 

    void makeRepairables();

    bool nextRepairable(Peak& peak);

    void pruneRepairables(PeakCompletion& pc);


    unsigned score() const;

    string str(const unsigned offset = 0) const;
};

#endif
