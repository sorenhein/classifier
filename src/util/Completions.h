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


  public:

    Completions();

    ~Completions();

    void reset();

    CarCompletion& emplace_back();

    CarCompletion& back();

    void markWith(
      Peak& peak,
      const CompletionType type);

    Citerator begin();
    Citerator end();

    unsigned numComplete(CarCompletion *& complete);

    void makeShift();

    void condense();

    void makeRepairables();

    bool nextRepairable(Peak& peak);

    string str(const unsigned offset = 0) const;
};

#endif
