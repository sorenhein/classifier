#ifndef TRAIN_PEAKGENERAL_H
#define TRAIN_PEAKGENERAL_H

#include <vector>

using namespace std;


struct PeaksInfo
{
  vector<float> times;
  vector<unsigned> carNumbers;
  vector<unsigned> peakNumbers; // 0+
  vector<unsigned> peakNumbersInCar; // 0-3


  unsigned numCars; // If > 0, the next two are set and meaningful
  unsigned numPeaks;
  unsigned numFrontWheels;
};



#endif
