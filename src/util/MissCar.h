/*
    A simple class to keep track of missing, sought-after peaks
    that collectively would complete a car.
 */

#ifndef TRAIN_MISSCAR_H
#define TRAIN_MISSCAR_H

#include <list>
#include <vector>
#include <string>

#include "MissPeak.h"


typedef list<MissPeak>::iterator Miterator;


class MissCar
{
  private:

    list<MissPeak> misses;
    unsigned weight;
    unsigned distance;

    vector<Peak const *> _closestPeaks;

    unsigned _limitLower;
    unsigned _limitUpper;

    list<MissPeak *> repairables;
    list<MissPeak *>::iterator itRep;


  public:

    MissCar();

    ~MissCar();

    void reset();

    void setWeight(const unsigned weightIn);

    void setDistance(const unsigned dist);

    void add(
      const unsigned target,
      const unsigned tolerance);

    Miterator begin();
    Miterator end();

    void addPeak(Peak& peak);

    void markWith(
      Peak& peak,
      const MissType type);

    bool complete() const;

    void setMatch(
      vector<Peak const *>& closestPeaksIn,
      const unsigned limitLowerIn,
      const unsigned limitUpperIn);

    void getMatch(
      vector<Peak const *>*& closestPtr,
      unsigned& limitLowerOut,
      unsigned& limitUpperOut);

    bool condense(MissCar& miss2); 

    void makeRepairables();

    bool nextRepairable(Peak& peak);

    void pruneRepairables(MissPeak& miss);


    unsigned score() const;

    string str(const unsigned offset = 0) const;
};

#endif
