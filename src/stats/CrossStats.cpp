#include <iostream>
#include <iomanip>
#include <sstream>

#include "CrossStats.h"
#include "../const.h"

#define DEFAULT_SIZE 20
#define DEFAULT_INCR 10


CrossStats::CrossStats()
{
  numEntries = 0;
  nameMap.clear();
  numberMap.clear();
  CrossStats::resize(DEFAULT_SIZE);
}


CrossStats::~CrossStats()
{
}


void CrossStats::resize(const unsigned n)
{
  countEntries.resize(n);

  countCross.resize(n);
  for (unsigned i = 0; i < n; i++)
    countCross[i].resize(n);
  
  numberMap.resize(n);
}


unsigned CrossStats::nameToNumber(const string& name)
{
  auto it = nameMap.find(name);
  if (it == nameMap.end())
  {
    if (numEntries == countEntries.size())
      CrossStats::resize(numEntries + DEFAULT_INCR);
    
    nameMap[name] = numEntries;
    numberMap[numEntries] = name;
    return numEntries++;
  }
  else
    return it->second;
}


void CrossStats::log(
  const string& trainActual,
  const string& trainEstimate)
{
  const unsigned nActual = CrossStats::nameToNumber(trainActual);
  const unsigned nEstimate = CrossStats::nameToNumber(trainEstimate);

  countEntries[nActual]++;
  countCross[nActual][nEstimate]++;
}


string CrossStats::percent(
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


string CrossStats::strHeader() const
{
  stringstream ss;

  ss << "Stats: Cross\n";
  ss << string(12, '-') << "\n\n";

  ss << 
    setw(3) << right << "No" << " " <<
    setw(16) << left << "Name" << 
    setw(6) << right << "Count";

  for (unsigned i = 0; i < nameMap.size(); i++)
    ss << setw(4) << right << i;

  ss << "\n" << string(26 + 4 * nameMap.size(), '-') << "\n";

  return ss.str();
}


string CrossStats::str() const
{
  stringstream ss;
  ss << CrossStats::strHeader();

  unsigned c = 0;
  for (auto& it1: nameMap)
  {
    const unsigned i = it1.second;
    ss <<
      setw(3) << c << " " <<
      setw(16) << left << numberMap[c] <<
      setw(6) << right << countEntries[c];
    c++;

    for (auto& it2: nameMap)
    {
      const unsigned j = it2.second;
      if (countCross[i][j] > 0)
        ss << setw(4) << countCross[i][j];
      else
        ss << setw(4) << right << "-";
    }
    ss << "\n";
  }

  return ss.str() + "\n";
}


string CrossStats::strCSV() const
{
  stringstream ss;

  string s = string(SEPARATOR) + "count";
  for (auto& it: nameMap)
    s += SEPARATOR + it.first;
  ss << s << endl;

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
    ss << s << endl;
  }

  return ss.str();
}


string CrossStats::strPercentCSV() const
{
  stringstream ss;

  string s = string(SEPARATOR) + "count";
  for (auto& it: nameMap)
    s += SEPARATOR + it.first;
  ss << s << endl;

  for (auto& it1: nameMap)
  {
    const unsigned i = it1.second;
    s = numberMap[i] + SEPARATOR + to_string(countEntries[i]);
    for (auto& it2: nameMap)
    {
      const unsigned j = it2.second;
      s += SEPARATOR + CrossStats::percent(countEntries[i], countCross[i][j]);
    }
    ss << s << endl;
  }

  return ss.str();
}

