import glob
import matplotlib.pyplot as plt
import numpy as np
import platform
from six.moves import input
from scipy import signal

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

extension = ".dat"


def get_index(curr, rawlist):
  if c < 1000:
    return 0, 0
  i = 0
  for r in rawlist:
    mp = r.find(val)
    if mp >= 0:
      return i, 1
    i += 1


def get_user_input(curr, limit, rawlist):
  """Gets user input for display: n, p, N, or number."""
  val = input("Input: ")
  c = curr
  if val == "q":
    return 0, 1
  elif val == "n":
    c += 1
  elif val == "N":
    c = limit
  elif val == "p":
    c -= 1
  else:
    try:
      c = int(val)
      if c > 1000:
        c, flag = get_index(val, rawlist)
        if flag == 0:
          print("Value " + val + " not recognized")
          return 0, -1
    except ValueError:
      print("Value " + val + " not recognized")
      return 0, -1
      
  if c < 0:
    c = 0
  elif c >= limit:
    c = limit-1
  return c, 0 
       

def run_filter(b, a, x, text):
  energy0 = sum(i * i for i in x)
  # y = signal.filtfilt(b, a, x, method="gust")
  y = signal.filtfilt(b, a, x)
  energy1 = sum(i * i for i in y)
  print(text + ": Energy " + str(round(energy0, 2)) + 
    " to " + str(round(energy1, 2)) + ", " + 
    str(round(100. * energy1/energy0, 0)) + "%")
  return y


def integrate(x):
  y = x
  i = 1
  while i < x.size:
    y[i] += y[i-1]
    i = i + 1
  return y


def cond(sensor = 0, num = 0, time = -1,
  lowcut = 0.005, highcut = 1, num_iter = 2):
  """Show conditioned signal."""

  if highcut != 1:
    bhigh, ahigh = signal.butter(5, highcut, btype = 'low')
  blow, alow = signal.butter(5, lowcut, btype = 'high')

  rawdir = basedir + sensors[sensor] + "\\raw\\"
  rawlist = glob.glob(rawdir + "*.dat")
  print("Have " + str(len(rawlist)) + " traces")

  plt.ion()
  plt.show()
  
  if time == -1:
    curr = num
  else:
    curr, flag = get_index(time, rawlist)
    if flag == 0:
      print("Value " + time + " not recognized")
      return

  l = len(rawlist)
  while True:
    rawdata = np.fromfile(rawlist[curr], dtype = np.float32)
    print("Read " + str(len(rawdata)) + " from " + rawlist[curr])
    
    title = "Sensor " + sensors[sensor] + ", no. " + str(curr) + \
      ", low " + str(round(lowcut, 3)) + ", high " + str(round(highcut, 3))
    plt.clf()
    plt.title(title)

    if highcut != 1:
      rawdata = run_filter(bhigh, ahigh, rawdata, "No HF")
    rawdata = integrate(rawdata)
    rawdata = run_filter(blow, alow, rawdata, "No DC")

    if highcut != 1:
      rawdata = run_filter(bhigh, ahigh, rawdata, "No HF")
    rawdata = integrate(rawdata)
    rawdata = run_filter(blow, alow, rawdata, "No DC")
    
    plt.plot(np.arange(0, len(rawdata)), rawdata, 'b')
    
    plt.draw()
    plt.pause(0.001)

    curr, done = get_user_input(curr, l, rawlist)
    if done == 1:
      return
    if done == -1:
      continue

