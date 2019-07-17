#include "CarCollection.h"


void CarCollection::configure()
{
  fields =
    {
    { "OFFICIAL_NAME", CORRESPONDENCE_STRING, CAR_OFFICIAL_NAME },
    { "NAME", CORRESPONDENCE_STRING, CAR_NAME },
    { "CONFIGURATION", CORRESPONDENCE_STRING, CAR_CONFIGURATION_UIC },
    { "COMMENT", CORRESPONDENCE_STRING, CAR_COMMENT },

    { "USAGE", CORRESPONDENCE_STRING_VECTOR, CAR_USAGE },
    { "COUNTRIES", CORRESPONDENCE_STRING_VECTOR, CAR_COUNTRIES },

    { "INTRODUCTION", CORRESPONDENCE_INT, CAR_INTRODUCTION },
    { "CAPACITY", CORRESPONDENCE_INT, CAR_CAPACITY },
    { "CLASS", CORRESPONDENCE_INT, CAR_CLASS },
    { "WEIGHT", CORRESPONDENCE_INT, CAR_WEIGHT },
    { "WHEEL_LOAD", CORRESPONDENCE_INT, CAR_WHEEL_LOAD },
    { "SPEED", CORRESPONDENCE_INT, CAR_SPEED },
    { "LENGTH", CORRESPONDENCE_INT, CAR_LENGTH },
    { "DIST_WHEELS", CORRESPONDENCE_INT, CAR_DIST_WHEELS },
    { "DIST_WHEELS1", CORRESPONDENCE_INT, CAR_DIST_WHEELS1 },
    { "DIST_WHEELS2", CORRESPONDENCE_INT, CAR_DIST_WHEELS2 },
    { "DIST_MIDDLES", CORRESPONDENCE_INT, CAR_DIST_MIDDLES },
    { "DIST_PAIR", CORRESPONDENCE_INT, CAR_DIST_PAIR },
    { "DIST_FRONT_TO_WHEEL", CORRESPONDENCE_INT, CAR_DIST_FRONT_TO_WHEEL },
    { "DIST_WHEEL_TO_BACK", CORRESPONDENCE_INT, CAR_DIST_WHEEL_TO_BACK },
    { "DIST_FRONT_TO_MID1", CORRESPONDENCE_INT, CAR_DIST_FRONT_TO_MID1 },
    { "DIST_FRONT_TO_MID2", CORRESPONDENCE_INT, CAR_DIST_FRONT_TO_MID2 },

    { "POWER", CORRESPONDENCE_BOOL, CAR_POWER },
    { "RESTAURANT", CORRESPONDENCE_BOOL, CAR_RESTAURANT },
    { "SYMMETRY", CORRESPONDENCE_BOOL, CAR_SYMMETRY }
  };
}

void CarCollection::complete(Entity& entry)
{
  const unsigned dw = entry.getInt(CAR_DIST_WHEELS);
  if (dw > 0)
  {
    if (entry.getInt(CAR_DIST_WHEELS1) == -1)
      entry.setInt(CAR_DIST_WHEELS1, dw);
    if (entry.getInt(CAR_DIST_WHEELS1) == -1)
      entry.setInt(CAR_DIST_WHEELS2, dw);
  }

  entry.setBool(CAR_FOURWHEEL_FLAG,
    entry.getInt(CAR_DIST_WHEELS1) > 0 && 
    entry.getInt(CAR_DIST_WHEELS2) > 0);
}

