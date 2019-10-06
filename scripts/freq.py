import glob
import matplotlib.pyplot as plt
import numpy as np
import platform
from six.moves import input
import site

if platform.node() == 'CAD04':
  homedir = R"D:\cygwin64\home\heins" + "\\"
else:
  homedir = R"C:\Program Files (x86)\cygwin64\home\s.hein" + "\\"

basedir = homedir + R"GitHub\Trains\src" + "\\"

sfile = R"s07-01-bogie.csv"


def get_info(source):
  contents = [line.rstrip('\n') for line in open(source, "r")]

  series = []
  for c in contents:
    series.append(float(c))

  return series


def plot_series(title, x, series):
  plt.clf()
  plt.title(title)
  plt.plot(x, series, 'b')


def make_sine(ampl, period):
  x = np.arange(0, period)
  y = - ampl * np.cos(x * 2 * np.pi / period)
  return y


def correlate(series, yp):
  ls = len(series)
  lp = len(yp)
  nshifts = ls + 1 - lp

  ssub = np.zeros(lp)
  corr = np.zeros(ls)

  for s in range(nshifts):
    ssub = series[s:s+lp]
    corr[s+lp/2-1] = np.dot(ssub, yp)

  smax = max(series)
  cmax = max(corr)
  corr = (smax / cmax) * corr

  return corr


def sweep(period):
  """Sweep a sine wave of this period over the signal."""

  series = get_info(basedir + sfile)
  print("Read " + str(len(series)) + " points from " + sfile)

  plt.ion()
  plt.show()

  title = "Period " + str(period)
  x = np.arange(0, len(series))
  plot_series(title, x, series)

  xp = np.arange(0, period)
  yp = make_sine(0.5, period)
  plt.plot(xp, yp, 'r')

  corr = correlate(series, yp)
  plt.plot(x, corr, "black")

  # diff = series - corr
  # plt.plot(x, diff, "green")


  plt.draw()
  
