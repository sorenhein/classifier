#ifndef TRAIN_CONTROL2_H
#define TRAIN_CONTROL2_H
// TODO Change guard name back

#include <map>

#include "Entity.h"

using namespace std;


enum ControlFieldStrings
{
  CTRL_CAR_DIRECTORY = 0,
  CTRL_TRAIN_DIRECTORY = 1,
  CTRL_CORRECTION_DIRECTORY = 2,
  CTRL_SENSOR_FILE = 3,
  CTRL_SENSOR_COUNTRY = 4,
  CTRL_TRACE_DIRECTORY = 5,
  CTRL_TRUTH_FILE = 6,
  CTRL_OVERVIEW_FILE = 7,
  CTRL_DETAIL_FILE = 8,
  CTRL_STRINGS_SIZE = 9
};


class Control2
{
  private:

    list<CorrespondenceEntry> fields;

    vector<unsigned> fieldCounts;

    Entity entry;

    void configure();


  public:

    Control2();

    ~Control2();

    void reset();

    bool readFile(const string& fname);

    const string& carDir() const;
    const string& trainDir() const;
    const string& correctionDir() const;
    const string& sensorFile() const;
    const string& sensorCountry() const;
    const string& traceDir() const;
    const string& truthFile() const;
    const string& crosscountFile() const;
    const string& crosspercentFile() const;
    const string& overviewFile() const;
    const string& detailFile() const;
};

#endif
