import glob
import matplotlib.pyplot as plt

def dualpane(sensor, compdir):
  """Show original signal and partial, matched signal."""
  basedir = "C:\Program Files (x86)\cygwin64\home\s.hein\mini_dataset_v012\data/sensors/"
  rawdir = basedir + sensor + "/raw"
  matchdir = basedir + sensor + compdir

  rawlist = glob.glob(rawdir + "/*.dat")
  matchlist = glob.glob(matchdir + "/*.dat")

  i = 0
  for r in rawlist:
    rawdict[r] = i++

  for mm in matchlist:
    m = mm
    mp = m.find(".dat")
    if mp == -1:
      print("Odd match name: " + r)
      continue

    m = m(:rp)

    offset = 0
    mp = m.find("_offset_")
    if mp >= 0:
      offset = int(m(:mp+8))
      m = m(:mp)

    m.replace("/" + compdir + "/". "/raw/")
    m += ".dat"

    if m in rawdict:
      matched[rawdict[m]] = mm

  curr = -1
  l = rawlist.len()
  while True:
    val = input()
    if val == "q":
      break
    elif val == "n":
      curr++
      if curr >= l:
        curr = l-1
    elif val == "p":
      curr--
      if curr < 0:
        curr = 0
    elif val < 0 || val >= l:
      print("Value " + val + " out of range")
      continue
    else
      curr = int(val)

    rp = rawlist[curr].find("/raw/");
    plt.title(rp(rp+5: + "(no. " + curr + ")")
    rawdata = np.fromfile(rawlist[curr], dtype = float32)
    if matched[curr] == 0:
      plt.plot(rawdata(:500))
      continue

    matchdata = np.fromfile(matched[curr], dtype = float32)
    ml = matchdata.len()
    x = np.arange(ml)
    plt.plot(x, rawdata(:ml), x, ml)


