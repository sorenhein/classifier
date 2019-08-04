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


void CrossStats::setConnectivity(
  list<ListEntry>& segments,
  vector<unsigned>& connectCount) const
{
  // Set up the basic connectivity (from -> to).
  const unsigned nsl = nameMap.size();
  connectCount.resize(nsl);

  for (auto& it: nameMap)
  {
    segments.emplace_back(ListEntry());
    ListEntry& le = segments.back();
    le.name = it.first;
    le.index = it.second;
    le.connected.resize(nsl);

    for (unsigned i = 0; i < nsl; i++)
    {
      if (countCross[le.index][i] > 0 || le.index == i)
      {
        le.connected[i] = true;
        connectCount[le.index]++;
      }
      else
      {
        le.connected[i] = false;
      }
    }
  }
}


void CrossStats::printConnectivity(
  const list<ListEntry>& segments,
  const vector<unsigned>& connectCount) const
{
  for (auto& segment: segments)
  {
    cout << "Name         " << segment.name << "\n";
    cout << "Index        " << segment.index << "\n";
    cout << "Count        " << connectCount[segment.index] << "\n";
    cout << "Connected to";
    for (unsigned i = 0; i < segment.connected.size(); i++)
      if (segment.connected[i])
        cout << " " << i;
    cout << "\n\n";
  }
}


void CrossStats::segment(
  list<string>& orphans,
  list<string>& singletons,
  list<map<string, bool>>& segmentMaps) const
{
  list<ListEntry> segments;
  vector<unsigned> connectCount;
  CrossStats::setConnectivity(segments, connectCount);
  if (segments.empty())
    return;

  const unsigned nsl = nameMap.size();
  vector<list<ListEntry>::iterator> keys;
  keys.resize(nsl);
  for (auto sit = segments.begin(); sit != segments.end(); sit++)
    keys[sit->index] = sit;

  vector<bool> active;
  active.resize(nsl, true);

  // Keep combining connected indices.
  bool changeFlag;
  do
  {
    changeFlag = false;
    for (auto sit1 = segments.begin(); sit1 != segments.end(); sit1++)
    {
      // if (! present[sit1->index] || ! active[sit1->index])
      if (! active[sit1->index])
        continue;

      for (unsigned i = 0; i < nsl; i++)
      {
        // Don't recurse.
        if (sit1->index == i)
          continue;

        // Already gone?
        if (! active[i])
          continue;

        // Not connected?
        if (! sit1->connected[i])
          continue;

        // Copy the connectivity.
        auto sit2 = keys[i];
        for (unsigned j = 0; j < nsl; j++)
        {
          if (sit2->connected[j] && ! sit1->connected[j])
          {
            sit1->connected[j] = true;
            connectCount[sit1->index]++;
            changeFlag = true;
            // segChangeFlag = true;
          }
        }

        // Eliminate.
const unsigned deadIndex = sit2->index;
          active[sit2->index] = false;
          segments.erase(sit2);

          for (auto& seg: segments)
          {
            if (seg.index == sit1->index)
              continue;

            if (seg.connected[deadIndex])
            {
              seg.connected[deadIndex] = false;
              if (seg.connected[sit1->index])
              {
                connectCount[sit1->index]--;
              }
              else
              {
                seg.connected[sit1->index] = true;
                changeFlag = true;
              }
            }
          }

      }
    }
  }
  while (changeFlag);

  // Split into singletons and more complex confusion matrices.
  for (auto segment: segments)
  {
    const unsigned index = segment.index;
    if (connectCount[index] == 0)
      orphans.push_back(segment.name);
    else if (connectCount[index] == 1)
      singletons.push_back(segment.name);
    else
    {
      segmentMaps.emplace_back(map<string, bool>());
      map<string, bool>& segMap = segmentMaps.back();
      for (unsigned i = 0; i < nsl; i++)
        segMap[numberMap[i]] = segment.connected[i];
    }
  }
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


string CrossStats::strHeader(const map<string, bool>& selectMap) const
{
  // Make a header for the subset of nameMap that is selected.

  stringstream ss;

  ss << 
    setw(3) << right << "No" << " " <<
    setw(18) << left << "Name" << 
    setw(6) << right << "Count";

  unsigned i = 0;
  for (auto& it: nameMap)
  {
    if (selectMap.at(it.first))
    {
      ss << setw(4) << right << i;
      i++;
    }
  }

  ss << "\n" << string(28 + 4*i, '-') << "\n";

  return ss.str();
}


string CrossStats::strLines(const map<string, bool>& selectMap) const
{
  // Make a matrix output for the subset of nameMap that is selected.

  stringstream ss;

  unsigned c = 0;
  for (auto& it1: nameMap)
  {
    if (! selectMap.at(it1.first))
      continue;

    const unsigned i = it1.second;
    ss <<
      setw(3) << c << " " <<
      setw(18) << left << numberMap[i] <<
      setw(6) << right << countEntries[i];
    c++;

    for (auto& it2: nameMap)
    {
      if (! selectMap.at(it2.first))
        continue;

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


string CrossStats::strSimple(
  const list<string>& simple,
  const string& text) const
{
  if (simple.empty())
    return "";

  stringstream ss;
  ss << text << "\n";

  for (auto& s: simple)
  {
    ss << setw(22) << left << s <<
      setw(6) << right <<  countEntries[nameMap.at(s)] << "\n";
  }
  
  return ss.str() + "\n";
}


string CrossStats::str() const
{
  // A default map using everything.
  map<string, bool> selectMap;
  for (auto& it1: nameMap)
    selectMap[it1.first] = true;

  stringstream ss;
  ss << "Stats: Cross\n";
  ss << string(12, '-') << "\n\n";

  // TODO Change back to ss
  ss << CrossStats::strHeader(selectMap);
  ss << CrossStats::strLines(selectMap);

  // Split into independent confusion matrices that are easier to print.
  list<string> orphans;
  list<string> singletons;
  list<map<string, bool>> segmentMaps;
  CrossStats::segment(orphans, singletons, segmentMaps);

  ss << CrossStats::strSimple(singletons, "Singletons");
  ss << CrossStats::strSimple(orphans, "Orphans");

  for (auto& segMap: segmentMaps)
  {
    ss << CrossStats::strHeader(segMap);
    ss << CrossStats::strLines(segMap);
  }

  return ss.str();
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

