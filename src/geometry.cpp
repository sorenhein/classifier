#include <iostream>
#include <iomanip>
#include <string>

#include "database/TrainDB.h"
#include "Regress.h"
#include "geometry.h"
#include "align/Alignment.h"

using namespace std;


bool nameMatch(
  const string& fullString,
  const string& searchString)
{
  return (fullString.find(searchString) != string::npos);
}


void dumpResiduals(
  const vector<PeakTime>& times,
  const TrainDB& trainDB,
  const unsigned order,
  const vector<Alignment>& matches,
  const string& heading,
  const string& trainTrue,
  const string& trainSelect,
  const unsigned numWheels)
{
  const unsigned lt = times.size();
  if (lt > numWheels)
  {
    cout << "ERROR: More time samples than wheels?\n";
    return;
  }

  Regress regress;
  vector<double> coeffs(order+1);
  double residuals;
  vector<double> posTrue;
// const unsigned numTrue = db.lookupTrainNumber(trainTrue);
// db.getPerfectPeaks(numTrue, posTrue);

  unsigned mno = 0;
  for (auto& ma: matches)
  {
    mno++;
    const string tname = trainDB.lookupName(ma.trainNo);
    if (! nameMatch(tname, trainSelect))
      continue;
    if (ma.distMatch > 3.)
      continue;

    regress.specificMatch(times, trainDB, ma, coeffs, residuals);

    vector<double> pos(numWheels, 0.);
    for (unsigned i = 0; i < times.size(); i++)
    {
      const double t = times[i].time;
      const int j = ma.actualToRef[i];
      if (j != -1) 
        pos[static_cast<unsigned>(j)] = 
          coeffs[0] + coeffs[1] * t + coeffs[2] * t * t;
    }

    trainDB.getPeakPositions(ma.trainNo, posTrue);

    cout << "SPECTRAIN " << trainTrue << " " << tname << endl;
    cout << heading << "/" << 
      fixed << setprecision(2) << ma.distMatch << "/#" << mno << "\n";

    for (unsigned i = 0; i < pos.size(); i++)
    {
      if (pos[i] == 0.)
        cout << i << ";" << endl;
      else
        cout << i << ";" << fixed << setprecision(4) << 
          pos[i] - posTrue[i] << endl;
    }
    cout << "ENDSPEC\n";
  }
}

