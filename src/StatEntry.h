#ifndef TRAIN_STAT_ENTRY_H
#define TRAIN_STAT_ENTRY_H

#include <iostream>
#include <vector>
#include <string>

using namespace std;


class StatEntry
{
  private:

    struct Entry
    {
      int count;
      double sum;
      double sumsq;
    };

    vector<Entry> entries;

    string avg(
      const unsigned index,
      const unsigned prec = 2) const;

    string sdev(
      const unsigned index,
      const unsigned prec = 2) const;


  public:

    StatEntry();

    ~StatEntry();

    bool log(
      const vector<double>& actual,
      const vector<double>& estimate,
      const double residuals);

    void printHeader(
      ofstream& fout,
      const string& header) const;

    void print(
      ofstream& fout,
      const string& title) const;
};

#endif
