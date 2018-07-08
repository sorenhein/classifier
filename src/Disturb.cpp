#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <random>
#include <ctime>


#include "Disturb.h"
#include "read.h"


Disturb::Disturb()
{
  injectPeaksMin = 0;
  injectPeaksMax = 0;
  injectedPeaks.clear();
  
  deletePeaksMin = 0;
  deletePeaksMax = 0;
  deletedPeaks.clear();

  pruneFrontMin = 0;
  pruneFrontMax = 0;
  prunedFronts.clear();
  
  pruneBackMin = 0;
  pruneBackMax = 0;
  prunedBacks.clear();

  detectedInjectedPeaks.clear();
  detectedDeletedPeaks.clear();
  detectedPrunedFronts.clear();
  detectedPrunedBacks.clear();

  srand(static_cast<unsigned>(time(NULL)));
}


Disturb::~Disturb()
{
}


bool Disturb::readFile(const string& fname)
{
  ifstream fin;
  fin.open(fname);
  string line;
  while (getline(fin, line))
  {
    if (line == "" || line.front() == '#')
      continue;

    const string err = "File " + fname + ": Bad line '" + line + "'";

    unsigned sp = line.find(" ");
    if (sp == string::npos || sp == 0 || sp == line.size()-1)
    {
      cout << err << endl;
      fin.close();
      return false;
    }

    const string& field = line.substr(0, sp);
    const string& rest = line.substr(sp+1);

    if (field == "NOISE_SDEV")
    {
      if (! readInt(rest, noiseSdev, err)) break;
    }
    else if (field == "INJECT_AT_LEAST")
    {
      if (! readInt(rest, injectPeaksMin, err)) break;
    }
    else if (field == "INJECT_UP_TO")
    {
      if (! readInt(rest, injectPeaksMax, err)) break;
    }
    else if (field == "INJECT")
    {
      if (! readInt(rest, injectPeaksMin, err)) break;
      injectPeaksMax = injectPeaksMin;
    }
    else if (field == "DELETE_AT_LEAST")
    {
      if (! readInt(rest, deletePeaksMin, err)) break;
    }
    else if (field == "DELETE_UP_TO")
    {
      if (! readInt(rest, deletePeaksMax, err)) break;
    }
    else if (field == "DELETE")
    {
      if (! readInt(rest, deletePeaksMin, err)) break;
      deletePeaksMax = deletePeaksMin;
    }
    else if (field == "PRUNE_FRONT_AT_LEAST")
    {
      if (! readInt(rest, pruneFrontMin, err)) break;
    }
    else if (field == "PRUNE_FRONT_UP_TO")
    {
      if (! readInt(rest, pruneFrontMax, err)) break;
    }
    else if (field == "PRUNE_FRONT")
    {
      if (! readInt(rest, pruneFrontMin, err)) break;
      pruneFrontMax = pruneFrontMin;
    }
    else if (field == "PRUNE_BACK_AT_LEAST")
    {
      if (! readInt(rest, pruneBackMin, err)) break;
    }
    else if (field == "PRUNE_BACK_UP_TO")
    {
      if (! readInt(rest, pruneBackMax, err)) break;
    }
    else if (field == "PRUNE_BACK")
    {
      if (! readInt(rest, pruneBackMin, err)) break;
      pruneBackMax = pruneBackMin;
    }
    else
    {
      cout << err << endl;
    }
  }
  return true;
}


int Disturb::getNoiseSdev() const
{
  return noiseSdev;
}


void Disturb::getInjectRange(
  int& injectMin,
  int& injectMax) const
{
  injectMin = injectPeaksMin;
  injectMax = injectPeaksMax;
}


void Disturb::getDeleteRange(
  int& deleteMin,
  int& deleteMax) const
{
  deleteMin = deletePeaksMin;
  deleteMax = deletePeaksMax;
}


void Disturb::getFrontRange(
  int& frontMin,
  int& frontMax) const
{
  frontMin = pruneFrontMin;
  frontMax = pruneFrontMax;
}


void Disturb::getBackRange(
  int& backMin,
  int& backMax) const
{
  backMin = pruneBackMin;
  backMax = pruneBackMax;
}


void Disturb::setInject(const unsigned pos)
{
  injectedPeaks.insert(pos);
}


void Disturb::setDelete(const unsigned pos)
{
  deletedPeaks.insert(pos);
}


void Disturb::setPruneFront(const unsigned number)
{
  prunedFronts.insert(number);
}


void Disturb::setPruneBack(const unsigned number)
{
  prunedBacks.insert(number);
}


void Disturb::detectInject(const unsigned pos)
{
  detectedInjectedPeaks.insert(pos);
}


void Disturb::detectDelete(const unsigned pos)
{
  detectedDeletedPeaks.insert(pos);
}


void Disturb::detectFront(const unsigned number)
{
  detectedPrunedFronts.insert(number);
}


void Disturb::detectBack(const unsigned number)
{
  detectedPrunedBacks.insert(number);
}

