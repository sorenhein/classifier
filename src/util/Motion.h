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

  float time2pos(const float time) const;

  bool pos2time(
    const float pos,
    const float sampleRate,
    unsigned& n) const;

  string strLine(
    const string& text,
    const float act,
    const float est) const;

  string strSpeed() const;
  string strAccel() const;

  string strEstimate(const string& title) const;

  string strBothHeader() const;

  string strBoth() const;
};

#endif
