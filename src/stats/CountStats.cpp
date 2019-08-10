#include <iostream>
#include <iomanip>
#include <sstream>
#include <mutex>

#include "CountStats.h"

static mutex mtx;


CountStats::CountStats()
{
  CountStats::reset();
}


CountStats::~CountStats()
{
}


void CountStats::reset()
{
  counts.clear();
}


void CountStats::init(
  const string& titleIn,
  const vector<string>& tagsIn)
{
  title = titleIn;

  fieldLength = 0;
  for (auto& tag: tagsIn)
  {
    counts[tag] = 0;
    if (tag.size() > fieldLength)
      fieldLength = tag.size();
  }
}


bool CountStats::log(
  const string& tag,
  const unsigned incr)
{
  auto it = counts.find(tag);
  if (it == counts.end())
    return false;

  mtx.lock();
  it->second += incr;
  mtx.unlock();
  return true;
}


string CountStats::str() const
{
  stringstream ss;
  const string s = "Stats: " + title;
  ss << s << "\n" <<
    string(s.size(), '-') << "\n\n";

  for (auto count: counts)
    ss << setw(fieldLength) << left << count.first <<
      setw(8) << right << count.second << "\n";
  
  return ss.str() + "\n";
}

