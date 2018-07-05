#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

#include "Classifier.h"
#include "Ckmeans/Ckmeans.1d.dp.h"

#define MIN_PEAKS 8
#define CLUSTER_FACTOR 1.05
#define MAX_CLUSTERS 12

#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))

      // If we ever need sdev:
      // float sum = 0.;
      // float sumsq = 0;

      // for (unsigned j = jlo; j <= jhi; j++)
      // {
        // const float df = static_cast<float>(timeDifferences[j]);
        // sum += df;
        // sumsq += df * df;
      // }

       // const float n = static_cast<float>(count);
       // cluster.center = sum / n;
       // cluster.sdev = sqrt((n*sumsq - sum*sum) / (n * (n-1.f)));



Classifier::Classifier()
{
  // Default values.
  hertz = 2000;
  country = "DEU";
  year = 2018;
}


Classifier::~Classifier()
{
}


void Classifier::makeTimeDifferences(
  const vector<Peak>& peaks,
  vector<int>& timeDifferences) const
{
  const unsigned l = peaks.size();
  for (unsigned i = 1; i < l; i++)
    timeDifferences.push_back(peaks[i].sampleNo - peaks[i-1].sampleNo);

  sort(timeDifferences.begin(), timeDifferences.end());
}


void Classifier::KmeansOptimalClusters(
  const vector<int>& timeDifferences,
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

  kmeans_1d_dp(x, l, y, 3, MAX_CLUSTERS,
    Kcluster, centers, withinss, size, BIC,
    "BIC", "linear", L2);
  
  const unsigned numClusters = static_cast<unsigned>(Kcluster[l-1]) + 1;
  clusters.clear();
  clusters.resize(numClusters);

  vector<unsigned> lastClusterIndex(numClusters);;

  for (unsigned i = 0; i < l; i++)
  {
    const unsigned c = static_cast<unsigned>(Kcluster[i]);
    clusters[c].count++;
    clusters[c].upper = static_cast<int>(x[i]);

    if (clusters[c].lower == 0)
      clusters[c].lower = static_cast<int>(x[i]);
    
    lastClusterIndex[c] = i;
  }

  for (unsigned c = 0; c < numClusters; c++)
  {
    clusters[c].center = static_cast<float>(centers[c]);
    clusters[c].sdev = static_cast<float>(sqrt(withinss[c]));

    const unsigned firstClusterIndex = 
      (c == 0 ? 0 : lastClusterIndex[c-1] + 1);

    const unsigned isum = firstClusterIndex + lastClusterIndex[c];
    const unsigned imid = isum/2;
    if (isum % 2 == 0)
      clusters[c].median = static_cast<float>(x[imid]);
    else
      clusters[c].median = static_cast<float>(x[imid] + x[imid+1]) / 2.f;
  }
}


bool Classifier::linearRegression(
  const vector<Peak>& observedTimes,
  const vector<Peak>& synthTimes,
  float& scaleFactor,
  int& timeShift) const
{
  // x ~ scaleFactor * y + timeShift, where 
  // x is observedTimes
  // y is synthTimes
  // Effectively a linear regression

  const unsigned l = observedTimes.size();
  const float lf = static_cast<float>(l);

  if (synthTimes.size() != l || l < 2)
    return false;

  float sumx = 0.f;
  float sumy = 0.f;
  float sumxy = 0.f;
  float sumyy = 0.f;

  for (unsigned i = 0; i< l; i++)
  {
    const float x = static_cast<float>(observedTimes[i].sampleNo);
    const float y = static_cast<float>(synthTimes[i].sampleNo);

    sumx += x;
    sumy += y;
    sumxy += x*y;
    sumyy += y*y;
  }

  const float avgx = sumx / lf;
  const float avgy = sumy / lf;
  const float avgxy = sumxy / lf;
  const float avgyy = sumyy / lf;

  if (abs(avgyy - avgy * avgy) < 1.f)
    return false;

  scaleFactor = (avgx * avgy - avgxy) / (avgy * avgy - avgyy);
  timeShift = static_cast<int>(avgx - scaleFactor * avgy);
  return true;
}


void Classifier::classifyClusters(
  const vector<Cluster>& clusters) const
{
  // TODO: Recognize the large ones
  // Set clusters[i].label to a DistLabel
  UNUSED(clusters);
}


void Classifier::labelIntraIntervals(
  const vector<Peak>& peaks,
  vector<Interval>& intervals) const
{
  UNUSED(peaks);
  UNUSED(intervals);
}


void Classifier::groupIntoCars(
  const vector<Interval>& intervals,
  vector<Car>& cars) const
{
  // TODO: Group a short, an intra and a short gap into a car
  UNUSED(intervals);
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


void Classifier::setSampleRate(const int hertzIn)
{
  hertz = hertzIn;
}


void Classifier::setCountry(const string& countryIn)
{
  country = countryIn;
}


void Classifier::setYear(const int yearIn)
{
  year = yearIn;
}


void Classifier::classify(
  const vector<Peak>& peaks,
  const Database& db,
  TrainFound& trainFound) const
{
  if (peaks.size() < MIN_PEAKS)
  {
    trainFound.dbTrainNo = -1;
    trainFound.distance = 0.;
    return;
  }

  vector<int> diffs;
  Classifier::makeTimeDifferences(peaks, diffs);

  vector<Cluster> clusters;
  // Classifier::detectClusters(diffs, clusters);
  Classifier::KmeansOptimalClusters(diffs, clusters);
Classifier::printClusters(clusters);

  Classifier::classifyClusters(clusters);

  vector<Interval> intervals;
  Classifier::labelIntraIntervals(peaks, intervals);

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
    cout << "sdev " << cluster.sdev << endl;
    cout << "range " << cluster.lower << " to " << cluster.upper << endl;
    cout << "count " << cluster.count << endl;
    cout << "label " << cluster.label << endl << endl;
  }
}

