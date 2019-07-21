#ifndef TRAIN_READ_H
#define TRAIN_READ_H

using namespace std;

struct Control;


bool readControlFile(
  Control& control,
  const string& fname);

#endif
