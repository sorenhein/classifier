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
      unsigned count;
      double median;
    };

    vector<Cluster> clusters;

    double logVector(
      const vector<double>& x,
      const unsigned l,
      const unsigned numClusters);

    void warp(
      vector<ClusterEntry>& newMedians,
      const Clusters& other,
      HistWarp& histwarp) const;

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

    void distance(
      const Clusters& other,
      HistWarp& histwarp) const;

    unsigned size() const;


  public:

    Clusters();

    ~Clusters();

    double log(
      const vector<int>& axles,
      const unsigned numClusters);

    double log(
      const vector<PeakTime>& times,
      const unsigned numClusters = 0);

    void getShorty(Cluster& cluster) const;

    void bestMatches(
      const vector<PeakTime>& times,
      Database& db,
      const unsigned trainNo,
      const unsigned maxTops,
      vector<HistMatch>& matches);

    void print() const;
};

#endif
