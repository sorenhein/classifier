/*
    A class to keep track of possible completions of partial cars.
 */

#ifndef TRAIN_COMPLETIONS_H
#define TRAIN_COMPLETIONS_H

#include <list>

#include "CarCompletion.h"


typedef list<CarCompletion>::iterator Citerator;

class Peak;


class Completions
{
  private:

    list<CarCompletion> completions;

    void sort();


  public:

    Completions();

    ~Completions();

    void reset();

    CarCompletion& emplace_back(const unsigned dist);

    CarCompletion& back();

    void markWith(
      Peak& peak,
      const MissType type);

    Citerator begin();
    Citerator end();

    unsigned numComplete(CarCompletion *& complete);

    void condense();

    void makeRepairables();

    bool nextRepairable(Peak& peak);

    string str(const unsigned offset = 0) const;
};

#endif
