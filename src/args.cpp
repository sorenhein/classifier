#include <iostream>
#include <iomanip>
#include <string>

#include "args.h"

using namespace std;


struct OptEntry
{
  string shortName;
  string longName;
  unsigned numArgs;
};

#define TRAIN_NUM_OPTIONS 6

static const OptEntry OPT_LIST[TRAIN_NUM_OPTIONS] =
{
  {"c", "control", 1},
  {"p", "pick", 1},
  {"s", "stats", 1},
  {"a", "append", 0},
  {"w", "writing", 1},
  {"v", "verbose", 1}
};

static string shortOptsAll, shortOptsWithArg;

static int getNextArgToken(
  int argc,
  char * argv[]);

static void setDefaults();


void usage(
  const char base[])
{
  string basename(base);
  const size_t l = basename.find_last_of("\\/");
  if (l != string::npos)
    basename.erase(0, l+1);

  cout <<
    "Usage (simple): " << basename << " control-file\n" <<
    "\n" <<
    "Usage (elaborate): " << basename << " [options]\n\n" <<
    "-c, --control s    Input control file.\n" <<
    "\n" <<
    "-p, --pick s       If set, pick the first file among those set in\n" <<
    "                   the input control file that contains 's'.\n" <<
    "\n" <<
    "-s, --stats s      Stats output file.\n" <<
    "\n" <<
    "-a, --append       If present, stats file is not rewritten.\n" <<
    "\n" <<
    "-w, -writing n     Binary output files (default: 0x30).  Bits:\n" <<
    "                   0x01: transient\n" <<
    "                   0x02: back\n" <<
    "                   0x04: front\n" <<
    "                   0x08: speed\n" <<
    "                   0x10: pos\n" <<
    "                   0x20: peak\n" <<
    "                   0x40: outline\n" <<
    "\n" <<
    "-v, -verbose n     Verbosity (default: 0x0).  Bits:\n" <<
    "                   0x01: (tbd)\n" <<
    "                   0x02: (tbd)\n" <<
    "                   0x04: (tbd)\n" <<
    endl;
}


static int nextToken = 1;
static char * optarg;

static int getNextArgToken(
  int argc,
  char * argv[])
{
  // 0 means done, -1 means error.

  if (nextToken >= argc)
    return 0;

  string str(argv[nextToken]);
  if (str[0] != '-' || str.size() == 1)
    return -1;

  if (str[1] == '-')
  {
    if (str.size() == 2)
      return -1;
    str.erase(0, 2);
  }
  else if (str.size() == 2)
    str.erase(0, 1);
  else
    return -1;

  for (unsigned i = 0; i < TRAIN_NUM_OPTIONS; i++)
  {
    if (str == OPT_LIST[i].shortName || str == OPT_LIST[i].longName)
    {
      if (OPT_LIST[i].numArgs == 1)
      {
        if (nextToken+1 >= argc)
          return -1;

        optarg = argv[nextToken+1];
        nextToken += 2;
      }
      else
        nextToken++;

      return str[0];
    }
  }

  return -1;
}


static void setDefaults(Control& options)
{
  options.controlFile = "";
  options.pickFileString = "";
  options.summaryFile = "comp.csv";
  options.summaryAppendFlag = false;

  options.writingTransient = false;
  options.writingBack = false;
  options.writingFront = false;
  options.writingSpeed = false;
  options.writingPos = true;
  options.writingPeak = true;
  options.writingOutline = false;

  options.verboseABC = false;
}


void printOptions(const Control& options)
{
  cout << left;
  cout << setw(12) << "control" << setw(12) << options.controlFile << "\n";
  cout << setw(12) << "pick" << setw(12) << options.pickFileString << "\n";
  cout << setw(12) << "summary" << setw(12) << options.summaryFile << "\n";

  if (options.summaryAppendFlag)
    cout << setw(12) << "append" << setw(12) << "set" << "\n";
  else
    cout << setw(12) << "append" << setw(12) << "not set" << "\n";
}


void readArgs(
  int argc,
  char * argv[],
  Control& options)
{
  for (unsigned i = 0; i < TRAIN_NUM_OPTIONS; i++)
  {
    shortOptsAll += OPT_LIST[i].shortName;
    if (OPT_LIST[i].numArgs)
      shortOptsWithArg += OPT_LIST[i].shortName;
  }

  if (argc == 1)
  {
    usage(argv[0]);
    exit(0);
  }

  setDefaults(options);

  int c;
  long m;
  bool errFlag = false;
  string stmp;
  char * temp;

  while ((c = getNextArgToken(argc, argv)) > 0)
  {
    switch(c)
    {
      case 'c':
        options.controlFile = optarg;
        break;

      case 'p':
        options.pickFileString = optarg;
        break;

      case 's':
        options.summaryFile = optarg;
        break;

      case 'a':
        options.summaryAppendFlag = true;
        break;

      case 'w':
        m = static_cast<int>(strtol(optarg, &temp, 0));
        if (temp == optarg || temp == '\0' ||
            ((m == LONG_MIN || m == LONG_MAX) && errno == ERANGE))
        {
          cout << "Could not parse writing\n";
          nextToken -= 2;
          errFlag = true;
        }
        
        options.writingTransient = ((m & 0x01) != 0);
        options.writingBack = ((m & 0x02) != 0);
        options.writingFront = ((m & 0x04) != 0);
        options.writingSpeed = ((m & 0x08) != 0);
        options.writingPos = ((m & 0x10) != 0);
        options.writingPeak = ((m & 0x20) != 0);
        options.writingOutline = ((m & 0x40) != 0);
        break;

      case 'v':
        m = static_cast<int>(strtol(optarg, &temp, 0));
        if (temp == optarg || temp == '\0' ||
            ((m == LONG_MIN || m == LONG_MAX) && errno == ERANGE))
        {
          cout << "Could not parse verbose\n";
          nextToken -= 2;
          errFlag = true;
        }
        
        options.verboseABC = ((m & 0x01) != 0);
        // options.verboseDEF = ((m & 0x02) != 0);
        break;

      default:
        cout << "Unknown option\n";
        errFlag = true;
        break;
    }
    if (errFlag)
      break;
  }

  if (errFlag || c == -1)
  {
    cout << "Error while parsing option '" << argv[nextToken] << "'\n";
    cout << "Invoke the program without arguments for help" << endl;
    exit(0);
  }
}

