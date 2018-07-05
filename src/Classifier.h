#ifndef TRAIN_CLASSIFIER_H
#define TRAIN_CLASSIFIER_H

#include <vector>
#include <list>

#include "Database.h"
#include "const.h"

using namespace std;


struct TrainFound
{
  int dbTrainNo;
  float distance;
};


class Classifier
{
  private:

    int hertz;
    string country;
    int year;

    enum DiffLabel
    {
      CLUSTER_WHEEL_PAIR = 0,
      CLUSTER_INTRA_CAR = 1,
      CLUSTER_INTER_CAR = 2,
      CLUSTER_UNKNOWN = 3
    };

    struct Cluster
    {
      float center;
      float sdev;
      int lower;
      int upper;
      int count;
      DiffLabel label;

      bool operator < (const Cluster& c2)
      {
        return (count < c2.count);
      }
    };

    struct Interval
    {
      int sampleFirst;
      float valueFirst;
      int sampleSecond;
      float valueSecond;
      int clusterNo;
      int carNo; 
      // Real car numbers start from 0.
      // -1: inter-car gap
      // -2: Unidentified gap
    };

    struct Car
    {
      int clusterNoWheels;
      int clusterNoIntra;
      int count;
      int countInternal;
      // Excludes end cars
    };

    void makeTimeDifferences(
      const vector<Peak>& peaks,
      vector<int>& timeDifferences) const;

    void KmeansOptimalClusters(
      const vector<int>& timeDifferences,
      vector<Cluster>& clusters) const;

    void detectClusters(
      const vector<int>& timeDifferences,
      vector<Cluster>& clusters) const;

    void classifyClusters(
      const vector<Cluster>& clusters) const;

    void labelIntraIntervals(
      const vector<Peak>& peaks,
      vector<Interval>& intervals) const;

    void groupIntoCars(
      const vector<Interval>& intervals,
      vector<Car>& cars) const;

    void lookupCarTypes(
      const Database& db,
      const vector<Cluster>& clusters,
      const vector<Car>& cars,
      vector<list<int>>& carTypes) const;

    void printClusters(
      const vector<Cluster>& clusters) const;


  public:

    Classifier();

    ~Classifier();

    void setSampleRate(const int hertzIn);
    void setCountry(const string& countryIn);
    void setYear(const int yearIn);

    void classify(
      const vector<Peak>& peaks,
      const Database& db,
      TrainFound& trainFound) const;
};

#endif

