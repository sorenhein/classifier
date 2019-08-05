#ifndef TRAIN_REGRESS_H
#define TRAIN_REGRESS_H

#include <vector>
#include <string>

#include "../align/Alignment.h"

using namespace std;

class Control;
class TrainDB;


class Regress
{
  private:

    vector<Alignment> matches;

    
    void storeResiduals(
      const vector<float>& x,
      const vector<float>& y,
      const unsigned lt,
      const float peakScale,
      Alignment& match) const;

  public:

    Regress();

    ~Regress();

    void specificMatch(
      const vector<float>& times,
      const vector<float>& refPeaks,
      const bool storeFlag,
      Alignment& match) const;

    void bestMatch(
      const TrainDB& trainDB,
      const vector<float>& times);

    vector<Alignment>& getMatches();

    void getBest(
      const unsigned& trainNoTrue,
      string& trainDetected,
      float& distDetected,
      unsigned& rankDetected) const;

    string strMatches(const string& title) const;

    string str(const Control& control) const;

    string strMatchingResiduals(
      const string& trainTrue,
      const string& pickAny,
      const string& heading) const;
};

#endif
