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


def dualpane(sensor, compdir):
  """Show original signal and partial, matched signal."""
  basedir = R"C:\Program Files (x86)\cygwin64\home\s.hein\mini_dataset_v012\data\sensors" + "\\"
  rawdir = basedir + sensor + R"\raw"
  matchdir = basedir + sensor + "\\" + compdir

  rawlist = glob.glob(rawdir + R"\*.dat")
  matchlist = glob.glob(matchdir + R"\*.dat")

  i = 0
  rawdict = {}
  for r in rawlist:
    rawdict[r] = i
    i += 1

  print("Have " + str(len(rawlist)) + " raw items, " + str(len(matchlist)) + " matched items")

  matched = [""] * len(rawlist)
  offsets = [-1] * len(rawlist)
  for mm in matchlist:
    m = mm
    mp = m.find(".dat")
    if mp == -1:
      print("Odd match name: " + r)
      continue

    m = m[:mp]
    offset = 0
    mp = m.find("_offset_")
    if mp >= 0:
      offset = int(m[mp+8:])
      m = m[:mp]

    was = "\\" + compdir + "\\"
    becomes = R"\raw" + "\\"
    m = m.replace(was, becomes)
    m += ".dat"

    if m in rawdict:
      matched[rawdict[m]] = mm
      offsets[rawdict[m]] = offset
    else:
      print("did not match " + m)
      
  plt.ion()
  plt.show()
  
  curr = -1
  l = len(rawlist)
  while True:
    val = raw_input("Input: ")
    if val == "q":
      break
    elif val == "n":
      curr += 1
      if curr >= l:
        curr = l-1
    elif val == "N":
      curr = curr + 1
      while curr < l and matched[curr] == "":
        curr = curr + 1
      if curr >= l:
        curr = l-1
    elif val == "p":
      curr -= 1
      if curr < 0:
        curr = 0
    else:
      try:
        curr = int(val)
      except ValueError:
        print("Value " + val + " not recognized")
        continue
        
      if curr < 0 or curr >= l:
        print("Value " + str(curr) + " out of range")

    rp = rawlist[curr].find("\\raw\\");
    title = rawlist[curr][(rp+5):] +  " (no. " + str(curr) + ")"
    
    rawdata = np.fromfile(rawlist[curr], dtype = np.float32)
    print("Read " + str(len(rawdata)) + " from " + rawlist[curr])
    
    plt.clf()
    plt.title(title)
    if matched[curr] == "":
      # plt.plot(rawdata[:100])q
      
      plt.plot(rawdata)
      plt.draw()
      plt.pause(0.001)
    else:
      matchdata = np.fromfile(matched[curr], dtype = np.float32)
      ml = len(matchdata)
      o = offsets[curr]
      x = np.arange(o, o+ml)
      plt.plot(x, rawdata[o:o+ml], 'b')
      plt.plot(x, matchdata[:ml], 'r')
      plt.draw()
      plt.pause(0.001)


