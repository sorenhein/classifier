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

enum RangeType
{
  RANGE_UNBOUNDED = 0,
  RANGE_BOUNDED_LEFT = 1,
  RANGE_BOUNDED_RIGHT = 2,
  RANGE_BOUNDED_BOTH = 3
};

struct TargetData
{
  unsigned modelNo;
  bool reverseFlag;
  unsigned weight;
  bool forceFlag;
  BordersType borders;
  RangeType range;
  unsigned distanceSquared;

  bool operator < (TargetData& tdata2) const
  {
    if (modelNo < tdata2.modelNo)
      return true;
    else if (modelNo > tdata2.modelNo)
      return false;
    else if (reverseFlag == tdata2.reverseFlag)
      return true; // To have something
    else
      return (! reverseFlag);
  };

  string strModel() const
  {
    return to_string(modelNo) + (reverseFlag ? "R" : "");
  };

  string strSource() const
  {
    string s = " ";
    if (borders == BORDERS_NONE)
      s += "none";
    else if (borders == BORDERS_SINGLE_SIDED_LEFT)
      s += "left";
    else if (borders == BORDERS_SINGLE_SIDED_RIGHT)
      s += "right";
    else if (borders == BORDERS_DOUBLE_SIDED_SINGLE)
      s += "single";
    else if (borders == BORDERS_DOUBLE_SIDED_SINGLE_SHORT)
      s += "single-short";
    else if (borders == BORDERS_DOUBLE_SIDED_DOUBLE)
      s += "single-double";
    
    s += " (" + to_string(weight) + ", dsq " + 
      to_string(distanceSquared) + ")";
    return s;
  };
};


class Target
{
  private:

    TargetData data;
    bool abutLeftFlag;
    bool abutRightFlag;
    unsigned start;
    unsigned end;
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
      const RangeType rangeIn,
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
