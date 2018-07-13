#include <iomanip>
#include <limits>
#include <algorithm>

#include "Ckmeans/Ckmeans.1d.dp.h"
#include "Clusters.h"

#define MAX_CLUSTERS 12


Clusters::Clusters()
{
  clusters.clear();
}


Clusters::~Clusters()
{
}


double Clusters::logVector(
  const double * x, 
  const unsigned l,
  const unsigned numClusters)
{
  double * y = (double *) malloc(l * sizeof(double));
  int * Kcluster = (int *) malloc(l * sizeof(int));

  for (unsigned i = 0; i < l; i++)
  {
    y[i] = 1;
    Kcluster[i] = 1;
  }

  double * centers = (double *) malloc(MAX_CLUSTERS * sizeof(double));
  double * withinss = (double *) malloc(MAX_CLUSTERS * sizeof(double));
  double * size = (double *) malloc(MAX_CLUSTERS * sizeof(double));
  double * BIC = (double *) malloc(MAX_CLUSTERS * sizeof(double));

  for (unsigned c = 0; c < MAX_CLUSTERS; c++)
  {
    centers[c] = 0.;
    withinss[c] = 0.;
    size[c] = 0.;
    BIC[c] = 0.;
  }

  unsigned nc;
  if (numClusters == 0)
  {
    // Let the Bayesian information criterion decide.
    kmeans_1d_dp(x, l, y, 2, MAX_CLUSTERS,
      Kcluster, centers, withinss, size, BIC,
      "BIC", "linear", L2);
    nc = static_cast<unsigned>(Kcluster[l-1]) + 1;

cout << "Cluster data\n";
for (unsigned i = 0; i < MAX_CLUSTERS; i++)
{
  cout << "i " << i << ": " << centers[i] << " " <<
    withinss[i] << " " << size[i] << " " <<
    BIC[i] << endl;
}
  }
  else
  {
    kmeans_1d_dp(x, l, y, numClusters, numClusters,
      Kcluster, centers, withinss, size, BIC,
      "BIC", "linear", L2);
    nc = numClusters;
  }

  clusters.clear();
  clusters.resize(nc);

  vector<unsigned> lastClusterIndex(nc);
  vector<double> sum(nc), sumsq(nc);

  for (unsigned i = 0; i < l; i++)
  {
    const unsigned c = static_cast<unsigned>(Kcluster[i]);
    clusters[c].count++;
    clusters[c].upper = x[i];

    if (clusters[c].lower == 0.)
      clusters[c].lower = x[i];

    lastClusterIndex[c] = i;

    const double df = x[i];
    sum[c] += df;
    sumsq[c] += df*df;
  }

  double residuals = 0.;
  for (unsigned c = 0; c < nc; c++)
  {
    clusters[c].center = centers[c];
    residuals += withinss[c];

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

  sort(clusters.begin(), clusters.end());
  return residuals;
}


void Clusters::log(
  const vector<int>& axles,
  const int numClusters)
{
  const unsigned l = axles.size()-1;
  if (l < 2)
  {
    cout << "Vector too short\n";
    return;
  }

  // Too much shuffling around of data, including in Ckmeans itself.
  vector<double> xtemp(l);
  for (unsigned i = 0; i < l; i++)
    xtemp[i] = (axles[i+1] - axles[i]) / 1000.; // In m
  sort(xtemp.begin(), xtemp.end());

  double * x = (double *) malloc(l * sizeof(double));
  for (unsigned i = 0; i < l; i++)
    x[i] = xtemp[i];

  // Specific number of clusters.
  (void) Clusters::logVector(x, l, numClusters);
}


void Clusters::log(const vector<PeakTime>& times)
{
  const unsigned l = times.size()-1;
  if (l < 2)
  {
    cout << "Vector too short\n";
    return;
  }

  vector<double> xtemp(l);
  for (unsigned i = 0; i < l; i++)
    xtemp[i] = times[i+1].time - times[i].time;
  sort(xtemp.begin(), xtemp.end());

  double * x = (double *) malloc(l * sizeof(double));
  for (unsigned i = 0; i < l; i++)
    x[i] = xtemp[i];

  // "Best" number of clusters.
  // double residuals = Clusters::logVector(x, l, 0);

  double residuals = Clusters::logVector(x, l, 2);
cout << "Start at 2: " << residuals << endl;
  for (unsigned i = 3; i < MAX_CLUSTERS; i++)
  {
    double newRes = Clusters::logVector(x, l, i);
cout << "At " << i << ": " << newRes << endl;
    if (newRes >= residuals / 2.)
    {
      // Stop if the error is no longer at least halving.
    }
    else
    {
      // residuals = Clusters::logVector(x, l, i-1);
      break;
    }
  }
}


const unsigned Clusters::size() const
{
  return clusters.size();
}


double Clusters::warp(
  vector<ClusterEntry>& newMedians,
  const Clusters& other) const
{
  /*
     The penalty for moving an N-point bin from median M1 to M2 is 
     N * abs(M2-M1).  Say we have N pairs of (xi, yi) = (median, count).
     We also have reference clusters with medians zi that have certain 
     ratios ri = zi / z0 (r1 = 0).  We want to make the smallest change 
     to the xi such that the xi's have the same ratios.

     When we're done, (xi + di) / (x0 + d0) = ri, so 
     di = ri (x0 + d0) - xi (Eq 1).

     We want to minimize D = sum(yi * abs(di)).  Maybe a quadratic criterion
     would be better, but using (Eq 1) this is piecewise linear in d0.
     Therefore D must reach its minimum at a point where one of the absolute
     values is zero.  Therefore we only need to check values of d0 such that
     di = 0, i.e.

     d0 = -x0 + xi/ri.
  */
  
  const unsigned l = other.clusters.size();
  vector<double> ratios(l);
  for (unsigned i = 0; i < l; i++)
    ratios[i] = other.clusters[i].median / other.clusters[0].median;

  // Large starting points.
  double dist = numeric_limits<double>::max();
  unsigned bestIndex = l;
  double bestd0 = numeric_limits<double>::max();

  for (unsigned kink = 0; kink < l; kink++)
  {
    const double d0 = -clusters[0].median + 
      clusters[kink].median / ratios[kink];

    double d = 0;
    for (unsigned i = 0; i < l; i++)
    {
      const double di = ratios[i] * (clusters[0].median + d0) - 
        clusters[i].median;

      d += clusters[i].median * abs(di);
    }

    if (d < dist)
    {
      dist = d;
      bestIndex = kink;
      bestd0 = d0;
    }
  }

  for (unsigned i = 0; i < l; i++)
  {
    const double di = ratios[i] * (clusters[0].median + bestd0) - 
      clusters[i].median;
    newMedians[i].median = clusters[i].median + di;
    newMedians[i].count = clusters[i].count;
  }

  return dist;
}


double Clusters::removeFromBelow(
  vector<ClusterEntry>& newMedians,
  const Clusters& other,
  unsigned number) const
{
  const unsigned nc = newMedians.size();
  double dist = 0.;
  for (unsigned i = 0; i < nc && number > 0; i++)
  {
    if (newMedians[i].count <= other.clusters[i].count)
      continue;

    unsigned toRemove = newMedians[i].count - other.clusters[i].count;
    if (toRemove > number)
      toRemove = number;

    newMedians[i].count -= toRemove;
    dist += toRemove * newMedians[i].median;
    number -= toRemove;
  }
  return dist;
}


double Clusters::addFromBelow(
  vector<ClusterEntry>& newMedians,
  const Clusters& other,
  unsigned number) const
{
  const unsigned nc = newMedians.size();
  double dist = 0.;
  for (unsigned i = 0; i < nc && number > 0; i++)
  {
    if (newMedians[i].count >= other.clusters[i].count)
      continue;

    unsigned toAdd = other.clusters[i].count - newMedians[i].count;
    if (toAdd > number)
      toAdd = number;

    newMedians[i].count += toAdd;
    dist += toAdd * newMedians[i].median;
    number -= toAdd;
  }
  return dist;
}


double Clusters::moveUpFrom(
  vector<ClusterEntry>& newMedians,
  const Clusters& other,
  const unsigned index,
  unsigned number) const
{
  const unsigned nc = newMedians.size();
  double dist = 0.;
  for (unsigned j = index+1; j < nc && number > 0; j++)
  {
    if (newMedians[j].count < other.clusters[j].count)
    {
      unsigned toMove = 
        other.clusters[j].count - newMedians[j].count;
      if (toMove > number)
        toMove = number;

      newMedians[index].count -= toMove;
      newMedians[j].count += toMove;
      dist += toMove * (newMedians[j].median - newMedians[index].median);
      number -= toMove;
    }
  }
  return dist;
}


double Clusters::moveDownTo(
  vector<ClusterEntry>& newMedians,
  const Clusters& other,
  const unsigned index,
  unsigned number) const
{
  const unsigned nc = newMedians.size();
  double dist = 0.;
  for (unsigned j = index+1; j < nc && number > 0; j++)
  {
    if (newMedians[j].count > other.clusters[j].count)
    {
      unsigned toMove = 
        newMedians[j].count - other.clusters[j].count;
      if (toMove > number)
        toMove = number;

      newMedians[index].count += toMove;
      newMedians[j].count -= toMove;
      dist += toMove * (newMedians[j].median - newMedians[index].median);
      number -= toMove;
    }
  }
  return dist;
}


double Clusters::balance(
  vector<ClusterEntry>& newMedians,
  const Clusters& other) const
{
  /*
     The penalty for adding or subtracting N points in a bin with median M
     is N * M.  This probably looks a bit more objective than it is.
     Also not sure that the below algorithm is completely optimal.
  */
  
  const unsigned nc = clusters.size();
  unsigned countOwn = 0, countOther = 0;
  for (unsigned i = 0; i < nc; i++)
  {
    countOwn += newMedians[i].count;
    countOther += other.clusters[i].count;
  }

  double dist = 0.;

  if (countOwn > countOther)
    dist += Clusters::removeFromBelow(newMedians, other, countOwn-countOther);
  else if (countOwn < countOther)
    dist += Clusters::addFromBelow(newMedians, other, countOther-countOwn);

  // There might a more closed-form solution for this.
  for (unsigned i = 0; i < nc; i++)
  {
    if (newMedians[i].count > other.clusters[i].count)
    {
      dist += Clusters::moveUpFrom(newMedians, other, i, 
        newMedians[i].count - other.clusters[i].count);
    }
    else if (newMedians[i].count < other.clusters[i].count)
    {
      dist += Clusters::moveDownTo(newMedians, other, i, 
        newMedians[i].count - other.clusters[i].count);
    }
  }

  return dist;
}


double Clusters::distance(const Clusters& other) const
{
  /*
     We quantify the changes we have to make to our own histogram to turn 
     it into the "other" histogram.  First we warp the medians to have the
     same ratios.  Second we add, subtract or move entries from each bin
     in order to balance the two.
  */
  
  const unsigned nc = clusters.size();
  if (nc != other.clusters.size())
  {
    cout << "Cluster numbers do not match up.\n";
    return -1.;
  }

  if (nc < 2)
  {
    cout << "Too few clusters.\n";
    return -1.;
  }

  if (other.clusters[0].median <= 0.)
  {
    cout << "Reference cluster is too low.\n";
    return -1.;
  }

  vector<ClusterEntry> newMedians(nc);
  double dist = Clusters::warp(newMedians, other);
  dist += Clusters::balance(newMedians, other);

  return dist;
}


void Clusters::print() const
{
  for (auto &cluster: clusters)
  {
    cout << "center " << cluster.center << endl;
    cout << "median " << cluster.median << endl;
    cout << "sdev   " << cluster.sdev << endl;
    cout << "range  " << cluster.lower << " to " << cluster.upper << endl;
    cout << "count  " << cluster.count << endl;
    cout << "tag    " << cluster.tag << endl << endl;
  }
}

