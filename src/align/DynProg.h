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


    void initNeedlemanWunsch(
      const unsigned lreff,
      const unsigned lteff,
      vector<vector<Mentry>>& matrix) const;

    void initNeedlemanWunsch2(
      const vector<float>& refPeaks,
      const vector<Peak const *>& peaks,
      const Alignment& match,
      vector<float>& penaltyRef,
      vector<float>& penaltySeen) const;

    void fillNeedlemanWunsch(
      const vector<float>& refPeaks,
      const vector<float>& scaledPeaks,
      const vector<float>& penaltyRef,
      const vector<float>& penaltySeen,
      Alignment& alignment,
      const unsigned lreff,
      const unsigned lteff,
      vector<vector<Mentry>>& matrix) const;

    void backtrackNeedlemanWunsch(
      const unsigned lreff,
      const unsigned lteff,
      const vector<vector<Mentry>>& matrix,
      Alignment& match) const;



  public:

    DynProg();

    ~DynProg();

    void run(
      const vector<float>& refPeaks,
      const PeaksInfo& peaksInfo,
      const vector<float>& scaledPeaks,
      Alignment& alignment) const;
};

#endif
