#include <iostream>
#include <iomanip>
#include <sstream>

#include "Control2.h"


Control2::Control2()
{
  Control2::reset();
}


Control2::~Control2()
{
}


void Control2::reset()
{
  fields.clear();
  fieldCounts.clear();

  Control2::configure();
}


void Control2::configure()
{
  fields =
  {
    // Example: ../data/cars
    { "CAR_DIRECTORY", CORRESPONDENCE_STRING, CTRL_CAR_DIRECTORY },
    // Example: ../data/trains
    { "TRAIN_DIRECTORY", CORRESPONDENCE_STRING, CTRL_TRAIN_DIRECTORY },
    // Example: ../data/residuals
    { "CORRECTION_DIRECTORY", CORRESPONDENCE_STRING, 
      CTRL_CORRECTION_DIRECTORY },
    // Example: ../../../mini_dataset_v012/sensors.txt
    { "SENSOR_FILE", CORRESPONDENCE_STRING, CTRL_SENSOR_FILE },
    // Example: DEU
    { "COUNTRY", CORRESPONDENCE_STRING, CTRL_SENSOR_COUNTRY },
    // Example: ../../../mini_dataset_v012/data/sensors/062493/raw
    { "TRACE_DIRECTORY", CORRESPONDENCE_STRING, CTRL_TRACE_DIRECTORY },
    // Example: ../../../mini_dataset_v012/labels.csv
    { "TRUTH_FILE", CORRESPONDENCE_STRING, CTRL_TRUTH_FILE },
    // Example: output/overview.txt
    { "OVERVIEW_FILE", CORRESPONDENCE_STRING, CTRL_OVERVIEW_FILE },
    // Example: output/details.txt
    { "DETAIL_FILE", CORRESPONDENCE_STRING, CTRL_DETAIL_FILE }
  };

  fieldCounts =
  {
    CTRL_STRINGS_SIZE,
    0,
    0,
    0,
    0,
    0,
    0
  };
}


bool Control2::readFile(const string& fname)
{
  if (! entry.readTagFile(fname, fields, fieldCounts))
    return false;

  return true;
}


const string& Control2::carDir() const
{
  return entry.getString(CTRL_CAR_DIRECTORY);
}


const string& Control2::trainDir() const
{
  return entry.getString(CTRL_TRAIN_DIRECTORY);
}


const string& Control2::correctionDir() const
{
  return entry.getString(CTRL_CORRECTION_DIRECTORY);
}


const string& Control2::sensorFile() const
{
  return entry.getString(CTRL_SENSOR_FILE);
}


const string& Control2::sensorCountry() const
{
  return entry.getString(CTRL_SENSOR_COUNTRY);
}


const string& Control2::traceDir() const
{
  return entry.getString(CTRL_TRACE_DIRECTORY);
}


const string& Control2::truthFile() const
{
  return entry.getString(CTRL_TRUTH_FILE);
}


const string& Control2::overviewFile() const
{
  return entry.getString(CTRL_OVERVIEW_FILE);
}


const string& Control2::detailFile() const
{
  return entry.getString(CTRL_DETAIL_FILE);
}

