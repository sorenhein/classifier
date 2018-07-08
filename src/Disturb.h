#ifndef TRAIN_DISTURB_H
#define TRAIN_DISTURB_H

#include <set>

using namespace std;


class Disturb
{
  private:

    int noiseSdev; // In milli-seconds

    int injectPeaksMin;
    int injectPeaksMax;
    set<unsigned> injectedPeaks;

    int deletePeaksMin;
    int deletePeaksMax;
    set<unsigned> deletedPeaks;

    int pruneFrontMin;
    int pruneFrontMax;
    set<unsigned> prunedFronts;

    int pruneBackMin;
    int pruneBackMax;
    set<unsigned> prunedBacks;

    set<unsigned> detectedInjectedPeaks;
    set<unsigned> detectedDeletedPeaks;
    set<unsigned> detectedPrunedFronts;
    set<unsigned> detectedPrunedBacks;


  public:

    Disturb();

    ~Disturb();

    bool readFile(const string& fname);

    int getNoiseSdev() const;

    void getInjectRange(
      int& injectMin,
      int& injectMax) const;
      
    void getDeleteRange(
      int& deleteMin,
      int& deleteMax) const;
      
    void getFrontRange(
      int& frontMin,
      int& frontMax) const;
      
    void getBackRange(
      int& backMin,
      int& backMax) const;

    void setInject(const unsigned pos);

    void setDelete(const unsigned pos);

    void setPruneFront(const unsigned number);

    void setPruneBack(const unsigned number);

    void detectInject(const unsigned pos);

    void detectDelete(const unsigned pos);

    void detectFront(const unsigned number);

    void detectBack(const unsigned number);
};

#endif
