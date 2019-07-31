#ifndef TRAIN_ALIGNMENT_H
#define TRAIN_ALIGNMENT_H

#include <vector>
#include <string>

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
  string trainName;
  unsigned trainNo;

  float dist;
  float distMatch;

  unsigned numAdd;
  unsigned numDelete;
  
  vector<int> actualToRef;

  vector<RegrEntry> topResiduals;


  Alignment();

  ~Alignment();

  void reset();

  bool operator < (const Alignment& a2) const;

  string str() const;

  string strTopResiduals() const;
};

#endif
