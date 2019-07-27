#ifndef TRAIN_ALIGNMENT_H
#define TRAIN_ALIGNMENT_H

#include <vector>

using namespace std;


struct RegrEntry
{
  unsigned index;
  double value;
  double valueSq;
  double frac;

  bool operator < (const RegrEntry& regr2)
  {
    return (valueSq < regr2.valueSq);
  };
};


struct Alignment
{
  unsigned trainNo;

  double dist;
  double distMatch;

  unsigned numAdd;
  unsigned numDelete;
  
  vector<int> actualToRef;

  vector<RegrEntry> topResiduals;


  Alignment();

  ~Alignment();

  void reset();

  bool operator < (const Alignment& a2) const;
};

#endif