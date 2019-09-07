#include <iomanip>
#include <sstream>
#include <mutex>

#include "ResidualsStats.h"

#include "../align/Alignment.h"

static mutex mtx;


ResidualsStats::ResidualsStats()
{
  ResidualsStats::reset();
}


ResidualsStats::~ResidualsStats()
{
}


void ResidualsStats::reset()
{
  stats.clear();
}


void ResidualsStats::log(
  const string& trainName,
  const Alignment& alignment)
{
  // A bit restrictive to lock them all, but simple.
  mtx.lock();
  stats[trainName].log(alignment);
  mtx.unlock();
}


void ResidualsStats::calculate()
{
  for (auto& st: stats)
    st.second.calculate();
}


string ResidualsStats::str() const
{
  if (stats.empty())
    return "";

  const string s = "Stats: Residuals";

  stringstream ss;
  ss << s << "\n" << string(s.size(), '-') << "\n\n";

  ss << stats.begin()->second.strHeader();

  for (auto& st: stats)
    ss << setw(18) << left << st.first << st.second.str();

 return ss.str() + "\n";
}


void ResidualsStats::write(const string& dir) const
{
  for (auto& st: stats)
    st.second.write(dir + "/" + st.first + ".dat");
}

