#ifndef TRAIN_PEAKS_H
#define TRAIN_PEAKS_H

#include <list>
#include <string>

#include "Peak.h"
#include "struct.h"

using namespace std;


typedef list<Peak> PeakList;
typedef PeakList::iterator Piterator;
typedef PeakList::const_iterator Pciterator;

struct PiterPair
{
  Piterator pit;
  bool hasFlag;
};

struct Bracket
{
  PiterPair left;
  PiterPair right;
};


class Peaks
{
  private:

    PeakList peaks;


    float getFirstPeakTime(const PeakFncPtr& fptr) const;



    bool empty() const;

    unsigned size() const;

    Piterator begin();
    Pciterator cbegin() const;

    Piterator end();
    Pciterator cend() const;

    Peak& front();
    Peak& back();

    Piterator next(
      const Piterator& pit,
      const PeakFncPtr& fptr,
      const bool exclFlag = true);

    Pciterator next(
      const Piterator& pit,
      const PeakFncPtr& fptr,
      const bool exclFlag = true) const;

    Piterator prev(
      const Piterator& pit,
      const PeakFncPtr& fptr,
      const bool exclFlag = true);

    Pciterator prev(
      const Piterator& pit,
      const PeakFncPtr& fptr,
      const bool exclFlag = true) const;


    Piterator erase(
      Piterator pit1,
      Piterator pit2);

    Piterator collapse(
      Piterator pit1,
      Piterator pit2);

    void bracket(
      const unsigned pindex,
      const bool minFlag,
      Bracket& bracket);

    void bracketSpecific(
      const Bracket& bracketOther,
      const Piterator& foundIter,
      Bracket& bracket) const;

    bool near(
      const Peak& peakHint,
      const Bracket& bracket,
      Piterator& foundIter) const;

    bool getHighestMax(
      const Bracket& bracket,
      Peak *& pmax) const;

    bool getBestMax(
      const Piterator& leftIter,
      const Piterator& rightIter,
      const Piterator& pref,
      Piterator& pbest);

    bool brackets(
      const Piterator& foundIter,
      Bracket& bracketOuterMin,
      Bracket& bracketInnerMax);


    void getSamples(
      const PeakFncPtr& fptr,
      vector<float>& selected) const;

    void getTimes(
      const PeakFncPtr& fptr,
      vector<PeakTime>& times) const;


    string strTimesCSV(
      const PeakFncPtr& fptr,
      const string& text) const;

    string str(
      const string& text,
      const unsigned& offset) const;




  public:

    Peaks();

    ~Peaks();

    void clear();


};

#endif
