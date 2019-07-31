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

    float time2pos(
      const float& time,
      const vector<float>& coeffs) const;


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
};

#endif
