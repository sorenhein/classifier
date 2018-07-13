#ifndef TRAIN_CLUSTERS_H
#define TRAIN_CLUSTERS_H

#include <iostream>
#include <vector>

#include "struct.h"

using namespace std;


class Clusters
{
  private:

    struct ClusterEntry
    {
      int count;
      double median;
    };

    vector<Cluster> clusters;

    void logVector(
      const double * x,
      const unsigned l,
      const unsigned numClusters);

  double warp(
    vector<ClusterEntry>& newMedians,
    const Clusters& other) const;

  double removeFromBelow(
    vector<ClusterEntry>& newMedians,
    const Clusters& other,
    unsigned number) const;

  double addFromBelow(
    vector<ClusterEntry>& newMedians,
    const Clusters& other,
    unsigned number) const;

  double moveUpFrom(
    vector<ClusterEntry>& newMedians,
    const Clusters& other,
    const unsigned index,
    unsigned number) const;

  double moveDownTo(
    vector<ClusterEntry>& newMedians,
    const Clusters& other,
    const unsigned index,
    unsigned number) const;

  double balance(
    vector<ClusterEntry>& newMedians,
    const Clusters& other) const;


  public:

    Clusters();

    ~Clusters();

    void log(
      const vector<int>& axles,
      const int numClusters);

    void log(const vector<PeakTime>& times);

    const unsigned size() const;

    double distance(const Clusters& other) const;

    void print() const;
};

#endif
