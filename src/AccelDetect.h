#ifndef TRAIN_ACCELDETECT_H
#define TRAIN_ACCELDETECT_H

#include <vector>
#include <list>
#include <string>
#include <sstream>

#include "Peaks.h"

using namespace std;

class Control;


class AccelDetect
{
  private:

    struct Extremum
    {
      unsigned index;
      float ampl;
      bool minFlag;
      unsigned origin;
      Peak const * next;

      bool operator < (const Extremum& ext2) const
      {
        if (minFlag)
          return (ampl < ext2.ampl);
        else
          return (ampl > ext2.ampl);
      };

      string str(const unsigned offset = 0) const
      {
        stringstream ss;
        ss << "(" << setw(5) << left << offset + index << ", " <<
          setw(6) << setprecision(2) << fixed << ampl << ") " <<
          (minFlag ? "min" : "max") << ", origin " << origin << endl;
        return ss.str();
      };
    };


    unsigned len;
    unsigned offset;

    Peaks peaks;

    Peak scale;


    float integrate(
      const vector<float>& samples,
      const unsigned i0,
      const unsigned i1) const;

    void annotate();

    void logFirst(const vector<float>& samples);
    void logLast(const vector<float>& samples);

    const list<Peak>::iterator collapsePeaks(
      const list<Peak>::iterator peak1,
      const list<Peak>::iterator peak2);

    void reduceSmallPeaks(
      const PeakParam param,
      const float paramLimit,
      const bool preserveKinksFlag);

    void eliminateKinks();

    void estimateScale();

    bool below(
      const bool minFlag,
      const float level,
      const float threshold) const;

    void simplifySide(
      const bool minFlag,
      const float threshold);

    void simplifySides(
      const float ampl);

    void completePeaks();

    void setExtrema(
      list<Extremum>& minima,
      list<Extremum>& maxima) const;

    void printPeak(
      const Peak& peak,
      const string& text) const;


  public:

    AccelDetect();

    ~AccelDetect();

    void reset();

    void log(
      const vector<float>& samples,
      const unsigned offsetIn,
      const unsigned lenIn);

    void extract(const string& text);

    void extractCorr(
      const float ampl,
      const string& text);

    bool getLimits(
      unsigned& lower,
      unsigned& upper,
      unsigned& spacing) const;

    string printPeaks() const;

    void printPeaksCSV(const vector<double>& timesTrue) const;

    void writePeak(
      const Control& control,
      const unsigned first,
      const string& filename) const;

    Peaks& getPeaks();

};

#endif
