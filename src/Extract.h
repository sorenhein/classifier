#ifndef TRAIN_EXTRACT_H
#define TRAIN_EXTRACT_H

#include <vector>
#include <string>

using namespace std;


class Extract
{
  private:

    struct Run
    {
      unsigned first;
      unsigned len;
      bool posFlag;
      double cum;
    };

    vector<double> samples;

    string filename;
    unsigned firstActiveSample;
    unsigned lastTransientSample;
    double midLevel;
    double timeConstant;
    double sdev;
    double average;

    unsigned findCrossing(const double level) const;

    bool processTransient();

    bool skipTransient();

    bool calcAverage();

    void calcRuns(vector<Run>& runs) const;

    bool runsToBumps(
      const vector<Run>& runs,
      vector<Run>& bumps) const;

    void tallyBumps(
      const vector<Run>& bumps,
      unsigned& longRun,
      double& tallRun) const;


  public:

    Extract();

    ~Extract();

    bool read(const string& fname);

    void printStats() const;

};

#endif
