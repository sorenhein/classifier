/*
    Statistics on residuals for a given train.
 */

#ifndef TRAIN_RESIDUALSTATS_H
#define TRAIN_RESIDUALSTATS_H

#include <vector>
#include <string>

using namespace std;

struct Alignment;


class ResidualStats
{
  private:

    vector<float> sum;
    vector<float> sumSq;
    vector<unsigned> count;
    vector<float> mean;
    vector<float> sdev;

    unsigned numCloseToZero;
    unsigned numSignificant;
    unsigned numInsignificant;
    float avg;
    float avgAbs;


  public:

    ResidualStats();

    ~ResidualStats();

    void reset();

    void log(const Alignment& alignment);

    void calculate();

    string strHeader() const;

    string str() const;

    void write(const string& filename) const;
};

#endif
