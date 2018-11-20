#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "PeakStructure.h"
#include "CarModels.h"
#include "Except.h"


#define UNUSED(x) ((void)(true ? 0 : ((x), void(), 0)))



PeakStructure::PeakStructure()
{
  PeakStructure::reset();

  // This is a temporary tracker.
  for (unsigned i = 0; i < 3; i++)
    for (unsigned j = 0; j < 24; j++)
      matrix[i][j] = 0;
}


PeakStructure::~PeakStructure()
{
}


void PeakStructure::reset()
{
}


void PeakStructure::resetProfile()
{
  profile.bogeyWheels.clear();
  profile.wheels.clear();
  profile.stars.clear();

  profile.bogeyWheels.resize(4);
  profile.wheels.resize(2);
  profile.stars.resize(4);
  profile.sumGreat = 0;
  profile.sum = 0;
}


bool PeakStructure::matchesModel(
  const CarModels& models,
  const CarDetect& car, 
  unsigned& index,
  float& distance) const
{
  if (! models.findClosest(car, distance, index))
    return false;
  else
    return (distance <= 1.5f);
}


bool PeakStructure::findFourWheeler(
  const CarModels& models,
  const unsigned start,
  const unsigned end,
  const bool leftGapPresent,
  const bool rightGapPresent,
  const vector<unsigned>& peakNos, 
  const vector<Peak *>& peakPtrs,
  CarDetect& car) const
{
  car.setLimits(start, end);

  if (leftGapPresent)
    car.logLeftGap(peakNos[0] - start);

  if (rightGapPresent)
    car.logRightGap(end - peakNos[3]);

  car.logCore(
    peakNos[1] - peakNos[0],
    peakNos[2] - peakNos[1],
    peakNos[3] - peakNos[2]);

  car.logPeakPointers(
    peakPtrs[0], peakPtrs[1], peakPtrs[2], peakPtrs[3]);

  return models.gapsPlausible(car);
}


bool PeakStructure::findLastTwoOfFourWheeler(
  const CarModels& models,
  const unsigned start, 
  const unsigned end,
  const bool rightGapPresent,
  const vector<unsigned>& peakNos, 
  const vector<Peak *>& peakPtrs,
  CarDetect& car) const
{
  car.setLimits(start, end);

  if (rightGapPresent)
    car.logRightGap(end - peakNos[1]);

  car.logCore(0, 0, peakNos[1] - peakNos[0]);

  car.logPeakPointers(
    nullptr, nullptr, peakPtrs[0], peakPtrs[1]);

  if (! models.sideGapsPlausible(car))
    return false;

  // As we don't have a complete car, we'll at least require the
  // right bogey gap to be similar to something we've seen.

  if (models.rightBogeyPlausible(car))
    return true;
  else
  {
    cout << "Error: Suspect right bogey gap: ";
    cout << car.strGaps(0) << endl;
    cout << "Checked against " << models.size() << " ref cars\n";
    return false;
  }
}


bool PeakStructure::findLastThreeOfFourWheeler(
  const CarModels& models,
  const unsigned start, 
  const unsigned end,
  const vector<unsigned>& peakNos, 
  const vector<Peak *>& peakPtrs,
  CarDetect& car,
  unsigned& numWheels) const
{
  car.setLimits(start, end);

  car.logCore(
    0, 
    peakNos[1] - peakNos[0],
    peakNos[2] - peakNos[1]);

  car.logRightGap(end - peakNos[2]);

  car.logPeakPointers(
    nullptr, peakPtrs[0], peakPtrs[1], peakPtrs[2]);

  if (! models.sideGapsPlausible(car))
    return false;

  // As we don't have a complete car, we'll at least require the
  // right bogey gap to be similar to something we've seen.

  if (! models.rightBogeyPlausible(car))
  {
    cout << "Error: Suspect right bogey gap: ";
    cout << car.strGaps(0) << endl;
    cout << "Checked against " << models.size() << " ref cars\n";
    return false;
  }

  if (! car.midGapPlausible())
  {
    // Drop the first peak.
    numWheels = 2;
    car.logPeakPointers(
      nullptr, nullptr, peakPtrs[1], peakPtrs[2]);

    cout << "Dropping very first peak as too close\n";
    return true;
  }
  else
  {
    numWheels = 3;
    return true;
  }

  return true;
}


void PeakStructure::markWheelPair(
  Peak& p1,
  Peak& p2,
  const string& text) const
{
  if (text != "")
    cout << text << " wheel pair at " << p1.getIndex() + offset <<
      "-" << p2.getIndex() + offset << "\n";

  p1.select();
  p2.select();

  p1.markWheel(WHEEL_LEFT);
  p2.markWheel(WHEEL_RIGHT);
}


void PeakStructure::fixTwoWheels(
  Peak& p1,
  Peak& p2) const
{
  // Assume the two rightmost wheels, as the front ones were lost.
  PeakStructure::markWheelPair(p1, p2, "");

  p1.markBogey(BOGEY_RIGHT);
  p2.markBogey(BOGEY_RIGHT);
}


void PeakStructure::fixThreeWheels(
  Peak& p1,
  Peak& p2,
  Peak& p3) const
{
  // Assume the two rightmost wheels, as the front one was lost.
  p1.select();
  p1.markWheel(WHEEL_RIGHT);
  PeakStructure::markWheelPair(p2, p3, "");

  p1.markBogey(BOGEY_LEFT);
  p2.markBogey(BOGEY_RIGHT);
  p3.markBogey(BOGEY_RIGHT);
}


void PeakStructure::fixFourWheels(
  Peak& p1,
  Peak& p2,
  Peak& p3,
  Peak& p4) const
{
  PeakStructure::markWheelPair(p1, p2, "");
  PeakStructure::markWheelPair(p3, p4, "");

  p1.markBogey(BOGEY_LEFT);
  p2.markBogey(BOGEY_LEFT);
  p3.markBogey(BOGEY_RIGHT);
  p4.markBogey(BOGEY_RIGHT);
}


void PeakStructure::updateCars(
  CarModels& models,
  vector<CarDetect>& cars,
  CarDetect& car) const
{
  unsigned index;
  float distance;

  if (! car.isPartial() &&
      PeakStructure::matchesModel(models, car, index, distance))
  {
    car.logStatIndex(index);
    car.logDistance(distance);

    models.add(car, index);
    cout << "Recognized car " << index << endl;
  }
  else
  {
    car.logStatIndex(models.size());

    models.append();
    models += car;
    cout << "Created new car\n";
  }

  cars.push_back(car);
}


void PeakStructure::makeProfile(const vector<Peak *>& peakPtrs)
{
  PeakStructure::resetProfile();
  for (auto& peakPtr: peakPtrs)
  {
    if (peakPtr->isLeftBogey())
    {
      if (peakPtr->isLeftWheel())
        profile.bogeyWheels[0]++;
      else if (peakPtr->isRightWheel())
        profile.bogeyWheels[1]++;
      else
        THROW(ERR_ALGO_PEAK_STRUCTURE, "Left bogey has no wheel mark");

      profile.sumGreat++;
    }
    else if (peakPtr->isRightBogey())
    {
      if (peakPtr->isLeftWheel())
        profile.bogeyWheels[2]++;
      else if (peakPtr->isRightWheel())
        profile.bogeyWheels[3]++;
      else
        THROW(ERR_ALGO_PEAK_STRUCTURE, "Right bogey has no wheel mark");

      profile.sumGreat++;
    }
    else if (peakPtr->isLeftWheel())
    {
      profile.wheels[0]++;
      profile.sumGreat++;
    }
    else if (peakPtr->isRightWheel())
    {
      profile.wheels[1]++;
      profile.sumGreat++;
    }
    else
    {
      const string stars = peakPtr->stars();
      if (stars == "***")
      {
        profile.stars[0]++;
        profile.sumGreat++;
      }
      else if (stars == "**")
        profile.stars[1]++;
      else if (stars == "**")
        profile.stars[2]++;
      else
        profile.stars[3]++;
    }
    profile.sum++;
  }
}


string PeakStructure::strProfile(unsigned origin) const
{
  stringstream ss;
  ss << "PROFILE " << origin << ": " <<
    PeakStructure::x(profile.bogeyWheels[0]) << " " <<
    PeakStructure::x(profile.bogeyWheels[1]) << " " <<
    PeakStructure::x(profile.bogeyWheels[2]) << " " <<
    PeakStructure::x(profile.bogeyWheels[3]) << ", wh " <<
    PeakStructure::x(profile.wheels[0]) << " " <<
    PeakStructure::x(profile.wheels[1]) << ", q " <<
    PeakStructure::x(profile.stars[0]) << " " <<
    PeakStructure::x(profile.stars[1]) << " " <<
    PeakStructure::x(profile.stars[2]) << " " <<
    PeakStructure::x(profile.stars[3]) << " (" <<
    profile.sumGreat << " of " <<
    profile.sum << ")\n";
  return ss.str();
}


void PeakStructure::updateFourPeaks(
  vector<Peak *>& peakPtrsNew,
  vector<Peak *>& peakPtrsUnused) const
{
  peakPtrsNew[0]->markBogey(BOGEY_LEFT);
  peakPtrsNew[0]->markWheel(WHEEL_LEFT);

  peakPtrsNew[1]->markBogey(BOGEY_LEFT);
  peakPtrsNew[1]->markWheel(WHEEL_RIGHT);

  peakPtrsNew[2]->markBogey(BOGEY_RIGHT);
  peakPtrsNew[2]->markWheel(WHEEL_LEFT);

  peakPtrsNew[3]->markBogey(BOGEY_RIGHT);
  peakPtrsNew[3]->markWheel(WHEEL_RIGHT);

  for (auto& pp: peakPtrsNew)
    pp->select();

  for (auto& pp: peakPtrsUnused)
    pp->unselect();
}


bool PeakStructure::findCarsNew(
  const unsigned start,
  const unsigned end,
  const bool leftGapPresent,
  const bool rightGapPresent,
  CarModels& models,
  vector<CarDetect>& cars,
  list<Peak *>& candidates) // const
{
  vector<Peak *> peakPtrs;
  vector<unsigned> peakNos;

  for (auto& cand: candidates)
  {
    const unsigned index = cand->getIndex();
    if (index > end)
      break;
    if (index < start)
      continue;

    peakPtrs.push_back(cand);
    peakNos.push_back(index);
  }

  PeakStructure::makeProfile(peakPtrs);


  if (profile.sumGreat == 4)
  {
    vector<Peak *> peakPtrsNew;
    vector<Peak *> peakPtrsUnused;
    vector<unsigned> peakNosNew;

    for (unsigned i = 0; i < peakPtrs.size(); i++)
    {
      Peak * pp = peakPtrs[i];
      if (pp->isLeftWheel() || 
          pp->isRightWheel() ||
          pp->greatQuality())
      {
        peakPtrsNew.push_back(pp);
        peakNosNew.push_back(peakNos[i]);
      }
      else
        peakPtrsUnused.push_back(pp);
    }

    if (peakPtrsNew.size() != 4)
      THROW(ERR_ALGO_PEAK_STRUCTURE, "Not 4 great peaks");

    CarDetect car;
    if (PeakStructure::findFourWheeler(models, start, end,
        leftGapPresent, rightGapPresent, peakNosNew, peakPtrsNew, car))
    {
      PeakStructure::updateCars(models, cars, car);
      PeakStructure::updateFourPeaks(peakPtrsNew, peakPtrsUnused);
      return true;
    }
    else
    {
      cout << "Failed to find car among 4 great peaks\n";
      cout << PeakStructure::strProfile(source);
      return false;
    }
  }

  cout << PeakStructure::strProfile(source);
  return false;
}


string PeakStructure::x(const unsigned v) const
{
  if (v == 0)
    return "-";
  else
    return to_string(v);
}


bool PeakStructure::findCars(
  const unsigned start,
  const unsigned end,
  const bool leftGapPresent,
  const bool rightGapPresent,
  CarModels& models,
  vector<CarDetect>& cars,
  list<Peak *>& candidates) // const
{
  if (findCarsNew(start, end, leftGapPresent, rightGapPresent,
      models, cars, candidates))
    return true;


  vector<unsigned> peakNos;
  vector<Peak *> peakPtrs;
  unsigned notTallCount = 0;
  vector<unsigned> notTallNos;
  bool notTallFlag = false;

  unsigned i = 0;
  for (auto& cand: candidates)
  {
    const unsigned index = cand->getIndex();
    if (index >= start && index <= end)
    {
      if (! cand->isSelected())
      {
        notTallFlag = true;
        notTallNos.push_back(peakNos.size());
        notTallCount++;
      }

      peakNos.push_back(index);
      peakPtrs.push_back(cand);
      
    }
    i++;
  }

  unsigned startLocal = start;
  unsigned endLocal = end;
  bool leftFlagLocal = leftGapPresent;
  bool rightFlagLocal = rightGapPresent;

  unsigned np = peakNos.size();
  bool firstRightFlag = true;
  const unsigned npOrig = np;
cout << "Starting out with " << np << " peaks\n";

  while (np >= 8)
  {
    // Might be multiple cars.  Work backwards, as this works best
    // for the leading cars.  Use groups of four good peaks.
    i = np;
    unsigned seen = 0;
    while (i > 0 && seen < 4)
    {
      i--;
      if (peakPtrs[i]->goodQuality())
        seen++;
    }

    if (seen == 4)
    {
      vector<unsigned> peakNosNew;
      vector<Peak *> peakPtrsNew;
      for (unsigned j = i; j < np; j++)
      {
        if (peakPtrs[j]->goodQuality())
        {
          peakNosNew.push_back(peakNos[j]);
          peakPtrsNew.push_back(peakPtrs[j]);
        }

        peakNos.pop_back();
        peakPtrs.pop_back();
      }
cout << "Trying a multi-peak split, np was " << np << ", is " << i << endl;
cout << "sizes " << peakNosNew.size() << ", " << peakPtrsNew.size() <<
  ", " << peakNos.size() << ", " << peakPtrs.size() << endl;
      np = i;

      if (peakPtrsNew.front() == peakPtrs.front())
      {
        startLocal = start;
        leftFlagLocal = leftGapPresent;
      }
      else
      {
        startLocal = peakPtrsNew[0]->getIndex();
        leftFlagLocal = false;
      }

      if (firstRightFlag)
      {
        endLocal = end;
        rightFlagLocal = rightGapPresent;
        firstRightFlag = false;
      }
      else
      {
        endLocal = peakPtrsNew[3]->getIndex();
        rightFlagLocal = false;
      }
cout << "Range is " << startLocal+offset << "-" << 
  endLocal+offset << endl;

      CarDetect car;
      if (PeakStructure::findFourWheeler(models, startLocal, endLocal,
        leftFlagLocal, rightFlagLocal, peakNosNew, peakPtrsNew, car))
      {
matrix[source][0]++;
        PeakStructure::fixFourWheels(
          * peakPtrsNew[0], * peakPtrsNew[1], 
          * peakPtrsNew[2], * peakPtrsNew[3]);
        for (unsigned k = 4; k < peakPtrsNew.size(); k++)
        {
          peakPtrsNew[k]->markNoWheel();
          peakPtrsNew[k]->unselect();
        }
        
        PeakStructure::updateCars(models, cars, car);
        // Then go on
      }
      else
      {
matrix[source][10]++;
cout << "Failed multi-peak split " << start+offset << "-" << end+offset << endl;
        return false;
      }
    }
    else
    {
      // Fall through to the other chances.
cout << "Falling through with remaining peaks, np " << np << endl;
      break;
    }
  }

  if (npOrig > np)
  {
    // Kludge
    if (peakPtrs.size() == 0)
      THROW(ERR_NO_PEAKS, "No residual peaks?");

        startLocal = start;
        leftFlagLocal = leftGapPresent;

        endLocal = peakPtrs.back()->getIndex();
        rightFlagLocal = false;

cout << "Fell through to " << startLocal+offset << "-" << endLocal+offset << endl;

  notTallCount = 0;
  notTallNos.clear();
  notTallFlag = false;
  peakNos.clear();
  peakPtrs.clear();

  i = 0;
  for (auto& cand: candidates)
  {
    const unsigned index = cand->getIndex();
    if (index >= startLocal && index <= endLocal)
    {
      if (! cand->isSelected())
      {
        notTallFlag = true;
        notTallNos.push_back(peakNos.size());
        notTallCount++;
      }

      peakNos.push_back(index);
      peakPtrs.push_back(cand);
      
    }
    i++;
  }

  np = peakNos.size();


  }


  if (np <= 2)
  {
matrix[source][11]++;
    cout << "Don't know how to do this yet: " << np << "\n";
    return false;
  }

  if (! leftFlagLocal &&
      (np == 4 || np == 5) && 
      notTallFlag && 
      notTallCount == np-3)
  {
cout << "4-5 leading wheels: Attempting to drop down to 3: " << np << "\n";

    if (np == 5)
    {
      peakNos.erase(peakNos.begin() + notTallNos[1]);
      peakPtrs.erase(peakPtrs.begin() + notTallNos[1]);
    }

    peakNos.erase(peakNos.begin() + notTallNos[0]);
    peakPtrs.erase(peakPtrs.begin() + notTallNos[0]);

    CarDetect car;
    unsigned numWheels;
    if (! PeakStructure::findLastThreeOfFourWheeler(models,
        startLocal, endLocal, peakNos, peakPtrs, car, numWheels))
    {
matrix[source][12]++;
      return false;
    }

    if (numWheels == 3)
    {
matrix[source][1]++;
      PeakStructure::fixThreeWheels(
        * peakPtrs[0], * peakPtrs[1], * peakPtrs[2]);
    }
    else
    {
      // Doesn't happen.
matrix[source][13]++;
      return false;
    }

    PeakStructure::updateCars(models, cars, car);

    return true;
  }

  if (np == 4)
  {
    CarDetect car;
    if (PeakStructure::findFourWheeler(models, startLocal, endLocal,
        leftFlagLocal, rightFlagLocal, peakNos, peakPtrs, car))
    {
matrix[source][2]++;
        PeakStructure::fixFourWheels(
          * peakPtrs[0], * peakPtrs[1], * peakPtrs[2], * peakPtrs[3]);
if (peakPtrs.size() != 4)
  cout << "ERROR1 " << peakPtrs.size() << endl;

      PeakStructure::updateCars(models, cars, car);

      return true;
    }
    
    // Give up unless this is the first car.
    if (leftFlagLocal)
    {
matrix[source][14]++;
      return false;
    }

    // Try with only the acceptable-quality peaks.
    vector<unsigned> peakNosNew;
    vector<Peak *> peakPtrsNew;
    for (i = 0; i < peakPtrs.size(); i++)
    {
      if (peakPtrs[i]->acceptableQuality())
      {
        peakNosNew.push_back(peakNos[i]);
        peakPtrsNew.push_back(peakPtrs[i]);
      }
    }

    if (peakPtrsNew.size() <= 1)
    {
      cout << "Failed first cars: Only " << peakPtrsNew.size() <<
        " peaks\n";
matrix[source][15]++;
      return false;
    }
    else if (peakPtrsNew.size() == 2)
    {
      if (! PeakStructure::findLastTwoOfFourWheeler(models,
          startLocal, endLocal, rightFlagLocal, peakNosNew, peakPtrsNew, 
          car))
      {
        cout << "Failed first cars: " << peakPtrsNew.size() <<
          " peaks\n";
matrix[source][16]++;
        return false;
      }

matrix[source][3]++;
      cout << "Hit first car with 2 peaks\n";
      PeakStructure::fixTwoWheels(* peakPtrsNew[0], * peakPtrsNew[1]);

      for (i = 0; i < peakPtrs.size(); i++)
      {
        if (! peakPtrs[i]->acceptableQuality())
        {
          peakPtrs[i]->markNoWheel();
          peakPtrs[i]->unselect();
        }
      }

      PeakStructure::updateCars(models, cars, car);

      return true;
    }


    // Try again without the first peak.
cout << "Trying again without the very first peak of first car\n";
    Peak * peakErased = peakPtrs.front();
    peakNos.erase(peakNos.begin());
    peakPtrs.erase(peakPtrs.begin());

    unsigned numWheels;
    if (! PeakStructure::findLastThreeOfFourWheeler(models,
        startLocal, endLocal, peakNos, peakPtrs, car, numWheels))
    {
matrix[source][17]++;
      return false;
    }

    if (numWheels == 3)
    {
matrix[source][4]++;
      PeakStructure::fixThreeWheels(
        * peakPtrs[0], * peakPtrs[1], * peakPtrs[2]);
    }
    else
    {
      // Doesn't happen.
matrix[source][18]++;
      return false;
    }

if (peakPtrs.size() != 3)
  cout << "ERRORb " << peakPtrs.size() << endl;

      peakErased->markNoWheel();
      peakErased->unselect();

    PeakStructure::updateCars(models, cars, car);

    return true;
  }

  if (np >= 5)
  {
    // Might be several extra peaks (1-2).
    vector<unsigned> peakNosNew;
    vector<Peak *> peakPtrsNew;

    float qmax = 0.f;
    unsigned qindex = 0;

    unsigned j = 0;
    for (auto& peakPtr: peakPtrs)
    {
      if (peakPtr->getQualityShape() > qmax)
      {
        qmax = peakPtr->getQualityShape();
        qindex = j;
      }

      if (peakPtr->goodQuality())
      {
        peakNosNew.push_back(peakPtr->getIndex());
        peakPtrsNew.push_back(peakPtr);
      }
      j++;
    }

    if (peakNosNew.size() != 4)
    {
      // If there are five and the middle one stands out, try that.
      if (np == 5 && qindex == 2)
      {
cout << "General try with " << np << " didn't fit -- drop the middle?" <<
 endl;
        peakNos.erase(peakNos.begin() + 2);
        peakPtrs.erase(peakPtrs.begin() + 2);
      
        CarDetect car;
    cout << "Trying the car anyway\n";
        if (! PeakStructure::findFourWheeler(models, startLocal, endLocal,
            leftFlagLocal, rightFlagLocal, peakNos, peakPtrs, car))
        {
    cout << "Failed the car: " << np << "\n";
matrix[source][19]++;
          return false;
        }

matrix[source][5]++;
        PeakStructure::fixFourWheels(
          * peakPtrs[0], * peakPtrs[1], * peakPtrs[2], * peakPtrs[3]);
if (peakPtrs.size() != 4)
  cout << "ERROR2 " << peakPtrs.size() << endl;

        PeakStructure::updateCars(models, cars, car);
        return true;
      }
      else
      {
matrix[source][20]++;
        cout << "I don't yet see one car here: " << np << ", " <<
          peakNosNew.size() << endl;

for (auto peakPtr: peakPtrs)
  cout << "index " << peakPtr->getIndex() << ", qs " <<
    peakPtr->getQualityShape() << endl;

        return false;
      }
    }

    CarDetect car;
    // Try the car.
cout << "Trying the car\n";
    if (! PeakStructure::findFourWheeler(models, startLocal, endLocal,
        leftFlagLocal, rightFlagLocal, peakNosNew, peakPtrsNew, car))
    {
matrix[source][21]++;
cout << "Failed the car: " << np << "\n";
      return false;
    }

    PeakStructure::fixFourWheels(
      * peakPtrsNew[0], * peakPtrsNew[1], 
      * peakPtrsNew[2], * peakPtrsNew[3]);
    for (unsigned k = 4; k < peakPtrsNew.size(); k++)
    {
      peakPtrsNew[k]->markNoWheel();
      peakPtrsNew[k]->unselect();
    }

matrix[source][6]++;
    PeakStructure::updateCars(models, cars, car);
    return true;
  }

  if (! leftFlagLocal && np == 3)
  {
cout << "Trying 3-peak leading car\n";
    // The first car, only three peaks.
    if (notTallCount == 1 && 
      ! peakPtrs[0]->isSelected())
    {
cout << "Two talls\n";
      // Skip the first peak.
      peakNos.erase(peakNos.begin() + notTallNos[0]);
      peakPtrs.erase(peakPtrs.begin() + notTallNos[0]);

      // TODO In all these erase's, need to unsetSeed and markNoWheel.
      // There should be a more elegant, central way of doing this.

      CarDetect car;
      if (! PeakStructure::findLastTwoOfFourWheeler(models,
          startLocal, endLocal, rightFlagLocal, peakNos, peakPtrs, car))
      {
matrix[source][22]++;
cout << "Failed\n";
        return false;
      }

matrix[source][7]++;
      PeakStructure::fixTwoWheels(* peakPtrs[0], * peakPtrs[1]);

      PeakStructure::updateCars(models, cars, car);

      return true;
    }
  }

matrix[source][23]++;
  cout << "Don't know how to do this yet: " << np << ", notTallCount " <<
    notTallCount << "\n";
  return false;
}


void PeakStructure::findWholeCars(
  CarModels& models,
  vector<CarDetect>& cars,
  list<Peak *>& candidates) const
{
  // Set up a sliding vector of running peaks.
  vector<list<Peak *>::iterator> runIter;
  vector<Peak *> runPtr;
  auto cit = candidates.begin();
  for (unsigned i = 0; i < 4; i++)
  {
    while (cit != candidates.end() && ! (* cit)->isSelected())
      cit++;

    if (cit == candidates.end())
      THROW(ERR_NO_PEAKS, "Not enough peaks in train");
    else
    {
      runIter.push_back(cit);
      runPtr.push_back(* cit);
    }
  }

  while (cit != candidates.end())
  {
    if (runPtr[0]->isLeftWheel() && runPtr[0]->isLeftBogey() &&
        runPtr[1]->isRightWheel() && runPtr[1]->isLeftBogey() &&
        runPtr[2]->isLeftWheel() && runPtr[2]->isRightBogey() &&
        runPtr[3]->isRightWheel() && runPtr[3]->isRightBogey())
    {
      // Fill out cars.
      cars.emplace_back(CarDetect());
      CarDetect& car = cars.back();

      car.logPeakPointers(
        runPtr[0], runPtr[1], runPtr[2], runPtr[3]);

      car.logCore(
        runPtr[1]->getIndex() - runPtr[0]->getIndex(),
        runPtr[2]->getIndex() - runPtr[1]->getIndex(),
        runPtr[3]->getIndex() - runPtr[2]->getIndex());

      car.logStatIndex(0); 

      // Check for gap in front of run[0].
      unsigned leftGap = 0;
      if (runIter[0] != candidates.begin())
      {
        Peak * prevPtr = * prev(runIter[0]);
        if (prevPtr->isBogey())
        {
          // This rounds to make the cars abut.
          const unsigned d = runPtr[0]->getIndex() - prevPtr->getIndex();
          leftGap  = d - (d/2);
        }
      }

      // Check for gap after run[3].
      unsigned rightGap = 0;
      auto postIter = next(runIter[3]);
      if (postIter != candidates.end())
      {
        Peak * nextPtr = * postIter;
        if (nextPtr->isBogey())
          rightGap = (nextPtr->getIndex() - runPtr[3]->getIndex()) / 2;
      }


      if (! car.fillSides(leftGap, rightGap))
      {
        cout << car.strLimits(offset, "Incomplete car");
        cout << "Could not fill out any limits: " <<
          leftGap << ", " << rightGap << endl;
      }

      models += car;

    }

    for (unsigned i = 0; i < 3; i++)
    {
      runIter[i] = runIter[i+1];
      runPtr[i] = runPtr[i+1];
    }

    do
    {
      cit++;
    } 
    while (cit != candidates.end() && ! (* cit)->isSelected());
    runIter[3] = cit;
    runPtr[3] = * cit;
  }
}


void PeakStructure::findWholeInnerCars(
  CarModels& models,
  vector<CarDetect>& cars,
  list<Peak *>& candidates) // const
{
  const unsigned csize = cars.size(); // As cars grows in the loop
  for (unsigned cno = 0; cno+1 < csize; cno++)
  {
    const unsigned end = cars[cno].endValue();
    const unsigned start = cars[cno+1].startValue();
    if (end == start)
      continue;

source = 0;
    if (PeakStructure::findCars(end, start, true, true, 
      models, cars, candidates))
      PeakStructure::printRange(end, start, "Did intra-gap");
    else
      PeakStructure::printRange(end, start, "Didn't do intra-gap");
  }
}


void PeakStructure::findWholeFirstCar(
  CarModels& models,
  vector<CarDetect>& cars,
  list<Peak *>& candidates) // const
{
  const unsigned u1 = candidates.front()->getIndex();
  const unsigned u2 = cars.front().startValue();
  if (u1 < u2)
  {
source = 1;
    if (PeakStructure::findCars(u1, u2, false, true, 
        models, cars, candidates))
      PeakStructure::printRange(u1, u2, "Did first whole-car gap");
    else
      PeakStructure::printRange(u1, u2, "Didn't do first whole-car gap");
    cout << endl;
  }
}


void PeakStructure::findWholeLastCar(
  CarModels& models,
  vector<CarDetect>& cars,
  list<Peak *>& candidates) // const
{
  const unsigned u1 = cars.back().endValue();
  const unsigned u2 = candidates.back()->getIndex();
  if (u2 > u1)
  {
source = 2;
    if (PeakStructure::findCars(u1, u2, true, false, 
        models, cars, candidates))
      PeakStructure::printRange(u1, u2, "Did last whole-car gap");
    else
      PeakStructure::printRange(u1, u2, "Didn't do last whole-car gap");
    cout << endl;
  }
}


void PeakStructure::updateCarDistances(
  const CarModels& models,
  vector<CarDetect>& cars) const
{
  for (auto& car: cars)
  {
    unsigned index;
    float distance;
    if (! PeakStructure::matchesModel(models, car, index, distance))
    {
      cout << "WARNING: Car doesn't match any model.\n";
      index = 0;
      distance = 0.f;
    }

    car.logStatIndex(index);
    car.logDistance(distance);
  }
}


void PeakStructure::markCars(
  CarModels& models,
  vector<CarDetect>& cars,
  list<Peak *>& candidates,
  const unsigned offsetIn)
{
  offset = offsetIn;

  models.append(); // Make room for initial model
  PeakStructure::findWholeCars(models, cars, candidates);

  if (cars.size() == 0)
    THROW(ERR_NO_PEAKS, "No cars?");

  PeakStructure::printCars(cars, "after inner gaps");

  for (auto& car: cars)
    models.fillSides(car);

  // TODO Could actually be multiple cars in vector, e.g. same wheel gaps
  // but different spacing between cars, ICET_DEU_56_N.

  PeakStructure::updateCarDistances(models, cars);

  PeakStructure::printCars(cars, "before intra gaps");

  // Check open intervals.  Start with inner ones as they are complete.

  PeakStructure::findWholeInnerCars(models, cars, candidates);
  PeakStructure::updateCarDistances(models, cars);
  sort(cars.begin(), cars.end());

  PeakStructure::printWheelCount(candidates, "Counting");
  PeakStructure::printCars(cars, "after whole-car gaps");
  PeakStructure::printCarStats(models, "after whole-car inner gaps");

  PeakStructure::findWholeLastCar(models, cars, candidates);
  PeakStructure::updateCarDistances(models, cars);

  PeakStructure::printWheelCount(candidates, "Counting");
  PeakStructure::printCars(cars, "after trailing whole car");
  PeakStructure::printCarStats(models, "after trailing whole car");

  PeakStructure::findWholeFirstCar(models, cars, candidates);
  PeakStructure::updateCarDistances(models, cars);
  sort(cars.begin(), cars.end());

  PeakStructure::printWheelCount(candidates, "Counting");
  PeakStructure::printCars(cars, "after leading whole car");
  PeakStructure::printCarStats(models, "after leading whole car");


  // TODO Check peak quality and deviations in carStats[0].
  // Detect inner and open intervals that are not done.
  // Could be carStats[0] with the right length etc, but missing peaks.
  // Or could be a new type of car (or multiple cars).
  // Ends come later.

  PeakStructure::printWheelCount(candidates, "Returning");
}


void PeakStructure::printPeak(
  const Peak& peak,
  const string& text) const
{
  cout << text << "\n";
  cout << peak.strHeader();
  cout << peak.str(0) << endl;
}


void PeakStructure::printRange(
  const unsigned start,
  const unsigned end,
  const string& text) const
{
  cout << text << " " << start + offset << "-" << end + offset << endl;
}


void PeakStructure::printWheelCount(
  const list<Peak *>& candidates,
  const string& text) const
{
  unsigned count = 0;
  for (auto cand: candidates)
  {
    if (cand->isWheel())
      count++;
  }
  cout << text << " " << count << " peaks" << endl;
}


void PeakStructure::printCars(
  const vector<CarDetect>& cars,
  const string& text) const
{
  if (cars.empty())
    return;

  cout << "Cars " << text << "\n";
  cout << cars.front().strHeaderFull();

  for (unsigned cno = 0; cno < cars.size(); cno++)
  {
    cout << cars[cno].strFull(cno, offset);
    if (cno+1 < cars.size() &&
        cars[cno].endValue() != cars[cno+1].startValue())
      cout << string(56, '.') << "\n";
  }
  cout << endl;
}


void PeakStructure::printCarStats(
  const CarModels& models,
  const string& text) const
{
  cout << "Car stats " << text << "\n";
  cout << models.str();
}


void PeakStructure::printPaths() const
{
  cout << setw(8) << left << "Place" <<
    setw(8) << right << "Inner" <<
    setw(8) << right << "First" <<
    setw(8) << right << "Last" <<
    setw(8) << right << "Sum" <<
    "\n";

  vector<unsigned> sumSource(3);
  vector<unsigned> sumPlace(12);

  for (unsigned j = 0; j < 24; j++)
  {
    if (matrix[0][j] + matrix[1][j] + matrix[2][j] == 0)
      continue;

    cout << setw(8) << left << j <<
      setw(8) << right << matrix[0][j] <<
      setw(8) << right << matrix[1][j] <<
      setw(8) << right << matrix[2][j] <<
      setw(8) << right << matrix[0][j] + matrix[1][j] + matrix[2][j] <<
      "\n";
    sumPlace[0] += matrix[0][j];
    sumPlace[1] += matrix[1][j];
    sumPlace[2] += matrix[2][j];
  }
  cout << string(40, '-') << endl;

  cout << setw(8) << left << "Sum" <<
    setw(8) << right << sumPlace[0] <<
    setw(8) << right << sumPlace[1] <<
    setw(8) << right << sumPlace[2] <<
    "\n";
}

