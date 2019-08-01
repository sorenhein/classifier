#ifndef TRAIN_REGRESS_H
#define TRAIN_REGRESS_H

#include <vector>
#include <string>

using namespace std;

class Control;
class TrainDB;
struct Alignment;


class Regress
{
  private:
    
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
      Alignment& match) const;

    void bestMatch(
      const TrainDB& trainDB,
      const vector<float>& times,
      vector<Alignment>& matches) const;

    string str(
      const Control& control,
      const vector<Alignment>& matches) const;

    string strMatchingResiduals(
      const string& trainTrue,
      const string& pickAny,
      const string& heading,
      const vector<Alignment>& matches) const;
};

#endif
