import matplotlib.pyplot as plt
import platform
from six.moves import input
import math
import re

sensors = [
  '062493', '063848', '063905', '065330', '066221', 
  '066254', '066270', '067138', '075149', '078622',
  '078630', '078655', '078663', '078747', '078754',
  '078770', '078796', '078804', '078812', '078960',
  '081857', '082863', '084992', '085320', '085478',
  '099743', '101630', '101648', '101796', '106346',
  '391705', '391710', '391711', '391718', '391731']

trains = [
  'ICE1_DEU_56_N', 
  'ICE1_old_CHE_56_N',
  'ICE2_DEU_32_N', 'ICE2_DEU_32_R', 
  'ICE2_DEU_32A_N', 'ICE2_DEU_32A_R', 
  'ICE2_DEU_32B_N', 'ICE2_DEU_32B_R',
  'ICE2_DEU_64_N', 'ICE2_DEU_64_R', 
  'ICE3_DEU_32_N', 'ICE3_DEU_32_R', 
  'ICE4_DEU_28_N', 'ICE4_DEU_28_R', 
  'ICE4_DEU_48_N', 'ICE4_DEU_48_R',
  'ICET_56_N',
  'ICET_DEU_20_N', 
  'ICET_DEU_28_N', 
  'ICET_DEU_48_N', 'ICET_DEU_48_R',
  'ICET_DEU_56_N',
  'MERIDIAN_DEU_08_N', 
  'MERIDIAN_DEU_14_N', 
  'MERIDIAN_DEU_22_N',
  'MERIDIAN_DEU_28_N', 
  'MERIDIAN_DEU_42_N',
  'SBAHN423_DEU_10_N', 
  'SBAHN423_DEU_20_N', 
  'SBAHN423_DEU_30_N',
  'X2_SWE_28_N', 'X2_SWE_28_R', 
  'X2_SWE_56_N', 'X2_SWE_56_R',
  'X31_SWE_12_N', 'X31_SWE_24_N', 'X31_SWE_36_N',
  'X55_SWE_16_N',
  'X61_SWE_10_N', 'X61_SWE_20_N',
  'X74_SWE_14_N', 'X74_SWE_14_R', 
  'X74_SWE_28_N', 'X74_SWE_28_R' ]

if platform.node() == 'CAD04':
  homedir = R"D:\cygwin64\home\heins" + "\\"
else:
  homedir = R"C:\Program Files (x86)\cygwin64\home\s.hein" + "\\"

basedir = homedir + R"GitHub/Trains/src" + "\\"
basename = "dist.txt"

axles = dict()
for t in trains:
  axles[t] = int(re.findall('\d+', t)[-1])


def guess_number(n, rlist):
  """Guess the index corresponding to a number."""
  
  s = str(n)
  if len(s) < 4:
    return n
  
  for i in range(len(rlist)):
    if s in rlist[i]:
      return i

  return n
       

def get_user_input(curr, rawlist):
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
    while c < limit:
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


def sensor_lookup(name):
  """Looks up name and returns number."""
  for i in range(len(sensors)):
    if sensors[i] == name:
      return i
  return -1


def load_file():
  """Loads scatter plots of distance and train speed."""
  contents = [line.rstrip('\n') for line in open(basedir + basename, "r")]

  # states:
  # 0: waiting for sensor
  # 1: waiting for train
  # 2: waiting for data line or empty line
  state = 0
  sensor = ""
  train = ""
  speeds = {}
  distances = {}
  norms = {}
  normsqrts = {}
  for c in contents:
    if state == 0:
      sensor = c
      state = 1
    elif state == 1:
      train = c
      state = 2
    else:
      if c == "":
        state = 0
      else:
        speed, dist = c.split(",", 1)
        fdist = float(dist)
        fspeed = float(speed)
        if not sensor in speeds:
          speeds[sensor] = {}
          distances[sensor] = {}
          norms[sensor] = {}
          normsqrts[sensor] = {}
        if not train in speeds[sensor]:
          speeds[sensor][train] = list()
          distances[sensor][train] = list()
          norms[sensor][train] = list()
          normsqrts[sensor][train] = list()
        speeds[sensor][train].append(fspeed)
        distances[sensor][train].append(fdist)
        norms[sensor][train].append(fdist / axles[train])
        normsqrts[sensor][train].append(math.sqrt(fdist / axles[train]))

  return speeds, distances, norms, normsqrts


def plot_y_label(ptype):
  y = ""
  if ptype == 0:
    y = "raw distance (m^2)"
  elif ptype == 1:
    y = "distance per axle (m^2)"
  elif ptype == 2:
    y = "distance per axle (m)"
  plt.ylabel(y)
  

def plot_selected(speed, dist, norm, normsqrt, label, ptype):
  if ptype == 0:
    # Raw distance
    plt.scatter(speed, dist, label = label)
  elif ptype == 1:
    # Per-axle distance in meters^2
    plt.scatter(speed, norm, label = label)
  elif ptype == 2:
    # Per-axle in meters
    plt.scatter(speed, normsqrt, label = label)


def splot(sensor, ptype = 0):
  """Show diagram of sensor errors.  0/1: raw/normalized by axes."""
  speeds = {}
  distances = {}
  norms = {}
  normsqrts = {}
  speeds, distances, norms, normsqrts = load_file()

  plt.ion()
  plt.show()

  sno = guess_number(sensor, sensors)

  while True:
    if sno >= len(sensors):
      print(sno, "out of range")
    else:
      sname = sensors[sno]
      if not sname in speeds:
        print("No data for sensor", sname)
      else:
        plt.clf()
        title = sensors[sno] + ", no. " + str(sno)
        plt.title(title)
        plot_y_label(ptype)

        smax = 0
        for train in speeds[sname]:
          plot_selected(speeds[sname][train], distances[sname][train],
            norms[sname][train], normsqrts[sname][train], train, ptype)

          snew = max(speeds[sname][train])
          if snew > smax:
            smax = snew

        smax = 10 * (int(smax / 10.) + 1)
        plt.gca().set_xlim([0, smax])
        plt.legend()
        plt.draw()
        plt.pause(0.001)

    sno, done = get_user_input(sno, sensors)
    if done == 1:
      break
    if done == -1:
      continue
  

def tplot(train, ptype = 0):
  """Show diagram of sensor errors.  0/1: raw/normalized by axes."""
  speeds = {}
  distances = {}
  speeds, distances, norms, normsqrts = load_file()

  plt.ion()
  plt.show()

  tno = guess_number(train, trains)

  while True:
    if tno >= len(trains):
      print(tno, "out of range")
    else:
      tname = trains[tno]
      if not tname in trains:
        print("No data for train", tname)
      else:
        plt.clf()
        title = tname + ", no. " + str(tno)
        plt.title(title)
        plot_y_label(ptype)

        smax = 0
        for sname in speeds:
          if not tname in speeds[sname]:
            continue

          slabel = sname + " (" + str(sensor_lookup(sname)) + ")"

          plot_selected(speeds[sname][tname], distances[sname][tname],
            norms[sname][tname], normsqrts[sname][tname], slabel, ptype)

          snew = max(speeds[sname][tname])
          if snew > smax:
            smax = snew

        smax = 10 * (int(smax / 10.) + 1)
        plt.gca().set_xlim([0, smax])
        plt.legend()
        plt.draw()
        plt.pause(0.001)

    tno, done = get_user_input(tno, trains)
    if done == 1:
      break
    if done == -1:
      continue
  
