#ifndef TRAIN_PEAKPTRLIST_H
#define TRAIN_PEAKPTRLIST_H

#include <string>
#include <list>

#include "Peak.h"

using namespace std;


class Peak;

// Iterators for the pointer list itself.
typedef list<Peak *>::iterator PPLiterator;
typedef list<Peak *>::const_iterator PPLciterator;

// A list of iterators to the pointer list.
typedef list<PPLiterator> PeakIterListNew;

// Iterators for the iterator list.
typedef PeakIterListNew::iterator PILiterator;
typedef PeakIterListNew::const_iterator PILciterator;


class PeakPtrListNew
{
  private:

    list<Peak *> peaks;


  public:

    PeakPtrListNew();

    ~PeakPtrListNew();

    void clear();

    void push_back(Peak * peak);

    bool add(Peak * peak);

    void insert(
      PPLiterator& it,
      Peak * peak);

    PPLiterator erase(PPLiterator& it);

    void erase_below(const unsigned limit);

    PPLiterator begin();
    PPLiterator end();

    PPLciterator cbegin() const;
    PPLciterator cend() const;

    unsigned size() const;
    unsigned count(const PeakFncPtr& fptr) const;
    bool empty() const;

    unsigned firstIndex() const;
    unsigned lastIndex() const;


    PPLiterator next(
      const PPLiterator& it,
      const PeakFncPtr& fptr,
      const bool exclFlag = true);

    PPLciterator next(
      const PPLciterator& it,
      const PeakFncPtr& fptr,
      const bool exclFlag = true) const;


    PPLiterator prevExcl(
      const PPLiterator& it,
      const PeakFncPtr& fptr);

    PPLciterator prevExcl(
      const PPLciterator& it,
      const PeakFncPtr& fptr) const;


    PPLiterator prevIncl(
      const PPLiterator& it,
      const PeakFncPtr& fptr);

    PPLciterator prevIncl(
      const PPLciterator& it,
      const PeakFncPtr& fptr) const;


    void fill(
      const unsigned start,
      const unsigned end,
      vector<Peak *>& pplNew,
      list<PPLciterator>& pilNew);


    string strQuality(
      const string& text,
      const PeakFncPtr& fptr,
      const unsigned offset) const;

};

#endif
