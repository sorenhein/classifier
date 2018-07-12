#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "StatCross.h"

#define DEFAULT_SIZE 20
#define DEFAULT_INCR 10
#define SEPARATOR ";"


StatCross::StatCross()
{
  numEntries = 0;
  nameMap.clear();
  numberMap.clear();
  StatCross::resize(DEFAULT_SIZE);
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
      StatCross::resize(numEntries + DEFAULT_INCR);
    
    nameMap[name] = numEntries;
    numberMap[numEntries] = name;
    return numEntries++;
  }
  else
    return it->second;
}


void StatCross::log(
  const string& trainActual,
  const string& trainEstimate)
{
  const unsigned nActual = StatCross::nameToNumber(trainActual);
  const unsigned nEstimate = StatCross::nameToNumber(trainEstimate);

  countEntries[nActual]++;
  countCross[nActual][nEstimate]++;
}


string StatCross::percent(
  const int num,
  const int denom) const
{
  if (denom == 0 || num == 0)
    return "";

  stringstream ss;
  ss << fixed << setprecision(2) << 
    100. * num / static_cast<double>(denom) << "%";
  return ss.str();
}



void StatCross::printCountCSV(const string& fname) const
{
  ofstream fout;
  fout.open(fname);

  string s = string(SEPARATOR) + "count";
  for (unsigned i = 0; i < numEntries; i++)
    s += SEPARATOR + numberMap[i];
  fout << s << endl;

  for (unsigned i = 0; i < numEntries; i++)
  {
    s = numberMap[i] + SEPARATOR + to_string(countEntries[i]);
    for (unsigned j = 0; j < numEntries; j++)
    {
      s += SEPARATOR;
      if (countCross[i][j] > 0)
        s += to_string(countCross[i][j]);
    }
    fout << s << endl;
  }

  fout.close();
}


void StatCross::printPercentCSV(const string& fname) const
{
  ofstream fout;
  fout.open(fname);

  string s = string(SEPARATOR) + "count";
  for (unsigned i = 0; i < numEntries; i++)
    s += SEPARATOR + numberMap[i];
  fout << s << endl;

  for (unsigned i = 0; i < numEntries; i++)
  {
    s = numberMap[i] + SEPARATOR + to_string(countEntries[i]);
    for (unsigned j = 0; j < numEntries; j++)
      s += SEPARATOR + StatCross::percent(countEntries[i], countCross[i][j]);
    fout << s << endl;
  }

  fout.close();
}

