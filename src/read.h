#ifndef TRAIN_READ_H
#define TRAIN_READ_H

using namespace std;

class Database;


void readCarFiles(
  Database& db,
  const string& dir);

void readTrainFiles(
  Database& db,
  const string& dir);

#endif
