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

class Files(object):
  raw = []
  times = []
  values = []

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
extension = ".dat"


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


def get_level(source):
  values = np.fromfile(source, dtype = np.float32)
  return sum(values) / len(values)


def get_info(source):
  # f = open(source, "r")
  # contents = f.readLines()
  # f.close()
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
      print "Bad tag:", tag
      raise SystemExit

  return info


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

  return o + lr


def plot_composite(times, values, m, color):
  """Plot a composite of values vs. times as a complete trace."""
  if times != "":
    matchdata = read_pieces(times, values, m)
    plt.plot(np.arange(0, len(matchdata)), matchdata, color)


def plot_box(times, cars, info, level, color, colorMiss):
  """Plot the complete stick diagram."""
  ax = plt.gca()
  basex, basey = ax.transData.transform((0, 0))
  stepx, stepy = ax.transData.transform((info.diam/2, 1.))
  oneRadius = stepx - basex
  oneY = stepy - basey
  print "oneRadius", oneRadius, "oneY", oneY

  phis = np.linspace(0, 2*np.pi, 65)
  for i in range(len(times)):
    t = times[i]
    c = cars[i]
    print "Time", t, "car", c
    x = t + (info.diam/2) * np.cos(phis)
    y = level + (oneRadius / oneY) * np.sin(phis)
    if c > 1000:
      # Show as a miss
      plt.plot(x, y, colorMiss)
    elif c > 0:
      # Show normally
      plt.plot(x, y, color)



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
  matches.times = match_up(rawdict, files.times, rawdir, timedir)
  matches.values = match_up(rawdict, files.values, rawdir, valuedir)

  plt.ion()
  plt.show()
  
  curr = guess_number(n, files.raw)
  while True:
    m = plot_raw(files.raw, rawoffsets, rawdir, curr)
    plot_composite(matches.times[curr], matches.values[curr], m, 'r')

    plt.draw()
    plt.pause(0.001)

    curr, done = get_user_input(curr, files.raw, matches.times)
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

  files = Files()
  files.raw = glob.glob(sensordir + rawdir + "*" + extension)
  files.values = glob.glob(sensordir + valuedir + "*" + extension)
  files.times = glob.glob(sensordir + boxtimes + "*" + extension)
  files.cars = glob.glob(sensordir + boxcars + "*" + extension)
  files.info = glob.glob(sensordir + boxinfo + "*" + extension)

  rawdict = {}
  rawoffsets = {}
  set_rawdict(files.raw, rawdict, rawoffsets)

  matches = Files()
  matches.values = match_up(rawdict, files.values, rawdir, valuedir)
  matches.times = match_up(rawdict, files.times, rawdir, boxtimes)
  matches.cars = match_up(rawdict, files.cars, rawdir, boxcars)
  matches.info = match_up(rawdict, files.info, rawdir, boxinfo)

  plt.ion()
  plt.show()
  
  curr = guess_number(n, files.raw)
  while True:
    m = plot_raw(files.raw, rawoffsets, rawdir, curr)

    if matches.values[curr] == "":
      print "No peaks"
      continue

    level = get_level(matches.values[curr])
    print "Level", level

    if matches.info[curr] == "":
      print "No info file"
      continue

    info = get_info(matches.info[curr])
    print "Train", info.train
    print "Speed", info.speed
    print "Accel", info.accel
    print "Dist", info.dist
    print "Diam", info.diam

    times = np.fromfile(matches.times[curr], dtype = np.uint32)
    cars = np.fromfile(matches.cars[curr], dtype = np.uint32)

    plot_box(times, cars, info, level, 'r', 'black')

    plt.draw()
    plt.pause(0.001)

    curr, done = get_user_input(curr, files.raw, matches.times)
    if done == 1:
      break
    if done == -1:
      continue
  
