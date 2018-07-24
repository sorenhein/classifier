#ifndef TRAIN_EXTRACT_H
#define TRAIN_EXTRACT_H

#include <vector>
#include <string>

using namespace std;


class Extract
{
  private:

    vector<double> samples;

    string filename;
    unsigned firstActiveSample;
    unsigned lastTransientSample;
    double midLevel;
    double timeConstant;
    double sdev;

    unsigned findCrossing(const double level) const;

    bool processTransient();

  public:

    Extract();

    ~Extract();

    bool read(const string& fname);

    void printStats() const;

};

#endif
