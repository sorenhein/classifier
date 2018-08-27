import glob
import matplotlib.pyplot as plt
import numpy as np

sensors = [
  '062493', '063848', '063905', '065330', '066221', 
  '066254', '066270', '067138', '075149', '078663',
  '078630', '078655', '078663', '078747', '078754',
  '078770', '078796', '078804', '078812', '078960',
  '081857', '082863', '084992', '085320', '085478',
  '099743', '101630', '101648', '101796', '106346',
  '391705', '391710', '391711', '391718', '391731']

basedir = R"C:\Program Files (x86)\cygwin64\home\s.hein\mini_dataset_v012\data\sensors" + "\\"

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


def get_user_input(curr, limit, matched):
  """Gets user input for display: n, p, N, or number."""
  val = raw_input("Input: ")
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
      
  if c < 0:
    c = 0
  elif c >= limit:
    c = limit-1
  return c, 0 
       

def dualpane(sensor, rawdir, compdir):
  """Show original signal and partial, matched signal."""
  rawdirslash = "\\" + rawdir + "\\"
  compdirslash = "\\" + compdir + "\\"
  
  rawdir = basedir + sensor + rawdirslash
  matchdir = basedir + sensor + compdirslash

  rawlist = glob.glob(rawdir + "*.dat")
  matchlist = glob.glob(matchdir + "*.dat")

  i = 0
  rawdict = {}
  rawoffsets = [-1] * len(rawlist)

  for r in rawlist:
    newraw, newoff = remove_offset(r)
    rawdict[newraw] = i
    rawoffsets[i] = newoff
    i += 1

  print("Have " + str(len(rawlist)) + " raw items, " + str(len(matchlist)) + " matched items")

  matched = [""] * len(rawlist)
  compoffsets = [-1] * len(rawlist)
  for mm in matchlist:
    m, offset = remove_offset(mm)
    m = m.replace(compdirslash, rawdirslash)

    if m in rawdict:
      matched[rawdict[m]] = mm
      compoffsets[rawdict[m]] = offset
    else:
      print("did not match " + m)
      
  plt.ion()
  plt.show()
  
  curr = -1
  l = len(rawlist)
  while True:
    curr, done = get_user_input(curr, l, matched)
    if done == 1:
      break
    if done == -1:
      continue

    rp = rawlist[curr].find(rawdirslash);
    title = rawlist[curr][(rp+len(rawdirslash)):] +  " (no. " + str(curr) + ")"
    
    rawdata = np.fromfile(rawlist[curr], dtype = np.float32)
    print("Read " + str(len(rawdata)) + " from " + rawlist[curr])
    
    plt.clf()
    plt.title(title)
    
    o = rawoffsets[curr]
    x = np.arange(o, o+len(rawdata))
    plt.plot(x, rawdata, 'b')

    if matched[curr] != "":
      matchdata = np.fromfile(matched[curr], dtype = np.float32)
      o = compoffsets[curr]
      x = np.arange(o, o+len(matchdata))
      plt.plot(x, matchdata, 'r')
    
    plt.draw()
    plt.pause(0.001)


