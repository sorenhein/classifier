#ifndef TRAIN_DYNPROG_H
#define TRAIN_DYNPROG_H

#include <vector>
#include <string>

using namespace std;

struct Alignment;
struct PeaksInfo;
struct DynamicPenalties;


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


    unsigned lenPenaltyRef;
    unsigned lenPenaltySeen;
    unsigned lenMatrixRef;
    unsigned lenMatrixSeen;
    unsigned lenMatrixRefUsed;
    unsigned lenMatrixSeenUsed;

    vector<float> penaltyRef;
    vector<float> penaltySeen;
    vector<vector<Mentry>> matrix;


    void initNeedlemanWunsch(
      const vector<float>& seenPenaltyFactor,
      const DynamicPenalties& penalties,
      const unsigned refSize,
      const unsigned seenSize,
      const Alignment& match);

    void fillNeedlemanWunsch(
      const vector<float>& refPeaks,
      const vector<float>& scaledPeaks,
      Alignment& alignment);

    void backtrackNeedlemanWunsch(Alignment& match);


  public:

    DynProg();

    ~DynProg();

    void run(
      const vector<float>& refPeaks,
      const vector<float>& penaltyFactor,
      const vector<float>& scaledPeaks,
      const DynamicPenalties& penalties,
      Alignment& alignment);
};

#endif
