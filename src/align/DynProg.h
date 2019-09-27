#ifndef TRAIN_DYNPROG_H
#define TRAIN_DYNPROG_H

#include <vector>
#include <string>

using namespace std;

struct Alignment;
struct PeaksInfo;
class Peak;


class DynProg
{
  private:

    // Used in Needleman-Wunsch.
    enum Origin
    {
      NW_MATCH = 0,
      NW_DELETE = 1,
      NW_INSERT = 2
    };

    struct Mentry
    {
      float dist;
      Origin origin;
    };


    unsigned len;

    vector<float> penaltyRef;
    vector<float> penaltySeen;
    vector<vector<Mentry>> matrix;


    void initNeedlemanWunsch(
      const vector<float>& refPeaks,
      const PeaksInfo& peaksInfo,
      const Alignment& match);

    void fillNeedlemanWunsch(
      const vector<float>& refPeaks,
      const vector<float>& scaledPeaks,
      Alignment& alignment,
      const unsigned lreff,
      const unsigned lteff);

    void backtrackNeedlemanWunsch(
      const unsigned lreff,
      const unsigned lteff,
      Alignment& match);


  public:

    DynProg();

    ~DynProg();

    void run(
      const vector<float>& refPeaks,
      const PeaksInfo& peaksInfo,
      const vector<float>& scaledPeaks,
      Alignment& alignment);
};

#endif
