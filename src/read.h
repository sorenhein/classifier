#ifndef TRAIN_READ_H
#define TRAIN_READ_H

#include <vector>

#include "struct.h"

using namespace std;

class Database;


void tokenize(
  const string& text,
  vector<string>& tokens,
  const string& delimiters);

unsigned countDelimiters(
  const string& text,
  const string& delimiters);

bool readInt(
  const string& text,
  int& value,
  const string& err);

bool readDouble(
  const string& text,
  double& value,
  const string& err);

bool readControlFile(
  Control& control,
  const string& fname);

void readCarFiles(
  Database& db,
  const string& dir);

void readTrainFiles(
  Database& db,
  const string& dir);

bool readInputFile(
  const string& fname,
  vector<InputEntry>& actualList);

#endif
