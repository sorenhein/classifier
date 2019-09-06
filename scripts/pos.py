import glob
import matplotlib.pyplot as plt
import numpy as np
import platform
from six.moves import input
import site

sensors = [
  '062493', '063848', '063905', '065330', '066221', 
  '066254', '066270', '067138', '075149', '078622',
  '078630', '078655', '078663', '078747', '078754',
  '078770', '078796', '078804', '078812', '078960',
  '081857', '082863', '084992', '085320', '085478',
  '099743', '101630', '101648', '101796', '106346',
  '391705', '391710', '391711', '391718', '391731']

if platform.node() == 'CAD04':
  basedir = R"D:\cygwin64\home\heins\mini_dataset_v012\data\sensors" + "\\"
else:
  basedir = R"C:\Program Files (x86)\cygwin64\home\s.hein\mini_dataset_v012\data\sensors" + "\\"

carNumArgs = {
  'fontsize': 15, 
  'ha': 'center', 
  'va': 'center', 
  'weight': 'bold'}

infoArgs = {
  'fontsize': 15, 
  'ha': 'left', 
  'va': 'bottom', 
  'weight': 'bold'}

# The middle one is kind of a heavy orange
trafficColors = [ 'g', '#ff8c00', 'r', 'black' ]

class Files(object):
  raw = []
  reftimes = []
  values = []
  refgrade = []
  peakgrade = []

class Info(object):
  train = ""
  speed = ""
  accel = ""
  dist = ""
  diam = 0.

rawdir = R"\pos" + "\\"
timedir = R"\peak\times" + "\\"
valuedir = R"\peak\values" + "\\"

boxdir = R"\box" + "\\"
timessub = R"\times" + "\\"
carssub = R"\cars" + "\\"
infosub = R"\info" + "\\"
peakgradesub = R"\peakgrade" + "\\"
refgradesub = R"\refgrade" + "\\"

extension = ".dat"

singlepeakgreat = 0.04
singlepeakgood = 0.12


def remove_offset(text):
  """Remove any offset from name."""
  newtext = text
  mp = newtext.find(extension)
  if mp == -1:
    print("Odd name: " + text)
    return 0, 0

  newtext = newtext[:mp]
  newoffset = 0
  mp = newtext.find("_offset_")
  if mp >= 0:
    newoffset = int(newtext[mp+8:])
    newtext = newtext[:mp]
  newtext = newtext + extension
  return newtext, newoffset


def set_rawdict(rawlist, rawdict, rawoffsets):
  i = 0
  for r in rawlist:
    newraw, newoff = remove_offset(r)
    rawdict[newraw] = i
    rawoffsets[i] = newoff
    i += 1

  print("Have " + str(len(rawlist)) + " raw items")


def match_up(rlist, mlist, rdir, mdir):
  """Match up related files in different directories."""
  matched = [""] * len(rlist)
  for mm in mlist:
    m = mm
    m = m.replace(mdir, rdir)

    if m in rlist:
      matched[rlist[m]] = mm
    else:
      print("did not match " + m)
      raise SystemExit
  
  return matched


def guess_number(n, rlist):
  """Guess the index corresponding to a number."""
  
  s = str(n)
  if len(s) < 4:
    return n
  
  for i in range(len(rlist)):
    if s in rlist[i]:
      return i

  return n
       

def get_user_input(curr, rawlist, matched):
  """Gets user input for display: n, p, N, or number."""
  limit = len(rawlist)
  val = input("Input: ")
  c = curr
  if val == "q":
    return 0, 1
  elif val == "n":
    c += 1
  elif val == "N":
    c += 1
    while c < limit and matched[c] == "":
      c = c + 1
  elif val == "p":
    c -= 1
  else:
    try:
      c = int(val)
    except ValueError:
      print("Value " + val + " not recognized")
      return 0, -1
    c = guess_number(c, rawlist)
      
  if c < 0:
    c = 0
  elif c >= limit:
    c = limit-1
  return c, 0 


def read_pieces(timefile, valuefile, lr):
  """Reads times and values and puts them together."""
  times = np.fromfile(timefile, dtype = np.uint32)
  values = np.fromfile(valuefile, dtype = np.float32)
  matchdata = np.zeros(lr)

  for i in range(len(times)):
    matchdata[times[i]] = values[i]

  return matchdata


def get_info(source):
  contents = [line.rstrip('\n') for line in open(source, "r")]

  info = Info()
  for c in contents:
    d = " ".join(c.split())
    tag, val = d.split(" ", 1)
    if tag == "TRAIN_MATCH":
      info.train = val
    elif tag == "WHEEL_DIAMETER":
      info.diam = int(val)
    elif tag == "SPEED":
      info.speed = val
    elif tag == "ACCEL":
      info.accel = val
    elif tag == "DIST_MATCH":
      info.dist = val
    else:
      print("Bad tag:", tag)
      raise SystemExit

  return info


def peak_quality(value):
  if value < 0:
    return 3
  elif value <= singlepeakgreat:
    return 0
  elif value <= singlepeakgood:
    return 1
  else:
    return 2


def make_car_grades(cars, refgrades):
  m = max(cars)
  carcolors = [0] * (m + 1)
  for i in range(len(cars)):
    if cars[i] == 0:
      continue
    q = peak_quality(refgrades[i])
    carcolors[cars[i]] += 10 ** q
  return carcolors


def make_car_colors(cars, refgrades):
  carcolors = make_car_grades(cars, refgrades)
  for i in range(len(carcolors)):
    if carcolors[i] > 0:
      if carcolors[i] < 10 or carcolors[i] == 13:
        carcolors[i] = trafficColors[0]
      elif carcolors[i] < 100:
        carcolors[i] = trafficColors[1]
      elif carcolors[i] < 1000:
        carcolors[i] = trafficColors[2]
      else:
        carcolors[i] = trafficColors[3]
  return carcolors


def plot_raw(rawfiles, rawoffsets, rawdir, curr):
  """Plots the basic trace."""
  rp = rawfiles[curr].find(rawdir);
  title = rawfiles[curr][(rp+len(rawdir)):] +  " (no. " + str(curr) + ")"
    
  rawdata = np.fromfile(rawfiles[curr], dtype = np.float32)
  lr = len(rawdata)
  print("Read " + str(lr) + " points from " + rawfiles[curr])
    
  plt.clf()
  plt.title(title)
    
  o = rawoffsets[curr]
  x = np.arange(o, o + lr)
  plt.plot(x, rawdata, 'b')

  axisdata = [0] * lr
  plt.plot(x, axisdata, 'black')

  return o + lr


def peak_color(value):
  q = peak_quality(value)
  return trafficColors[q]


def plot_values(values, grades, level):
  """Plot vertical lines at the peaks down to the train."""
  # for v in values:
  for i in range(len(values)):
    v = values[i]
    x = [v, v]
    y = [level, 0]
    plt.plot(x, y, peak_color(grades[i]), linewidth = 3)
    # plt.plot(x, y, 'black', linewidth = 3)
    


def plot_composite(reftimes, values, m, color):
  """Plot a composite of values vs. reftimes as a complete trace."""
  if reftimes != "":
    matchdata = read_pieces(reftimes, values, m)
    plt.plot(np.arange(0, len(matchdata)), matchdata, color)


def draw_wheels(reftimes, cars, info, refgrades, level, Yfactor, colorMiss):
  """Draw the wheel circles."""
  # This would be pretty, but doesn't rescale well.
  """
  xhit = []
  xmiss = []
  for i in range(len(reftimes)):
    if cars[i] > 1000:
      xmiss.append(reftimes[i])
    elif cars[i] > 0:
      xhit.append(reftimes[i])
  area = np.pi * oneRadius * oneRadius
  plt.scatter(xhit, [level] * len(xhit), area, color)
  plt.scatter(xmiss, [level] * len(xmiss), area, colorMiss)
  """

  phis = np.linspace(0, 2*np.pi, 65)
  for i in range(len(reftimes)):
    t = reftimes[i]
    c = cars[i]
    x = t + (info.diam/2) * np.cos(phis)
    y = level + Yfactor * np.sin(phis)
    if c > 1000:
      # Show as a miss
      plt.fill(x, y, colorMiss)
    elif c > 0:
      # Show normally
      plt.fill(x, y, peak_color(refgrades[i]))


def draw_horizontal_low(reftimes, cars, info, level, offset, color):
  """Draw the horizontal lines at wheel level."""
  y = [level, level]
  for i in range(len(reftimes)-1):
    c0 = cars[i]
    t0 = reftimes[i] + (info.diam/2 if c0 > 0 else 0)
    c1 = cars[i+1]
    t1 = reftimes[i+1] - (info.diam/2 if c1 > 0 else 0)
    x = [t0, t1]
    plt.plot(x, y, color)
  if cars[0] != 0:
    # Draw a leading horizontal line
    x = [offset, reftimes[0] - info.diam/2]
    plt.plot(x, y, color)


def draw_vertical_lines(reftimes, cars, info, level, Yfactor, color):
  """Draw the vertical lines between cars."""
  heightFull = 15 * Yfactor
  walls = info.diam
  for i in range(len(reftimes)):
    if cars[i] != 0:
      continue
    if i == 0:
      x = [reftimes[i], reftimes[i]]
      y = [level, level + heightFull]
      plt.plot(x, y, color)
    elif i == len(reftimes)-1:
      x = [reftimes[i], reftimes[i]]
      y = [level, level + heightFull]
      plt.plot(x, y, color)
    else:
      x = [reftimes[i] - walls/2, reftimes[i] - walls/2]
      y = [level, level + heightFull]
      plt.plot(x, y, color)
      x = [reftimes[i] + walls/2, reftimes[i] + walls/2]
      plt.plot(x, y, color)


def draw_horizontal_high(reftimes, cars, info, level, offset, Yfactor, color):
  """Draw the horizontal tops within cars."""
  heightFull = 15 * Yfactor
  walls = info.diam

  # Draw the horizontal tops within cars.
  firstFlag = 1
  y = [level + heightFull, level + heightFull]
  for i in range(len(reftimes)):
    if cars[i] != 0:
      continue
    if firstFlag == 1:
      firstBoundary = reftimes[i] + (0 if i == 0 else walls/2)
      prevBoundary = firstBoundary
      firstFlag = 0
    else:
      x = [prevBoundary, reftimes[i] - (0 if i == len(reftimes)-1 else walls/2)]
      plt.plot(x, y, color)
      prevBoundary = reftimes[i] + walls/2
  if cars[0] != 0:
    # Draw a horizontal top line to the left boundary
    x = [offset, firstBoundary - walls]
    plt.plot(x, y, color)


def draw_car_numbers(reftimes, cars, level, offset, Yfactor, carcolors):
  """Write the car numbers."""
  firstFlag = 1
  heightFull = 15 * Yfactor
  textLevel = level + heightFull/2
  for i in range(len(reftimes)):
    if cars[i] != 0:
      continue
    if i > 0 and firstFlag == 1:
      # Partial first car
      pos = (offset + reftimes[i])/2
      carNumArgs['color'] = carcolors[cars[i-1]]
      plt.text(pos, textLevel, cars[i-1], carNumArgs)
    firstFlag = 0
    for j in range(i+1, len(reftimes)):
      if cars[j] != 0:
        continue
      pos = (reftimes[i] + reftimes[j])/2
      carNumArgs['color'] = carcolors[cars[i+1]]
      plt.text(pos, textLevel, cars[i+1], carNumArgs)
      break


def draw_text(info, level, offset, Yfactor, color):
  """Write the text."""
  textStep = 15 * Yfactor
  textLevel = level + textStep/2
  infoArgs['color'] = color

  multitext = info.train + "\ndist = " + info.dist + "\n" + info.speed + "\n" + info.accel
  plt.text(offset, textLevel + textStep, multitext, infoArgs)


def plot_box(reftimes, cars, info, refgrades,
  level, offset, color, colorMiss, carcolors):
  """Plot the complete stick diagram."""
  ax = plt.gca()
  basex, basey = ax.transData.transform((0, 0))
  stepx, stepy = ax.transData.transform((info.diam/2, 1.))
  oneRadius = stepx - basex
  oneY = stepy - basey
  Yfactor = oneRadius / oneY

  draw_wheels(reftimes, cars, info, refgrades, level, Yfactor, colorMiss)
  draw_horizontal_low(reftimes, cars, info, level, offset, color)
  draw_vertical_lines(reftimes, cars, info, level, Yfactor, color)
  draw_horizontal_high(reftimes, cars, info, level, offset, Yfactor, color)
  draw_car_numbers(reftimes, cars, level, offset, Yfactor, carcolors)
  draw_text(info, level, offset, Yfactor, color)


def pos(sensorNo, n = 0):
  """Show original signal and partial, matched signal."""
  sensor = sensors[sensorNo]
  sensordir = basedir + sensor

  files = Files()
  files.raw = glob.glob(sensordir + rawdir + "*" + extension)
  files.times = glob.glob(sensordir + timedir + "*" + extension)
  files.values = glob.glob(sensordir + valuedir + "*" + extension)

  rawdict = {}
  rawoffsets = {}
  set_rawdict(files.raw, rawdict, rawoffsets)

  matches = Files()
  matches.reftimes = match_up(rawdict, files.times, rawdir, timedir)
  matches.values = match_up(rawdict, files.values, rawdir, valuedir)

  plt.ion()
  plt.show()
  
  curr = guess_number(n, files.raw)
  while True:
    m = plot_raw(files.raw, rawoffsets, rawdir, curr)
    plot_composite(matches.reftimes[curr], matches.values[curr], m, 'r')

    plt.draw()
    plt.pause(0.001)

    curr, done = get_user_input(curr, files.raw, matches.reftimes)
    if done == 1:
      break
    if done == -1:
      continue


def box(sensorNo, n = 0, d = "best"):
  """Show stick diagram."""
  sensor = sensors[sensorNo]
  sensordir = basedir + sensor

  boxtimes =  boxdir + d + "\\" + timessub
  boxcars =  boxdir + d + "\\" + carssub
  boxinfo =  boxdir + d + "\\" + infosub
  boxrefgrade =  boxdir + d + "\\" + refgradesub
  boxpeakgrade =  boxdir + d + "\\" + peakgradesub

  files = Files()
  files.raw = glob.glob(sensordir + rawdir + "*" + extension)
  files.peaktimes = glob.glob(sensordir + timedir + "*" + extension)
  files.values = glob.glob(sensordir + valuedir + "*" + extension)
  files.times = glob.glob(sensordir + boxtimes + "*" + extension)
  files.cars = glob.glob(sensordir + boxcars + "*" + extension)
  files.info = glob.glob(sensordir + boxinfo + "*" + extension)
  files.refgrade = glob.glob(sensordir + boxrefgrade + "*" + extension)
  files.peakgrade = glob.glob(sensordir + boxpeakgrade + "*" + extension)

  rawdict = {}
  rawoffsets = {}
  set_rawdict(files.raw, rawdict, rawoffsets)

  matches = Files()
  matches.peaktimes = match_up(rawdict, files.peaktimes, rawdir, timedir)
  matches.values = match_up(rawdict, files.values, rawdir, valuedir)
  matches.reftimes = match_up(rawdict, files.times, rawdir, boxtimes)
  matches.cars = match_up(rawdict, files.cars, rawdir, boxcars)
  matches.info = match_up(rawdict, files.info, rawdir, boxinfo)
  matches.refgrade = match_up(rawdict, files.refgrade, rawdir, boxrefgrade)
  matches.peakgrade = match_up(rawdict, files.peakgrade, rawdir, boxpeakgrade)

  plt.ion()
  plt.show()

  # [left, bottom, width, height]
  plt.axis([0.05, 0.95, 0.05, 0.95])
  
  curr = guess_number(n, files.raw)
  # print("m.peaktimes", len(matches.peaktimes))
  # print("m.values", len(matches.values))
  # print("m.reftimes", len(matches.reftimes))
  # print("m.cars", len(matches.cars))
  # print("m.info", len(matches.info))
  # print("m.refgrade", len(matches.refgrade))
  # print("m.peakgrade", len(matches.peakgrade))

  while True:
    m = plot_raw(files.raw, rawoffsets, rawdir, curr)

    if matches.values[curr] == "" or matches.info[curr] == "":
      print("No peaks or no info file")
    else:
      values = np.fromfile(matches.values[curr], dtype = np.float32)
      peaktimes = np.fromfile(matches.peaktimes[curr], dtype = np.uint32)

      info = get_info(matches.info[curr])

      reftimes = np.fromfile(matches.reftimes[curr], dtype = np.uint32)
      cars = np.fromfile(matches.cars[curr], dtype = np.uint32)
      refgrades = np.fromfile(matches.refgrade[curr], dtype = np.float32)
      peakgrades = np.fromfile(matches.peakgrade[curr], dtype = np.float32)

      if len(values) > len(peakgrades):
        # Could happen that reference train is shorter than peaks seen.
        d = len(values) - len(peakgrades)
        values = values[d:]
        peaktimes = peaktimes[d:]

      carcolors = make_car_colors(cars, refgrades)

      # print("values", len(values))
      # print("peaktimes", len(peaktimes))
      # print("reftimes", len(reftimes))
      # print("cars", len(cars))
      # print("refgrades", len(refgrades))
      # print("peakgrades", len(peakgrades))

      level = sum(values) / len(values)
      plot_values(peaktimes, peakgrades, level)

      plot_box(reftimes, cars, info, refgrades,
        level, rawoffsets[curr], 'r', 'black', carcolors)

      plt.draw()
      plt.pause(0.001)

    curr, done = get_user_input(curr, files.raw, matches.reftimes)
    if done == 1:
      break
    if done == -1:
      continue
  

def carcheck():
  first = dict()
  later = dict()
  firstex = dict()
  laterex = dict()
  d = "best"

  for sensor in sensors:
    sensordir = basedir + sensor
    print("sensor", sensor)

    boxcars =  boxdir + d + "\\" + carssub
    boxrefgrade =  boxdir + d + "\\" + refgradesub

    files = Files()
    files.raw = glob.glob(sensordir + rawdir + "*" + extension)
    files.cars = glob.glob(sensordir + boxcars + "*" + extension)
    files.refgrade = glob.glob(sensordir + boxrefgrade + "*" + extension)

    rawdict = {}
    rawoffsets = {}
    set_rawdict(files.raw, rawdict, rawoffsets)

    matches = Files()
    matches.cars = match_up(rawdict, files.cars, rawdir, boxcars)
    matches.refgrade = match_up(rawdict, files.refgrade, rawdir, boxrefgrade)

    for curr in range(len(files.raw)):
      print("curr", curr)
      
      if matches.cars[curr] == "" or matches.refgrade[curr] == "":
        print("No data")
        continue

      cars = np.fromfile(matches.cars[curr], dtype = np.uint32)
      refgrades = np.fromfile(matches.refgrade[curr], dtype = np.float32)

      carcolors = make_car_grades(cars, refgrades)

      i = 1
      while carcolors[i] == 0:
        i += 1
      s = str(carcolors[i])
      if s in first:
        first[s] += 1
      else:
        first[s] = 1
      firstex[s] = sensor + "-" + str(curr)

      for c in range(i+1, len(carcolors)):
        s = str(carcolors[c])
        if s in later:
          later[s] += 1
        else:
          later[s] = 1
        laterex[s] = sensor + "-" + str(curr)
  
  print("first")
  for f in first:
    print(f, first[f])
  print("later")
  for l in later:
    print(l, later[l])
  print("firstex")
  for f in firstex:
    print(f, firstex[f])
  print("laterex")
  for l in laterex:
    print(l, laterex[l])

