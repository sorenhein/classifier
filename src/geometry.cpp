#include <iostream>
#include <iomanip>
#include <string>

#include "Database.h"
#include "Regress.h"

using namespace std;


bool nameMatch(
  const string& fullString,
  const string& searchString)
{
  return (fullString.find(searchString) != string::npos);
}


void dumpResiduals(
  const vector<PeakTime>& times,
  const Database& db,
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
  vector<double> pos(numWheels, 0.);
  vector<PeakPos> posTrue;
// const unsigned numTrue = db.lookupTrainNumber(trainTrue);
// db.getPerfectPeaks(numTrue, posTrue);

  unsigned mno = 0;
  for (auto& ma: matches)
  {
    mno++;
    const string tname = db.lookupTrainName(ma.trainNo);
    if (! nameMatch(tname, trainSelect))
      continue;
    if (ma.distMatch > 3.)
      continue;

    regress.specificMatch(times, db, ma, coeffs, residuals);

    for (unsigned i = 0; i < times.size(); i++)
    {
      const double t = times[i].time;
      const unsigned j = ma.actualToRef[i];
      pos[j] = coeffs[0] + coeffs[1] * t + coeffs[2] * t * t;
    }

    db.getPerfectPeaks(ma.trainNo, posTrue);

    cout << "SPECTRAIN " << trainTrue << " " << tname << endl;
    cout << heading << "/" << 
      fixed << setprecision(2) << ma.distMatch << "/#" << mno << "\n";

    for (unsigned i = 0; i < pos.size(); i++)
    {
      if (pos[i] == 0.)
        cout << i << ";" << endl;
      else
        cout << i << ";" << fixed << setprecision(4) << 
          pos[i] - posTrue[i].pos << endl;
    }
    cout << "ENDSPEC\n";
  }
}

