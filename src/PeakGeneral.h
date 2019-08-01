#ifndef TRAIN_PEAKGENERAL_H
#define TRAIN_PEAKGENERAL_H

#include <vector>

using namespace std;


struct PeakInfo
{
  float time;
  unsigned carNo;
  unsigned peakNo; // 0-
  unsigned peakNoInCar; // 0-3
};


struct PeaksInfo
{
  vector<PeakInfo> infos;

  unsigned numCars; // If > 0, the next two are set and meaningful
  unsigned numPeaks;
  unsigned numFrontWheels;
};



#endif
