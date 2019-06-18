/*
     Algorithmic parameters governing the overall behavior.
 */


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
// are obviously not car.  They are nowhere near good enough to avoid
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

// Tolerance when targeting peaks from a model.

#define CAR_BOGIE_TOLERANCE 0.3f
