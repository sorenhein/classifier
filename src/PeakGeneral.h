#ifndef TRAIN_PEAKGENERAL_H
#define TRAIN_PEAKGENERAL_H

#include <vector>

using namespace std;


struct PeaksInfo
{
  vector<float> times;
  vector<float> positions;
  vector<unsigned> carNumbers;
  vector<unsigned> peakNumbers; // 0+
  vector<unsigned> peakNumbersInCar; // 0-3


  unsigned numCars; // If > 0, the next two are set and meaningful
  unsigned numPeaks;
  unsigned numFrontWheels;


  void extend(
    const int posRunning,
    const int carRunning,
    int& numberInCar)
  {
    positions.push_back(posRunning / 1000.f);
    carNumbers.push_back(carRunning);
    peakNumbersInCar.push_back(numberInCar);
    numberInCar++;
  };

  void reversePositions()
  {
    const float m = positions.back();
    reverse(positions.begin(), positions.end());
    for (unsigned i = 0; i < positions.size(); i++)
      positions[i] = m - positions[i];
  };

  void reverseCarNumbers()
  {
    const unsigned m = numCars - 1;
    reverse(carNumbers.begin(), carNumbers.end());
    for (unsigned i = 0; i < carNumbers.size(); i++)
      carNumbers[i] = m - carNumbers[i];
  };

  void reversePeakNumbersInCar()
  {
    for (auto it = peakNumbersInCar.begin(); it != peakNumbersInCar.end();
        it++)
    {
      // Reverse within each car individually.
      auto it2 = it;
      while (it2 != peakNumbersInCar.end() && * it == * it2)
        it2++;

      // [it, it2) is now a car.
      const unsigned m = * prev(it2);
      reverse(it, it2);

      for (auto it3 = it; it3 != it2; it3++)
        * it3 = m - * it3;
    }
  };
};

#endif
