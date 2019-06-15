/*
    A class to keep track of possible completions of partial cars.
 */

#ifndef TRAIN_COMPLETIONS_H
#define TRAIN_COMPLETIONS_H

#include <list>

#include "MissCar.h"

class Peak;


class Completions
{
  private:

    list<MissCar> completions;

    void sort();


  public:

    Completions();

    ~Completions();

    void reset();

    MissCar& emplace_back(const unsigned dist);

    MissCar& back();

    void markWith(
      Peak& peak,
      const MissType type);

    unsigned numComplete(MissCar *& complete);

    void condense();

    void makeRepairables();
    bool nextRepairable(Peak& peak);

    string str(const unsigned offset = 0) const;
};

#endif
