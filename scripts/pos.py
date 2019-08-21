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

rawdirslash = "\\pos\\"
timedirslash = "\\peak\\times\\"
valuedirslash = "\\peak\\values\\"
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
       

def get_user_input(curr, limit, rawlist, matched):
  """Gets user input for display: n, p, N, or number."""
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


def pos(sensorNo, n = 0):
  """Show original signal and partial, matched signal."""
  sensor = sensors[sensorNo]

  rawdir = basedir + sensor + rawdirslash
  timedir = basedir + sensor + timedirslash
  valuedir = basedir + sensor + valuedirslash

  rawfull = rawdir + "*" + extension
  timefull = timedir + "*" + extension
  valuefull = valuedir + "*" + extension

  rawlist = glob.glob(rawfull)
  timelist = glob.glob(timefull)
  valuelist = glob.glob(valuefull)

  rawdict = {}
  rawoffsets = {}
  set_rawdict(rawlist, rawdict, rawoffsets)

  timematched = match_up(rawdict, timelist, rawdirslash, timedirslash)
  valuematched = match_up(rawdict, valuelist, rawdirslash, valuedirslash)

  plt.ion()
  plt.show()
  
  curr = guess_number(n, rawlist)
  l = len(rawlist)
  while True:
    rp = rawlist[curr].find(rawdirslash);
    title = rawlist[curr][(rp+len(rawdirslash)):] +  " (no. " + str(curr) + ")"
    
    rawdata = np.fromfile(rawlist[curr], dtype = np.float32)
    lr = len(rawdata)
    print("Read " + str(lr) + " points from " + rawlist[curr])
    
    plt.clf()
    plt.title(title)
    
    o = rawoffsets[curr]
    x = np.arange(o, o + lr)
    plt.plot(x, rawdata, 'b')

    if timematched[curr] != "":
      matchdata = read_pieces(timematched[curr], valuematched[curr], o+lr)

      x = np.arange(0, len(matchdata))
      plt.plot(x, matchdata, 'r')
    
    plt.draw()
    plt.pause(0.001)

    curr, done = get_user_input(curr, l, rawlist, timematched)
    if done == 1:
      break
    if done == -1:
      continue


