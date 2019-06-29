/*
    A simple class to keep track of possible completions of
    partial cars.
 */

#ifndef TRAIN_TARGET_H
#define TRAIN_TARGET_H

#include <vector>
#include <list>
#include <string>

using namespace std;


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

struct TargetData
{
  unsigned modelNo;
  bool reverseFlag;
  unsigned weight;
  bool forceFlag;
};


class Target
{
  private:

    TargetData data;
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

    Target();

    ~Target();

    void reset();

    bool fill(
      const TargetData& tdata,
      const unsigned indexRangeLeft,
      const unsigned indexRangeRight,
      const BordersType bordersIn,
      const list<unsigned>& carPoints);

    void revise(
      const unsigned pos,
      const unsigned value);

    const vector<unsigned>& indices();

    unsigned index(const unsigned pos) const;

    unsigned bogieGap() const;

    void limits(
      unsigned& limitLower,
      unsigned& limitUpper) const;

    const TargetData& getData() const;


    string strIndices(
      const string& title,
      const unsigned offset) const;

    string strGaps(const string& title) const;

};

#endif
