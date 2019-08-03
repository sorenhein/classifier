/*
    A class to keep track of possible completions of partial cars.
 */

#ifndef TRAIN_COMPLETIONS_H
#define TRAIN_COMPLETIONS_H

#include <list>

#include "Completion.h"


typedef list<Completion>::iterator Citerator;

class Peak;


class Completions
{
  private:

    list<Completion> completions;


  public:

    Completions();

    ~Completions();

    void reset();

    Completion& emplace_back();

    Completion& back();

    void pop_back();

    void markWith(
      Peak& peak,
      const CompletionType type,
      const bool forceFlag);

    Citerator begin();
    Citerator end();

    bool empty() const;

    unsigned numComplete(Completion *& complete);

    unsigned numPartial(
      Completion *& partial,
      BordersType& borders);

    bool effectivelyEmpty() const;

    void makeShift();

    void calcMetrics();

    void condense();

    void sort();

    void makeRepairables();

    bool nextRepairable(
      Peak& peak,
      bool& forceFlag);

    string str(const unsigned offset = 0) const;
};

#endif
