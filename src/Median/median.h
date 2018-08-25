#ifndef TRAIN_MEDIAN_H
#define TRAIN_MEDIAN_H


typedef float Item;

struct Mediator
{
 Item* data; // circular queue of values
 int * pos;  // index into `heap` for each value
 int * heap; // max/median/min heap holding indexes into `data`.
 int N; // allocated size.
 int idx; // position in circular queue
 int ct; // count of items in queue
};

struct MediatorStats
{
  Item min;
  Item max;
  Item median;
};


Mediator * MediatorNew(int nItems);

void MediatorInsert(
  Mediator * m,
  Item v);

void GetMediatorStats(
  Mediator * m,
  MediatorStats * ms);

#endif
