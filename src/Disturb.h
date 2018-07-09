#ifndef TRAIN_DISTURB_H
#define TRAIN_DISTURB_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <set>

using namespace std;


struct DisturbType
{
  int peaksMin;
  int peaksMax;
  set<unsigned> peaks;
};


class Disturb
{
  private:

    int noiseSdev; // In milli-seconds
    int detectedNoiseSdev; // In milli-seconds

    DisturbType injection;
    DisturbType deletion;
    DisturbType prunedFront;
    DisturbType prunedBack;

    DisturbType detInjection;
    DisturbType detDeletion;
    DisturbType detPrunedFront;
    DisturbType detPrunedBack;

    void resetDisturbType(DisturbType& d);

    void printLine(
      ofstream& fout,
      const string& text,
      const DisturbType& actual,
      const DisturbType& detected) const;


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

    void print(const string& fname) const;
};

#endif
