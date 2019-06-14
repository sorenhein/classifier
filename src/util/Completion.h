/*
    A simple class to keep track of possible completions of
    partial cars.
 */

#ifndef TRAIN_COMPLETION_H
#define TRAIN_COMPLETION_H

#include <vector>
#include <list>
#include <string>

using namespace std;


class Completion
{
  private:

    enum BordersType
    {
      BORDERS_NONE = 0,
      BORDERS_SINGLE_SIDED_LEFT = 1,
      BORDERS_SINGLE_SIDED_RIGHT = 2,
      BORDERS_DOUBLE_SIDED_SINGLE = 3,
      BORDERS_DOUBLE_SIDED_SINGLE_SHORT = 4,
      BORDERS_DOUBLE_SIDED_DOUBLE = 5,
      BORDERS_SIZE = 6
    };

    unsigned modelNo;
    bool reverseFlag;
    bool abutLeftFlag;
    bool abutRightFlag;
    unsigned start;
    unsigned end;
    BordersType borders;
    vector<unsigned> _indices;


    bool fillPoints(
      const list<unsigned>& carPoints,
      const unsigned indexBase);


  public:

    Completion();

    ~Completion();

    void reset();

    bool fill(
      const unsigned modelNoIn,
      const bool reverseFlagIn,
      const unsigned indexRangeLeft,
      const unsigned indexRangeRight,
      const BordersType bordersIn,
      const list<unsigned>& carPoints);

    const vector<unsigned>& indices();

    void limits(
      unsigned& limitLower,
      unsigned& limitUpper) const;


    string strIndices(
      const string& title,
      const unsigned offset) const;

    string strGaps(const string& title) const;

};

#endif
