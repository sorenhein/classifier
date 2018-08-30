#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "PeakCluster.h"

#define RATIO_LIMIT 1.25


PeakCluster::PeakCluster()
{
  PeakCluster::reset();
}


PeakCluster::~PeakCluster()
{
}


void PeakCluster::reset()
{
  newFlag = true;
}


void PeakCluster::operator +=(const PeakData& peak)
{
  // In principle this could be a proper clustering analysis.
  // For now we group the minima into 
  // (a) Relatively balanced, such as single wheels, 
  // (b) Dominated by the left side, such as the first wheel
  //     of a bogie pair,
  // (c) Dominated by the right side.
  
  if (peak.right.area == 0.f)
  {
    cout << "Attempted to log a peak with zero right area.\n";
    return;
  }

  newFlag = true;
  const float ratio = peak.left.area / peak.right.area;

  if (ratio > RATIO_LIMIT)
    leftHeavy += peak;
  else if (ratio < 1.f / RATIO_LIMIT)
    rightHeavy += peak;
  else
    similar += peak;
}


void PeakCluster::calc()
{
  if (newFlag)
  {
    similar.calc();
    leftHeavy.calc();
    rightHeavy.calc();
    newFlag = false;
  }
}


void PeakCluster::print(
  const ClusterStat& cstat,
  const string& title) const
{
  cout << title << ": " <<
    cstat.num << ", " <<
    cstat.sum << ", " <<
    cstat.sumsq << ", " <<
    cstat.mean << ", " <<
    cstat.sdev << "\n";
}


void PeakCluster::print(
  const PeakStat& pstat,
  const string& title) const
{
  cout << title << ":\n";
  PeakCluster::print(pstat.left, "left");
  PeakCluster::print(pstat.right, "right");
  PeakCluster::print(pstat.ratio, "ratio");
  cout << endl;
}


bool PeakCluster::isOutlier(const PeakData& peak)
{
  PeakCluster::calc();

  const float ratio = peak.left.area / peak.right.area;

  PeakStat * pstat;
  if (ratio > RATIO_LIMIT)
    pstat = &leftHeavy;
  else if (ratio < 1.f / RATIO_LIMIT)
    pstat = &rightHeavy;
  else
    pstat = &similar;

  // Statistically unlikely (2 standard deviations, 95%).
  if (ratio > pstat->ratio.mean + 2.f * pstat->ratio.sdev ||
      ratio < pstat->ratio.mean - 2.f * pstat->ratio.sdev)
    return true;

  // Statistically almost impossible.
  if (peak.left.area > 
      pstat->left.mean + 5.f * pstat->left.sdev ||
      peak.left.area < 
      pstat->left.mean - 5.f * pstat->left.sdev)
    return true;
      
  if (peak.right.area > 
      pstat->right.mean + 5.f * pstat->right.sdev ||
      peak.right.area < 
      pstat->right.mean - 5.f * pstat->right.sdev)
    return true;

  return false;
}

