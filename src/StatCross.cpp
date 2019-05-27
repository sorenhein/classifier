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
  for (auto& it: nameMap)
    s += SEPARATOR + it.first;
  fout << s << endl;

  for (auto& it1: nameMap)
  {
    const unsigned i = it1.second;
    s = numberMap[i] + SEPARATOR + to_string(countEntries[i]);
    for (auto& it2: nameMap)
    {
      const unsigned j = it2.second;
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
  for (auto& it: nameMap)
    s += SEPARATOR + it.first;
  fout << s << endl;

  for (auto& it1: nameMap)
  {
    const unsigned i = it1.second;
    s = numberMap[i] + SEPARATOR + to_string(countEntries[i]);
    for (auto& it2: nameMap)
    {
      const unsigned j = it2.second;
      s += SEPARATOR + StatCross::percent(countEntries[i], countCross[i][j]);
    }
    fout << s << endl;
  }

  fout.close();
}


void StatCross::printQuality() const
{
  // The second one counts train_N and train_R as the same.
  unsigned numAll = 0;
  unsigned numPerfect = 0;
  unsigned numUndirected = 0;

  for (auto& it: nameMap)
  {
    const unsigned i = it.second;
    numPerfect += static_cast<unsigned>(countCross[i][i]);
    numUndirected += static_cast<unsigned>(countCross[i][i]);

    for (auto& it2: nameMap)
    {
      const unsigned j = it2.second;
      numAll += static_cast<unsigned>(countCross[i][j]);
    }

    const string n = it.first;
    const unsigned l = n.size();
    if (l <= 2)
      continue;
    const string s = n.substr(l-2);
    string rev = n.substr(0, l-2);
    if (s == "_N")
      rev += "_R";
    else if (s == "_R")
      rev += "_N";
    else
      continue;

    auto itr = nameMap.find(rev);
    if (itr == nameMap.end())
      continue;

    const unsigned j = itr->second;
    numUndirected += static_cast<unsigned>(countCross[i][j]);
  }

  if (numAll == 0)
    return;

  cout << setw(20) << left << "Perfect matches" << 
    setw(8) << numPerfect << 
    setw(8) << right << fixed << setprecision(2) <<
    100. * numPerfect / numAll << "%" << endl;
  cout << setw(20) << left << "Undirected matches" << 
    setw(8) << numUndirected << 
    setw(8) << right << fixed << setprecision(2) <<
    100. * numUndirected / numAll << "%" << endl;
}

