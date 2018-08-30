// This is currently not hooked up to anything, but it's quite nice.
// It's a sliding min/max detector.

#include <deque>

void slidingMinMax(
  const vector<float>& samples,
  const bool maxFlag)
{
  // https://www.nayuki.io/res/
  // sliding-window-minimum-maximum-algorithm/SlidingWindowMinMax.hpp

  const unsigned filterWidth = 1001;
  const unsigned filterMid = (filterWidth-1) >> 1;

  const unsigned ls = samples.size();
  if (ls <= filterWidth)
  {
    cout << "CAN'T COMPENSATE\n";
    return;
  }

  deque<float> deque;
  unsigned i = 0;
  auto tail = samples.cbegin();
  for (const float& val: samples)
  {
    while (! deque.empty() &&
      ((maxFlag && val > deque.back()) ||
      (! maxFlag && val < deque.back())))
      deque.pop_back();

    deque.push_back(val);

    if (maxFlag)
      posStats[i].max = deque.front();
    else
      posStats[i].min = deque.front();
    i++;

    if (i >= filterWidth)
    {
      if (* tail == deque.front())
        deque.pop_front();
      tail++;
    }
  }
}


// This one can do something like a sliding median.

void slidingMedian(const vector<float>& samples)
{
  const unsigned filterWidth = 1001;
  const unsigned filterMid = (filterWidth-1) >> 1;

  const unsigned ls = samples.size();
  if (ls <= filterWidth)
  {
    cout << "CAN'T COMPENSATE\n";
    return;
  }

  Mediator * mediator = MediatorNew(filterWidth);

  // Fill up the part that doesn't directly create values.
  for (unsigned i = 0; i < filterMid; i++)
    MediatorInsert(mediator, samples[i]);

  // Fill up the large middle part with (mostly) full data.
  for (unsigned i = filterMid; i < ls-filterMid; i++)
  {
    MediatorInsert(mediator, samples[i]);
    GetMediatorStats(mediator, &posStats[i-filterMid]);
  }

  free(mediator);

  // Special case for the tail.  (Median really should have a
  // way to push out old elements without pushing in new ones.)

  mediator = MediatorNew(filterWidth);

  for (unsigned i  = 0; i < filterMid; i++)
    MediatorInsert(mediator, samples[ls-1-i]);

  for (unsigned i = 0; i < filterMid; i++)
  {
    MediatorInsert(mediator, samples[ls-1-filterMid-i]);
    GetMediatorStats(mediator, &posStats[ls-1-i]);
  }

  free(mediator);
}

