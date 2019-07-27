#ifndef TRAIN_MOTION_H
#define TRAIN_MOTION_H

#include <vector>
#include <string>

using namespace std;


struct Motion
{
  unsigned order;
  vector<double> actual;
  vector<double> estimate;


  Motion();

  ~Motion();

  void reset();

  string strLine(
    const string& text,
    const double act,
    const double est) const;

  string strEstimate(const string& title) const;

  string strBothHeader() const;

  string strBoth() const;
};

#endif
