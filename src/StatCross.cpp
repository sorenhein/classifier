#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "StatCross.h"

#define DEFAULT_SIZE 20
#define DEFAULT_INCR 10


StatCross::StatCross()
{
  numEntries = 0;
  StatCross::resize(DEFAULT_SIZE);
  nameMap.clear();
  numberMap.clear();
}


StatCross::~StatCross()
{
}


void StatCross::resize(const unsigned n)
{
  countEntries.resize(n);

  countCross.resize(n);
  for (unsigned i = 0; i < n; i++)
    countCross[i].resize(n);
  
  numberMap.resize(n);
}


unsigned StatCross::nameToNumber(const string& name)
{
  auto it = nameMap.find(name);
  if (it == nameMap.end())
  {
    if (numEntries == countEntries.size())
    {
      StatCross::resize(numEntries + DEFAULT_INCR);
        numberMap.resize(numEntries + DEFAULT_INCR);
    }
    
    nameMap[name] = numEntries;
    return numEntries++;
  }
  else
    return it->second;
}


bool StatCross::log(
  const string& trainActual,
  const string& trainEstimate)
{
  const unsigned nActual = StatCross::nameToNumber(trainActual);
  const unsigned nEstimate = StatCross::nameToNumber(trainEstimate);

  countEntries[nActual]++;
  countCross[nActual][nEstimate]++;
}


string StatCross::percent(
  const double num,
  const double denom) const
{
  if (denom == 0.)
    return "";

  stringstream ss;
  ss << fixed << setprecision(2) << 
    100. * num / denom << "%";
  return ss.str();
}



void StatCross::printCountCSV(const string& fname) const
{
  ofstream fout;
  fout.open(fname);

  string s = ",count";
  for (unsigned i = 0; i < numEntries; i++)
    s += "," + numberMap[i];
  fout << s << endl;

  for (unsigned i = 0; i < numEntries; i++)
  {
    s = numberMap[i] + "," + to_string(countEntries[i]);
    for (unsigned j = 0; i < numEntries; j++)
      s += "," + to_string(countCross[i][j]);
    fout << s << endl;
  }

  fout.close();
}


void StatCross::printPercentCSV(const string& fname) const
{
  ofstream fout;
  fout.open(fname);

  string s = ",count";
  for (unsigned i = 0; i < numEntries; i++)
    s += "," + numberMap[i];
  fout << s << endl;

  for (unsigned i = 0; i < numEntries; i++)
  {
    s = numberMap[i] + "," + to_string(countEntries[i]);
    for (unsigned j = 0; i < numEntries; j++)
      s += "," + StatCross::percent(countEntries[i], countCross[i][j]);
    fout << s << endl;
  }

  fout.close();
}

