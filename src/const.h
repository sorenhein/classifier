/*
     Algorithmic parameters governing the overall behavior.
 */

#include <vector>
#include <list>


////////////////////////////////////////////////////////////////////////
//                                                                    //
//                            General                                 //
//                                                                    //
////////////////////////////////////////////////////////////////////////

#define SEPARATOR ";"

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
//                           Transients                               //
//                                                                    //
////////////////////////////////////////////////////////////////////////

// A transient must be among the first TRANSIENT_RANGE runs.

#define TRANSIENT_RANGE 5

// A transient must cover at least TRANSIENT_MIN_LENGTH seconds.

#define TRANSIENT_MIN_DURATION 0.01

// A transient must have a g average of at least TRANSIENT_MIN_AVG.

#define TRANSIENT_MIN_AVG 0.3

// The specific onset of a transient consists of at least
// TRANSIENT_SMALL_RUN samples trending towards the zero line.

#define TRANSIENT_SMALL_RUN 3

// The transient has to drop off with a time constant of at most this
// many seconds.

#define TRANSIENT_MAX_TAU 0.025f

// If the average deviation away from the zero line of four
// consecutive samples exceeds this factor times the value of the
// transient peak, then we consider that there is a wheel and we
// should stop the transient.

#define TRANSIENT_LARGE_BUMP 0.4

// A transient should decline by at least a factor TRANSIENT_RATIO_FULL
// between beginning and end.

#define TRANSIENT_RATIO_FULL 3.

// A transient should decline by at least a factor TRANSIENT_RATIO_MID
// between beginning and end (roughly the square root of the other).

#define TRANSIENT_RATIO_MID 1.7


////////////////////////////////////////////////////////////////////////
//                                                                    //
//                          Quiet periods                             //
//                                                                    //
////////////////////////////////////////////////////////////////////////

// We divide up the signal into intervals of this duration in seconds.

#define QUIET_DURATION_COARSE 0.05
#define QUIET_DURATION_FINE 0.005

// An interval is very quiet if (mean, sdev) is below both these,
// measured in g.  This will depend in general on the sensor.

#define MEAN_VERY_QUIET 0.2
#define SDEV_VERY_QUIET 0.06

// The interval is somewhat quiet if (mean, sdev) is below both these.

#define MEAN_SOMEWHAT_QUIET 0.2
#define SDEV_SOMEWHAT_QUIET 0.2

// The interval is not a complete write-off if one parameter is below
// the somewhat-quiet parameter and the other is below these.

#define MEAN_QUIET_LIMIT 0.3
#define SDEV_QUIET_LIMIT 0.3

// Stop looking for quiet intervals if we get this many non-quiet
// ones in a row.

#define NUM_NON_QUIET_RUNS 2

// We can overcome a non-quiet (red) interval if if it followed by
// this many quiet ones.

#define NUM_QUIET_FOLLOWERS 4

// When writing output, pad this much time in seconds beyond the 
// detected end.

#define QUIET_DURATION_PADDING 0.5


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

// The same filter as a series of second-order filters.  As the order
// is 5, one of the sections is of order 1.  All the zeroes are at DC.
// One of them will cancel out against an integrator.
//
// signal.butter(5, 0.005, btype='high', output='sos')

const vector<FilterFloat> Butterworth5HPF_SOS_float =
{
  { 
    { 0.9749039346036602f, -0.9749039346036602f,  0.f                },
    { 1.f                , -0.9844141274160969f,  0.f                }
  },
  { 
    { 1.f                , -2.f                ,  1.f                },
    { 1.f                , -1.9746602956354231f,  0.9749039346327976f},
  },
  { 
    { 1.f                , -2.f                ,  1.f                },
    { 1.f                , -1.9900936925050869f,  0.9903392357172509f}
  }
};

const vector<FilterDouble> Butterworth5HPF_SOS_double =
{
  // #1
  { 
    { 0.9749039346036602 , -0.9749039346036602 ,  0.                 },
    { 1.                 , -0.9844141274160969 ,  0.                 }
  },
  // #2
  { 
    { 1.                 , -2.                 ,  1.                 },
    { 1.                 , -1.9746602956354231 ,  0.9749039346327976 },
  },
  // #3
  { 
    { 1.                 , -2.                 ,  1.                 },
    { 1.                 , -1.9900936925050869 ,  0.9903392357172509 }
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


////////////////////////////////////////////////////////////////////////
//                                                                    //
//                           Alignment                                //
//                                                                    //
////////////////////////////////////////////////////////////////////////

// Regulates which alignment is preferred based on distances etc.
// Probably needs to be updated and simplified.

// Only apply special rules to match distances below this threshold.
// Otherwise just go by overall distance.

#define ALIGN_DISTMATCH_THRESHOLD 3.

// Must be this much better to beat other alignment.

#define ALIGN_DISTMATCH_BETTER 0.7

// Must not be worse by more than this to still beat other alignment.

#define ALIGN_DIST_NOT_WORSE 2.


////////////////////////////////////////////////////////////////////////
//                                                                    //
//                           Regression                               //
//                                                                    //
////////////////////////////////////////////////////////////////////////

// Normalization factor to make residuals from different-length trains
// comparable.  Residuals are normalized to a train of this length
// in meters.  The value of this constant doesn't really matter except 
// that it influences the absolute value of the residuals.

#define TRAIN_REF_LENGTH 200.f
