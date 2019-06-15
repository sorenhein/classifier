/*
    A simple class to keep track of missing, sought-after peaks
    that collectively would complete a car.
 */

#ifndef TRAIN_MISSCAR_H
#define TRAIN_MISSCAR_H

#include <list>
#include <string>

#include "MissPeak.h"


typedef list<MissPeak>::iterator Miterator;


class MissCar
{
  private:

    list<MissPeak> misses;
    unsigned weight;


  public:

    MissCar();

    ~MissCar();

    void reset();

    void setWeight(const unsigned weightIn);

    void add(
      const unsigned target,
      const unsigned tolerance);

    Miterator begin();
    Miterator end();

    void markWith(
      Peak * peak,
      const MissType type);

    bool complete() const;

    bool condense(MissCar& miss2); 

    unsigned score() const;

    void repairables(list<MissPeak *>& repairList);

    string str() const;
};

#endif
