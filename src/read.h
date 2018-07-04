#ifndef TRAIN_READ_H
#define TRAIN_READ_H

using namespace std;

class Database;


bool readInt(
  const string& text,
  int& value,
  const string& err);

bool readFloat(
  const string& text,
  float& value,
  const string& err);

void readCarFiles(
  Database& db,
  const string& dir);

void readTrainFiles(
  Database& db,
  const string& dir);

#endif
