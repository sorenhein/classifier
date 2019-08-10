/*
 * This class represents a single alignment between a trace and
 * a reference train.
 */

#ifndef TRAIN_ALIGNMENT_H
#define TRAIN_ALIGNMENT_H

#include <vector>
#include <string>

#include "../util/Motion.h"

using namespace std;


struct RegrEntry
{
  unsigned index;
  float value;
  float valueSq;
  float frac;

  bool operator < (const RegrEntry& regr2)
  {
    return (valueSq < regr2.valueSq);
  };
};


struct Alignment
{
  // Alignment is against this train (which may or may not be right).
  string trainName;
  unsigned trainNo;
  unsigned numAxles;
  unsigned numCars;

  float dist;
  float distOther;
  float distMatch;

  unsigned numAdd;
  unsigned numDelete;

  vector<int> actualToRef;

  Motion motion;

  vector<RegrEntry> residuals;

  vector<RegrEntry> topResiduals;


  Alignment();

  ~Alignment();

  void reset();

  bool operator < (const Alignment& a2) const;

  float time2pos(const float time) const;

  void setTopResiduals();

  void updateStats() const;

  string str() const;

  string strTopResiduals() const;

  string strDeviation() const;
};

#endif
