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

    enum EntryOrder
    {
      ENTRY_OFFSET = 0,
      ENTRY_SPEED = 1,
      ENTRY_ACCEL = 2,
      ENTRY_RESIDUAL = 3,
      ENTRY_SIZE = 4
    };

    string title;

    vector<Entry> entries;


  public:

    StatEntry();

    ~StatEntry();

    void setTitle(const string& text);

    void log(
        const vector<double>& actual,
        const vector<double>& estimate,
        const double residuals);

    void printHeader(const ofstream& fname) const;

    void print(const ofstream& fname) const;
};

#endif
