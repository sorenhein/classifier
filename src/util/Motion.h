#ifndef TRAIN_MOTION_H
#define TRAIN_MOTION_H

#include <vector>
#include <string>

using namespace std;


struct Motion
{
  unsigned order;
  vector<float> actual;
  vector<float> estimate;


  Motion();

  ~Motion();

  void reset();

  void setEstimate(const vector<float>& coeffs);

  string strLine(
    const string& text,
    const float act,
    const float est) const;

  string strEstimate(const string& title) const;

  string strBothHeader() const;

  string strBoth() const;
};

#endif
