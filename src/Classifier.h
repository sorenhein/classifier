#ifndef TRAIN_CLASSIFIER_H
#define TRAIN_CLASSIFIER_H

#include <vector>
#include <list>

#include "Database.h"
#include "struct.h"

using namespace std;


struct TrainFound
{
  int dbTrainNo;
  double distance;
};


class Classifier
{
  private:

    string country;
    int year;

    enum DiffLabel
    {
      CLUSTER_WHEEL_PAIR = 0,
      CLUSTER_INTRA_CAR = 1,
      CLUSTER_INTER_CAR = 2,
      CLUSTER_UNKNOWN = 3
    };

    struct Interval
    {
      int clusterNo;
      int tag;
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
      const vector<PeakTime>& peaks,
      vector<double>& timeDifferences) const;

    void KmeansOptimalClusters(
      const vector<double>& timeDifferences,
      vector<Cluster>& clusters) const;

    void classifyClusters(
      vector<Cluster>& clusters) const;

    void labelIntraIntervals(
      const vector<PeakTime>& peaks,
      const vector<Cluster>& clusters,
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

    void setCountry(const string& countryIn);
    void setYear(const int yearIn);

    void classify(
      const vector<PeakTime>& peaks,
      const Database& db,
      TrainFound& trainFound) const;
};

#endif

