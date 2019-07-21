/*
     Algorithmic parameters governing the overall behavior.
 */

#include <vector>
#include <list>


////////////////////////////////////////////////////////////////////////
//                                                                    //
//                            Physics                                 //
//                                                                    //
////////////////////////////////////////////////////////////////////////

// Natural constant (does vary with location/altitude!
#define G_FORCE 9.8f

// 10 m/s is 36 km/h
#define MS_TO_KMH 3.6


////////////////////////////////////////////////////////////////////////
//                                                                    //
//                            Filters                                 //
//                                                                    //
////////////////////////////////////////////////////////////////////////

struct FilterDouble
{
  vector<double> numerator;
  vector<double> denominator;
};

struct FilterFloat
{
  vector<float> numerator;
  vector<float> denominator;
};

// The following filters are reasonable for a sample rate of 2000 Hz.

// Fifth-order high-pass Butterworth filter with low cut-off.
// The filter is run forwards and backwards in order to
// get linear phase (like in Python's filtfilt, but without
// the padding or other clever stuff).  It eliminates slow/DC
// components.
//
// from scipy import signal
// num, denom = signal.butter(5, 0.005, btype='high')
// numpy.set_printoptions(precision=16)

const FilterDouble Butterworth5HPF_double =
{
  // Also equals 0.9749039346036602 * (1, -5, 10, -10, 5, -1)
  {
    0.9749039346036602,
    -4.8745196730183009,
    9.7490393460366018,
    -9.7490393460366018,
    4.8745196730183009,
    -0.9749039346036602
  },

  {
    1.,
    -4.9491681155566063,
    9.7979620071891631,
    -9.698857155930046,
    4.8005009469355944,
    -0.9504376817056966
  }
};

const FilterFloat Butterworth5HPF_float =
{
  {
    0.9749039346036602f,
    -4.8745196730183009f,
    9.7490393460366018f,
    -9.7490393460366018f,
    4.8745196730183009f,
    -0.9749039346036602f
  },

  {
    1.,
    -4.9491681155566063f,
    9.7979620071891631f,
    -9.698857155930046f,
    4.8005009469355944f,
    -0.9504376817056966f
  }
};

// Fifth-order low-pass Butterworth filter.  Eliminates
// high frequencies which probably contribute to jitter and drift.
//
// num, denom = signal.butter(5, 0.04, btype = 'low')


const FilterDouble Butterworth5LPF_double =
{
  // Also equals 8.0423564219711682e-07 * (1, 5, 10, 10, 5, 1)
  {
    8.0423564219711682e-07,
    4.0211782109855839e-06,
    8.0423564219711678e-06,
    8.0423564219711678e-06,
    4.0211782109855839e-06,
    8.0423564219711682e-07
  },

  {
    1.,
    -4.5934213998076876,
    8.4551152235101341,
   -7.7949183180444468,
   3.5989027680539127,
   -0.6656525381713611
  }
};

const FilterFloat Butterworth5LPF_float =
{
  {
    8.0423564219711682e-07f,
    4.0211782109855839e-06f,
    8.0423564219711678e-06f,
    8.0423564219711678e-06f,
    4.0211782109855839e-06f,
    8.0423564219711682e-07f
  },

  {
    1.f,
    -4.5934213998076876f,
    8.4551152235101341f,
   -7.7949183180444468f,
   3.5989027680539127f,
   -0.6656525381713611f
  }
};


////////////////////////////////////////////////////////////////////////
//                                                                    //
//                           Peak gaps                                //
//                                                                    //
////////////////////////////////////////////////////////////////////////

// A gap between two peaks is considered the same if it is
// within this tolerance.

#define GAP_TOLERANCE 0.1f
#define GAP_FACTOR (1.f + GAP_TOLERANCE)
#define GAP_FACTOR_SQUARED GAP_FACTOR * GAP_FACTOR


////////////////////////////////////////////////////////////////////////
//                                                                    //
//                             Cars                                   //
//                                                                    //
////////////////////////////////////////////////////////////////////////

// The following constants help to eliminate peak constellations that
// are obviously not cars.  They are nowhere near good enough to avoid
// false positives, but that is not their purpose.

// If we interpolate an entire car without having any end gaps,
// the end gap should at least not be too large relative to the
// estimated bogie gap.

#define CAR_NO_BORDER_FACTOR 3.0f

// The inter-car gap should be larger than effectively zero.

#define CAR_SMALL_BORDER_FACTOR 0.1f

// If we have a car-sized internal, the lengths should match.

#define CAR_LEN_FACTOR_GREAT 0.05f
#define CAR_LEN_FACTOR_GOOD 0.05f

// Expect a short car length to be within these factors of a 
// typical car.

#define CAR_SHORT_FACTOR_LO 0.5f
#define CAR_SHORT_FACTOR_HI 0.9f


////////////////////////////////////////////////////////////////////////
//                                                                    //
//                          Completions                               //
//                                                                    //
////////////////////////////////////////////////////////////////////////

// When looking for peak completions, go +/- this factor of the 
// estimated bogie distance.

#define CAR_BOGIE_TOLERANCE 0.3f

struct CompletionEntry
{
  bool flag;
  float limit;
};

struct CompletionCase
{
  CompletionEntry dSquared; // Upper limit; others are lower limits
  CompletionEntry dSquaredRatio;
  CompletionEntry qualShapeRatio;
  CompletionEntry qualPeakRatio;
};

// These are cases for which one car completion is deemed to dominate
// another.  
// In the first one, there is almost-dominance except that the peak 
// quality shape is slightly inferior.  
// In the second one, the absolute squared distance is low and there
// is dominance in the qualities, so it doesn't matter that the 
// relative distance is slightly off.

static list<CompletionCase> CompletionCases =
{
  { {false, 0.f}, {true, 3.f}, {true, 0.9f}, {true, 1.2f} },
  { {true, 50.f}, {true, 0.6667f}, {true, 1.25f}, {true, 1.25f} }
};

