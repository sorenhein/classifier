#ifndef TRAIN_CLUSTERS_H
#define TRAIN_CLUSTERS_H

#include <iostream>
#include <vector>

#include "struct.h"

using namespace std;

class Database;


class Clusters
{
  private:

    struct ClusterEntry
    {
      int count;
      double median;
    };

    vector<Cluster> clusters;

    double logVector(
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

    double distance(const Clusters& other) const;

    const unsigned size() const;


  public:

    Clusters();

    ~Clusters();

    double log(
      const vector<int>& axles,
      const int numClusters);

    double log(
      const vector<PeakTime>& times,
      const int numClusters = 0);

    void bestMatches(
      const vector<PeakTime>& times,
      Database& db,
      const int trainNo,
      const unsigned maxTops,
      vector<int>& matches);

    void print() const;
};

#endif
