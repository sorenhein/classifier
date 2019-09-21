#ifndef TRAIN_PEAKGENERAL_H
#define TRAIN_PEAKGENERAL_H

#include <vector>

using namespace std;

class Peak;


struct PeaksInfo
{
  vector<Peak const *> peaks;
  vector<float> penaltyFactor;
  vector<float> times; // In s
  vector<float> positions; // In m
  vector<int> carNumbers; // 0+
  vector<int> peakNumbers; // 0+
  vector<int> peakNumbersInCar; // 0-3

  vector<float> points; // Like positions, but with car ends too
  vector<int> carNumbersForPoints; // -1 is a car end, otherwise 0+
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

  void mirrorFloat(
    vector<float>& v,
    const float anchor)
  {
    reverse(v.begin(), v.end());
    for (unsigned i = 0; i < v.size(); i++)
        v[i] = anchor - v[i];
  };

  void mirrorInt(
    vector<int>& v,
    const int anchor)
  {
    reverse(v.begin(), v.end());
    for (unsigned i = 0; i < v.size(); i++)
    {
      if (v[i] != -1)
        v[i] = anchor - v[i];
    }
  };

  void mirrorPeaks(vector<int>& v)
  {
    reverse(v.begin(), v.end());
    auto it = v.begin();
    while (it != v.end())
    {
      // Reverse within each car individually.
      auto it2 = next(it);
      while (it2 != v.end() && * it > * it2)
        it2++;

      // [it, it2) is now a car.
      reverse(it, it2);
      it = it2;
    }
  };

  void reverseAll()
  {
    const int numCarsMinus1 = static_cast<int>(numCars-1);

    PeaksInfo::mirrorFloat(positions, positions.back());
    PeaksInfo::mirrorInt(carNumbers, numCars-1);
    PeaksInfo::mirrorPeaks(peakNumbersInCar);

    // The first and last wheels.
    PeaksInfo::mirrorFloat(points, points[points.size()-2] - points[1]);
    PeaksInfo::mirrorInt(carNumbersForPoints, numCars-1);
    PeaksInfo::mirrorInt(peakNumbersForPoints, peakNumbers.back());
  };
};

#endif
