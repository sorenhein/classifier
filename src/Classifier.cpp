#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

#include "Classifier.h"
#include "Ckmeans/Ckmeans.1d.dp.h"
#include "regress/PolynomialRegression.h"

#define MIN_PEAKS 8
#define MAX_CLUSTERS 12

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))


Classifier::Classifier()
{
  // Default values.
  country = "DEU";
  year = 2018;
}


Classifier::~Classifier()
{
}


void Classifier::setCountry(const string& countryIn)
{
  country = countryIn;
}


void Classifier::setYear(const int yearIn)
{
  year = yearIn;
}


void Classifier::makeTimeDifferences(
  const vector<PeakTime>& peaks,
  vector<double>& timeDifferences) const
{
  const unsigned l = peaks.size();
  for (unsigned i = 1; i < l; i++)
    timeDifferences.push_back(peaks[i].time - peaks[i-1].time);

  sort(timeDifferences.begin(), timeDifferences.end());
}


void Classifier::KmeansOptimalClusters(
  const vector<double>& timeDifferences,
  vector<Cluster>& clusters) const
{
  const unsigned l = timeDifferences.size();

  double * x = (double *) malloc(l * sizeof(double));
  double * y = (double *) malloc(l * sizeof(double));
  int * Kcluster = (int *) malloc(l * sizeof(int));
  double * centers = (double *) malloc(l * sizeof(double));
  double * withinss = (double *) malloc(l * sizeof(double));
  double * size = (double *) malloc(l * sizeof(double));
  double * BIC = (double *) malloc(l * sizeof(double));

  for (unsigned i = 0; i < l; i++)
  {
    x[i] = timeDifferences[i];
    y[i] = 1;
    Kcluster[i] = 1;
  }

  for (unsigned c = 0; c < MAX_CLUSTERS; c++)
  {
    centers[c] = 0.;
    withinss[c] = 0.;
    size[c] = 0.;
    BIC[c] = 0.;
  }

  // kmeans_1d_dp(x, l, y, 3, MAX_CLUSTERS,
  kmeans_1d_dp(x, l, y, 3, 3,
    Kcluster, centers, withinss, size, BIC,
    "BIC", "linear", L2);
  
  const unsigned numClusters = static_cast<unsigned>(Kcluster[l-1]) + 1;
  clusters.clear();
  clusters.resize(numClusters);

  vector<unsigned> lastClusterIndex(numClusters);;
  vector<double> sum(numClusters), sumsq(numClusters);

  for (unsigned i = 0; i < l; i++)
  {
    const unsigned c = static_cast<unsigned>(Kcluster[i]);
    clusters[c].count++;
    clusters[c].upper = static_cast<int>(x[i]);

    if (clusters[c].lower == 0)
      clusters[c].lower = static_cast<int>(x[i]);
    
    lastClusterIndex[c] = i;

    const double df = timeDifferences[i];
    sum[c] += df;
    sumsq[c] += df*df;
  }

  for (unsigned c = 0; c < numClusters; c++)
  {
    clusters[c].center = centers[c];

    const double n = static_cast<double>(clusters[c].count);
    if (n > 1.f)
      clusters[c].sdev = 
        sqrt((n*sumsq[c] - sum[c]*sum[c]) / (n * (n-1.f)));
    else
      clusters[c].sdev = 0.f;

    const unsigned firstClusterIndex = 
      (c == 0 ? 0 : lastClusterIndex[c-1] + 1);

    const unsigned isum = firstClusterIndex + lastClusterIndex[c];
    const unsigned imid = isum/2;
    if (isum % 2 == 0)
      clusters[c].median = x[imid];
    else
      clusters[c].median = (x[imid] + x[imid+1]) / 2.f;
  }
}


void Classifier::classifyClusters(
  vector<Cluster>& clusters) const
{
  if (clusters.size() != 3)
  {
    // This is pretty basic for now...
    cout << "Not 3 clusters" << endl;
    return;
  }

  clusters[0].label = CLUSTER_WHEEL_PAIR; // Small
  clusters[1].label = CLUSTER_INTER_CAR; // Medium
  clusters[2].label = CLUSTER_INTRA_CAR; // Large
}


void Classifier::labelIntraIntervals(
  const vector<PeakTime>& peaks,
  const vector<Cluster>& clusters,
  vector<Interval>& intervals) const
{
  // A bit wasteful, as this was already known in Kmeans above...
  const unsigned lp = peaks.size();
  if (lp < 2)
  {
    cout << "Too few time differences" << endl;
    return;
  }

  intervals.clear();
  intervals.resize(lp-1);

  const unsigned lc = clusters.size();

  for (unsigned i = 0; i < lp-1; i++)
  {
    bool found = false;
    const double t = peaks[i+1].time - peaks[i].time;

    for (unsigned c = 0; c < lc && ! found; c++)
    {
      if (t >= clusters[c].lower && t <= clusters[c].upper)
      {
        intervals[i].clusterNo = c;
        intervals[i].label = clusters[c].label;
        found = true;
      }
    }
    
    if (! found)
    {
      cout << "Out-of-interval sample" << endl;
      return;
    }
  }
}


void Classifier::groupIntoCars(
  const vector<Interval>& intervals,
  vector<Car>& cars) const
{
  const unsigned l = intervals.size();
  int prevNonshort = -1;
  int prev = -1;

  for (unsigned i = 0; i < l; i++)
  {
    if (i == 0)
    {
      cout << i << " " << intervals[i].clusterNo << endl;
      prev = intervals[i].clusterNo;
      if (prev != CLUSTER_WHEEL_PAIR)
        prevNonshort = prev;
      continue;
    }

    cout << i << " " << intervals[i].clusterNo;

    const int c = intervals[i].clusterNo;
    if (c == CLUSTER_WHEEL_PAIR)
    {
      if (prev == CLUSTER_WHEEL_PAIR)
        cout << " two SS";

      prev = c;
    }
    else if (c == CLUSTER_INTRA_CAR)
    {
      if (prev == CLUSTER_INTRA_CAR)
        cout << " two MM";
      else if (prev == CLUSTER_INTER_CAR)
        cout << " ML";
      else if (prevNonshort == CLUSTER_INTRA_CAR)
        cout << " MSM";

      prev = c;
      prevNonshort = c;
    }
    else if (c == CLUSTER_INTER_CAR)
    {
      if (prev == CLUSTER_INTER_CAR)
        cout << " two LL";
      else if (prev == CLUSTER_INTRA_CAR)
        cout << " LM";
      else if (prevNonshort == CLUSTER_INTER_CAR)
        cout << " LSL";

      prev = c;
      prevNonshort = c;
    }
    else
    {
      cout << "Bad interval" << endl;
      return;
    }

    cout << endl;
  }

  // TODO: Group a short, an intra and a short gap into a car
  UNUSED(cars);
}


void Classifier::lookupCarTypes(
  const Database& db,
  const vector<Cluster>& clusters,
  const vector<Car>& cars,
  vector<list<int>>& carTypes) const
{
  carTypes.clear();
  carTypes.resize(cars.size());

  for (auto &car: cars)
  {
    DatabaseCar dbCar;

    dbCar.shortGap = clusters[car.clusterNoWheels].center;
    dbCar.longGap = clusters[car.clusterNoIntra].center;
    dbCar.count = car.count;
    dbCar.countInternal = car.countInternal;

    list<int> carList;
    db.lookupCar(dbCar, country, year, carList);
    carTypes.push_back(carList);
  }
}


void Classifier::classify(
  const vector<PeakTime>& peaks,
  const Database& db,
  TrainFound& trainFound) const
{
  if (peaks.size() < MIN_PEAKS)
  {
    trainFound.dbTrainNo = -1;
    trainFound.distance = 0.;
    return;
  }

  vector<double> diffs;
  Classifier::makeTimeDifferences(peaks, diffs);

  vector<Cluster> clusters;
  // Classifier::detectClusters(diffs, clusters);
  Classifier::KmeansOptimalClusters(diffs, clusters);
Classifier::printClusters(clusters);

  Classifier::classifyClusters(clusters);

  vector<Interval> intervals;
  Classifier::labelIntraIntervals(peaks, clusters, intervals);

  vector<Car> cars;
  Classifier::groupIntoCars(intervals, cars);

  vector<list<int>> carTypes;
  Classifier::lookupCarTypes(db, clusters, cars, carTypes);

  unsigned firstCar, lastCar, carsMissing;
  trainFound.dbTrainNo = 
    db.lookupTrain(carTypes, firstCar, lastCar, carsMissing);

  for (unsigned i = 0, j = firstCar;
      i < cars.size() && j <= lastCar; i++, j++)
  {
    // TODO: Add up distances somehow.
  }
}


void Classifier::printClusters(const vector<Cluster>& clusters) const
{
  for (auto &cluster: clusters)
  {
    cout << "center " << cluster.center << endl;
    cout << "median " << cluster.median << endl;
    cout << "sdev   " << cluster.sdev << endl;
    cout << "range  " << cluster.lower << " to " << cluster.upper << endl;
    cout << "count  " << cluster.count << endl;
    cout << "label  " << cluster.label << endl << endl;
  }
}

