#ifndef TRAIN_PEAKGENERAL_H
#define TRAIN_PEAKGENERAL_H

#include <vector>

using namespace std;


struct PeaksInfo
{
  vector<float> times; // In s
  vector<float> positions; // In m
  vector<unsigned> carNumbers;
  vector<unsigned> peakNumbers; // 0+
  vector<unsigned> peakNumbersInCar; // 0-3

  vector<float> points; // Like positions, but with car ends too
  vector<int> carNumbersForPoints; // -1 is a car end
  vector<int> peakNumbersForPoints; // -1 is a car end, otherwise 0+


  unsigned numCars; // If > 0, the next two are set and meaningful
  unsigned numPeaks;
  unsigned numFrontWheels;


  void extend(
    const int posRunning,
    const int carRunning,
    int& numberOverall,
    int& numberInCar)
  {
    positions.push_back(posRunning / 1000.f);
    carNumbers.push_back(carRunning);
    peakNumbers.push_back(numberOverall);
    peakNumbersInCar.push_back(numberInCar);

    PeaksInfo::extendPoints(posRunning, carRunning, numberOverall);

    numberInCar++;
    numberOverall++;
  };

  void extendPoints(
    const int posRunning,
    const int carRunning = -1,
    const int numberOverall = -1)
  {
    // Extend by a boundary point without a peak.
    points.push_back(posRunning / 1000.f);
    carNumbersForPoints.push_back(carRunning);
    peakNumbersForPoints.push_back(numberOverall);
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
