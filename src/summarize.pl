#!perl

use strict;
use warnings;

# Provide summary and detail of sensor??.txt files.
# - Details go in $SUMMARY/sensor??.txt or .csv
# - Summary goes to stdout in .txt or .csv format

my $SUMMARY_DIR = "summary";
my $SEPARATOR = ";";

my $FORMAT_TXT = 0;
my $FORMAT_CSV = 1;

my $REGRESS_THRESHOLD = 3.;

my $NUM_EMPTY_FNC = 5;
my $NUM_LAST_FNC = 6;

if ($#ARGV < 0)
{
  print "Usage: summarize.pl [CSV] sensor??.txt > file\n";
  exit;
}

my $format;
if (lc($ARGV[0]) eq 'csv')
{
  $format = $FORMAT_CSV;
  shift @ARGV;
}
else
{
  $format = $FORMAT_TXT;
}

my %carTable;
my (@detailsLower, @detailsUpper, @detailsNames);
setCarTables();
setDetails();


# Summary hashes.
my (%errorsSummary, %carsSummary, %fullSummary, %imperfSummary);
my (%detailsSummaryFirst, %detailsSummaryLater);

# Totals.
my (@errorsTotal, @carsTotal, $fullTotal, @imperfTotal);
my $sensor;

my @fracHist;
for my $i (0 .. 100)
{
  $fracHist[$i][$_] = 0 for 0 .. 3;
}

for my $file (@ARGV)
{
  # Per-sensor local house-keeping.
  my @details;
  my ($sname, $date, $time);
  my $fileno = -1;
  
  $sensor = $file;
  $sensor =~ s/.txt//;
  # print "$sensor\n";

  open my $fh, "<", $file or die "Cannot open $file: $!";

  my $fout = ($format == $FORMAT_TXT ? "summary/$file" :
    "summary/$sensor.csv");

  open my $fo, ">", $fout or die "Cannot open $fout: $!";
  print $fo summaryHeader($format), detailHeader($format), "\n";

  # Per-trace arrays.
  my (@errorsTrace, @carsTrace, $fullTrace, @imperfTrace);
  @errorsTrace = (0, 0, 0, 0);
  @imperfTrace = (0, 0, 0, 0);
  $fullTrace = 0;
  my $hasIssues = 0;
  my $numIssues = 0;
  my $regressError = 0.;
  my $regressSeen = 0;
  my $mismatchSeen = 0;
  my $exceptSeen = 0;
  my $peakFrac;
  while (my $line = <$fh>)
  {
    chomp $line;
    $line =~ s///g;

    if ($line =~ /^File/)
    {
      $line =~ /^File.*\/([0-9_]*)_001_channel1.dat:/;
      my @a = split /_/, $1;

      #if ($hasIssues)
      if ($fileno >= 0)
      {
        if ($regressError > $REGRESS_THRESHOLD)
        {
          $errorsTrace[1]++;
          $fracHist[$peakFrac][1]++;
        }
        # elsif (! $regressSeen && $hasIssues)
        elsif (! $regressSeen && ! $mismatchSeen && ! $exceptSeen)
        {
          # Can fail silently.
          $errorsTrace[0]++;
          $fracHist[$peakFrac][0]++;
        }

        $errorsTrace[3] += $numIssues;

        print $fo dividerLine($format);

        print $fo summaryLine($sensor, 
          $time,
          $fileno,
          \@errorsTrace, 
          \@carsTrace, 
          $fullTrace, 
          \@imperfTrace, 
          $format), "\n";

        print $fo forceNewLine($format);

        transferStats(\@errorsTrace, \@carsTrace, 
          $fullTrace, \@imperfTrace);
      }

      $fileno++;
      $date = $a[0];
      $time = $a[1];
      $sname = $a[2];

      @errorsTrace = (0, 0, 0, 0);
      @carsTrace = ();
      $fullTrace = 0;
      @imperfTrace = (0, 0, 0, 0);

      $hasIssues = 0;
      $numIssues = 0;
      $regressSeen = 0;
      $regressError = 0.;
      $mismatchSeen = 0;
      $exceptSeen = 0;
    }
    elsif ($line =~ /^HITS/)
    {
      while (1)
      {
        $line = <$fh>;
        chomp $line;
        $line =~ s///g;
        last if $line =~ /^\s*$/;
  
        $line =~ /^(\d+)\s+(\d+)/;
        $carsTrace[$1] = $2;
      }
    }
    elsif ($line =~ /^IMPERF/)
    {
      $line =~ / (\d+)-(\d+), (\d+)-(\d+)/;
      $imperfTrace[0] = $1;
      $imperfTrace[1] = $2;
      $imperfTrace[2] = $3;
      $imperfTrace[3] = $4;

      if ($1 > 0 || $2 > 0 || $3 > 0 || $4 > 0)
      {
        $hasIssues = 1;
      }
    }
    elsif ($line =~ /^FRAC/)
    {
      $line =~ /^FRAC \d+ \d+ (.+)/;
      $peakFrac = int($1 + 0.5);
      $fracHist[$peakFrac][3]++;
    }
    elsif ($line =~ /^WARNFINAL/)
    {
      $hasIssues = 1;
      while (1)
      {
        $line = <$fh>;
        chomp $line;
        $line =~ s///g;
        last if $line =~ /^\s*$/;
  
        my $mline = $line;

        # Also get the profile lines.
        $line = <$fh>;
        chomp $line;
        $line =~ s///g;
        last if $line =~ /^\s*$/;

        parseLines($mline, $line, \@details);
        print $fo summaryEmpty($format), 
          detailLine(\@details, $format), "\n";
        $numIssues++;
        logDetails($sensor, \@details);
      }
    }
    elsif ($line =~ /MISMATCH/)
    {
      $mismatchSeen = 1;
      $errorsTrace[0]++;
      $errorsTrace[3] += $numIssues;
      $fracHist[$peakFrac][0]++;

      print $fo dividerLine($format);

      print $fo summaryLine($sensor, 
        $time,
        $fileno,
        \@errorsTrace, 
        \@carsTrace, 
        $fullTrace, 
        \@imperfTrace, 
        $format), "\n";

      print $fo forceNewLine($format);

      transferStats(\@errorsTrace, \@carsTrace, 
        $fullTrace, \@imperfTrace);

      @errorsTrace = (0, 0, 0, 0);
      @carsTrace = ();
      $fullTrace = 0;
      @imperfTrace = (0, 0, 0, 0);

      $hasIssues = 0;
      $numIssues = 0;
      $regressSeen = 0;
      $regressError = 0.;
      $exceptSeen = 0;
    }
    elsif ($line =~ /^Regression alignment/)
    {
      $line = <$fh>;
      chomp $line;
      $line =~ s///g;
      my @a = split /\s+/, $line;
      $regressError = $a[2];
      $regressSeen = 1;
    }
    elsif ($line =~ /^True train/)
    {
      $line =~ /^True train (\S+)/;
      $fullTrace = guessCars($1);
    }
    elsif ($line =~ /^Exception/)
    {
      $line =~ /^Exception.*function (.*), line number (\d+)/;
      my $fnc = $1;
      my $lno = $2;
      $line = <$fh>;
      $line = <$fh>;
      chomp $line;
      $line =~ s///g;

      my $p = $fnc . " line " . $lno . ": " . $line;
      print $fo summaryEmpty($format), "  ", $p, "\n";

      $errorsTrace[2]++;
      $errorsTrace[3] += $numIssues;
      $fracHist[$peakFrac][2]++;

      print $fo dividerLine($format);

      print $fo summaryLine($sensor, 
        $time,
        $fileno,
        \@errorsTrace, 
        \@carsTrace, 
        $fullTrace, 
        \@imperfTrace, 
        $format), "\n";

      print $fo forceNewLine($format);

      transferStats(\@errorsTrace, \@carsTrace, 
        $fullTrace, \@imperfTrace);

      @errorsTrace = (0, 0, 0, 0);
      @carsTrace = ();
      $fullTrace = 0;
      @imperfTrace = (0, 0, 0, 0);

      $hasIssues = 0;
      $numIssues = 0;
      $regressSeen = 0;
      $regressError = 0.;
      $mismatchSeen = 0;
      $exceptSeen = 1;
    }
  }
  close $fh;

  # Stragglers.
  #if ($hasIssues)
  {
    if ($regressError > $REGRESS_THRESHOLD)
    {
      $errorsTrace[1]++;
      $fracHist[$peakFrac][1]++;
    }
    # elsif (! $regressSeen && $hasIssues)
    elsif (! $regressSeen && ! $mismatchSeen && ! $exceptSeen)
    {
      # Can fail silently.
      $errorsTrace[0]++;
      $fracHist[$peakFrac][0]++;
    }
    $errorsTrace[3] += $numIssues;

    print $fo dividerLine($format);

    print $fo summaryLine($sensor, 
      $time,
      $fileno,
      \@errorsTrace, 
      \@carsTrace, 
      $fullTrace, 
      \@imperfTrace, 
      $format), "\n";

    print $fo forceNewLine($format);

    transferStats(\@errorsTrace, \@carsTrace, 
      $fullTrace, \@imperfTrace);
  }

  close $fo;
}

# Print global results.

print summaryHeader($format);
print "\n";

for my $sensor (sort keys %carsSummary)
{
  print summaryLine(
    $sensor, 
    "",
    "",
    \@{$errorsSummary{$sensor}},
    \@{$carsSummary{$sensor}},
    $fullSummary{$sensor},
    \@{$imperfSummary{$sensor}},
    $format);
  print forceNewLine($format);
}

print dividerLine($format);

print summaryLine(
  "SUM",
  "",
  "",
  \@errorsTotal, 
  \@carsTotal, 
  $fullTotal, 
  \@imperfTotal, 
  $format);
print forceNewLine($format);
print forceNewLine($format);

print summaryDetails(\%detailsSummaryFirst, $format);
print forceNewLine($format);
print summaryDetails(\%detailsSummaryLater, $format);
print forceNewLine($format);

print summaryFrac($format);

exit;


sub setCarTables
{
  $carTable{"MERIDIAN_DEU_08_N"} = 3;
  $carTable{"MERIDIAN_DEU_14_N"} = 6;
  $carTable{"MERIDIAN_DEU_22_N"} = 9;
  $carTable{"MERIDIAN_DEU_22_R"} = 9;
  $carTable{"MERIDIAN_DEU_28_N"} = 12;
  $carTable{"MERIDIAN_DEU_42_N"} = 18;
  $carTable{"SBAHN423_DEU_10_N"} = 4;
  $carTable{"SBAHN423_DEU_20_N"} = 8;
  $carTable{"SBAHN423_DEU_30_N"} = 12;
  $carTable{"X61_SWE_10_N"} = 4;
  $carTable{"X61_SWE_20_N"} = 8;
  $carTable{"X74_SWE_14_N"} = 5;
  $carTable{"X74_SWE_14_R"} = 5;
  $carTable{"X74_SWE_28_N"} = 10;
  $carTable{"X74_SWE_28_R"} = 10;
}


sub setDetails
{
  @detailsLower = qw(
     0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16
    17 21 25 29 33 65);
  @detailsUpper = qw(
     0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16
    20 24 28 32 64 999);
  @detailsNames = qw(
     0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16
     17-20 21-24 25-28 29-32 33-64 65+);

}


sub transferStats
{
  my ($errorsRef, $carsRef, $fullVal, $imperfRef) = @_;
  # Transfer to summary hashes.
  for my $i (0 .. $#$errorsRef)
  {
    $errorsSummary{$sensor}[$i] += $errorsRef->[$i];
    $errorsTotal[$i] += $errorsRef->[$i];
  }

  for my $i (0 .. $#$carsRef)
  {
    $carsSummary{$sensor}[$i] += $carsRef->[$i];
    $carsTotal[$i] += $carsRef->[$i];
  }

  $fullSummary{$sensor} += $fullVal;
  $fullTotal += $fullVal;

  for my $i (0 .. $#$imperfRef)
  {
    $imperfSummary{$sensor}[$i] += $imperfRef->[$i];
    $imperfTotal[$i] += $imperfRef->[$i];
  }
}


sub summaryHeaderTXT
{
  my $str = sprintf "%-10s%8s%6s%6s%6s%6s%6s  %6s%6s%6s%6s  ",
    "Sensor", "Time", "Trace",
    "Error", "Dev", "Exc", "Warn",
    "Fskip", "Fspur", "Skip", "Spur";

  for my $i (0 .. $NUM_LAST_FNC)
  {
    $str .= sprintf "%6d", $i;
  }
  $str .= sprintf "%6s%6s",
    "Sum", "Miss";
  return $str;
}


sub summaryHeaderCSV
{
  my $str = 
    "Sensor" . $SEPARATOR .
    "Time" . $SEPARATOR .
    "Trace" . $SEPARATOR . $SEPARATOR .
    "Error" . $SEPARATOR . 
    "Dev" . $SEPARATOR .
    "Exc" . $SEPARATOR .
    "Warn" . $SEPARATOR . $SEPARATOR .
    "Fskip" . $SEPARATOR .
    "Fspur" . $SEPARATOR .
    "Skip" . $SEPARATOR .
    "Spur" . $SEPARATOR . $SEPARATOR;

  for my $i (0 .. $NUM_LAST_FNC)
  {
    $str .= $i . $SEPARATOR;
  }
  $str .= "Sum" . $SEPARATOR . "Miss";
  return $str;
}


sub summaryHeader
{
  my $format = pop;
  if ($format == $FORMAT_TXT)
  {
    return summaryHeaderTXT();
  }
  else
  {
    return summaryHeaderCSV();
  }
}

sub detailHeaderTXT
{
  my $str = sprintf "  %-6s%-12s%4s%4s  %s",
    "first",
    "range",
    "***",
    "**",
    "profile";
  return $str;
}


sub detailHeaderCSV
{
  my $str =
    $SEPARATOR .
    $SEPARATOR .
    "first" . $SEPARATOR .
    "range" . $SEPARATOR .
    "***" . $SEPARATOR .
    "**" . $SEPARATOR .
    "profile";
  return $str;
}


sub detailHeader
{
  my $format = pop;
  if ($format == $FORMAT_TXT)
  {
    return detailHeaderTXT();
  }
  else
  {
    return detailHeaderCSV();
  }
}


sub dividerLine
{
  my $format = pop;
  if ($format == $FORMAT_TXT)
  {
    return ("-" x (94 + 6 * $NUM_LAST_FNC)) . "\n";
  }
  else
  {
    return "";
  }
}


sub newLine
{
  my $format = pop;
  if ($format == $FORMAT_TXT)
  {
    return "\n";
  }
  else
  {
    return "";
  }
}


sub forceNewLine
{
  my $format = pop;
  return "\n";
}


sub summaryLineTXT
{
  my ($sensor, $time, $trace,
    $errorsRef, $carsRef, $full, $imperfRef) = @_;
  my $str = sprintf "%-10s%8s%6s%6s%6s%6s%6s  %6s%6s%6s%6s  ",
    $sensor,
    $time,
    $trace,

    $errorsRef->[0],
    $errorsRef->[1],
    $errorsRef->[2],
    $errorsRef->[3],

    $imperfRef->[0],
    $imperfRef->[1],
    $imperfRef->[3],
    $imperfRef->[2];

  my $s = 0;
  for my $i (0 .. $NUM_LAST_FNC)
  {
    if (defined $carsRef->[$i])
    {
      $str .= sprintf "%6d", $carsRef->[$i];
      $s += $carsRef->[$i];
    }
    else
    {
      $str .= sprintf "%6d", 0;
    }
  }

  my $reduced = $s;
  if (defined $carsRef->[$NUM_EMPTY_FNC])
  {
    $reduced -= $carsRef->[$NUM_EMPTY_FNC];
  }

  $str .= sprintf "%6d%6d",
    $full, $full - $reduced;
  return $str;
}


sub summaryLineCSV
{
  my ($sensor, $time, $trace,
    $errorsRef, $carsRef, $full, $imperfRef) = @_;
  my $str = 
    $sensor . $SEPARATOR .
    $time . $SEPARATOR .
    $trace . $SEPARATOR . $SEPARATOR .

    $errorsRef->[0] . $SEPARATOR .
    $errorsRef->[1] . $SEPARATOR .
    $errorsRef->[2] . $SEPARATOR .
    $errorsRef->[3] . $SEPARATOR . $SEPARATOR .

    $imperfRef->[0] . $SEPARATOR .
    $imperfRef->[1] . $SEPARATOR .
    $imperfRef->[3] . $SEPARATOR .
    $imperfRef->[2] . $SEPARATOR . $SEPARATOR;

  my $s = 0;
  for my $i (0 .. $NUM_LAST_FNC)
  {
    if (defined $carsRef->[$i])
    {
      $str .= $carsRef->[$i] . $SEPARATOR;
      $s += $carsRef->[$i];
    }
    else
    {
      $str .= $SEPARATOR;
    }
  }

  my $reduced = $s;
  if (defined $carsRef->[$NUM_EMPTY_FNC])
  {
    $reduced -= $carsRef->[$NUM_EMPTY_FNC];
  }

  $str .=
    $full . $SEPARATOR .
    ($full - $reduced);
  return $str;
}


sub summaryLine
{
  my ($sensor, $time, $trace,
    $errorsRef, $carsRef, $full, $imperfRef, $format) = @_;
  if ($format == $FORMAT_TXT)
  {
    return summaryLineTXT($sensor, $time, $trace,
      $errorsRef, $carsRef, $full, $imperfRef);
  }
  else
  {
    return summaryLineCSV($sensor, $time, $trace,
      $errorsRef, $carsRef, $full, $imperfRef);
  }
}


sub detailLineTXT
{
  my $detailsRef = pop;
  my $str = sprintf "  %-6s%-12s%4d%4d  %s",
    $detailsRef->[0],
    $detailsRef->[1],
    $detailsRef->[2],
    $detailsRef->[3],
    $detailsRef->[4];
  return $str;
}


sub detailLineCSV
{
  my $detailsRef = pop;
  my $str =
    $detailsRef->[0] . $SEPARATOR .
    $detailsRef->[1] . $SEPARATOR .
    $detailsRef->[2] . $SEPARATOR .
    $detailsRef->[3] . $SEPARATOR .
    $detailsRef->[4] . $SEPARATOR;
}


sub detailLine
{
  my ($detailsRef, $format) = @_;
  if ($format == $FORMAT_TXT)
  {
    return detailLineTXT($detailsRef);
  }
  else
  {
    return detailLineCSV($detailsRef);
  }
}


sub summaryEmpty
{
  my $format = pop;
  if ($format == $FORMAT_TXT)
  {
    return " " x 124;
  }
  else
  {
    return $SEPARATOR x 23;
  }
}


sub guessCars
{
  my $text = pop;
  my @a = split /_/, $text;
  if ($text =~ /^ICE/)
  {
    return $a[2] / 4;
  }
  elsif ($a[0] eq "X2" || $a[0] eq "X31" || $a[0] eq "X55")
  {
    return $a[2] / 4;
  }
  elsif (defined $carTable{$text})
  {
    return $carTable{$text};
  }
  else
  {
    die "Unknown MERIDIAN train: $text";
  }
}


sub parseLines
{
  my ($line1, $line2, $detailsRef) = @_;

  if ($line1 =~ /^first/)
  {
    $detailsRef->[0] = "yes";
  }
  else
  {
    $detailsRef->[0] = "";
  }

  $line1 =~ /(\d+-\d+)/;
  $detailsRef->[1] = $1;

  $line2 =~ /: (\S+) (\S+) (\S+) (\S+)/;
  $detailsRef->[2] = 0;
  $detailsRef->[3] = 0;
  $detailsRef->[2] += $1 if ($1 ne '-');
  $detailsRef->[2] += $2 if ($2 ne '-');
  $detailsRef->[2] += $3 if ($3 ne '-');

  my $end = $4;
  $end =~ s/,$//;
  $detailsRef->[2] += $end if ($end ne '-');

  $line2 =~ / wh (\S+) (\S+)/;
  $detailsRef->[2] += $1 if ($1 ne '-');

  $end = $2;
  $end =~ s/,$//;
  $detailsRef->[2] += $end if ($end ne '-');

  $line2 =~ / q (\S+) (\S+)/;
  $detailsRef->[2] += $1 if ($1 ne '-');
  $detailsRef->[3] += $2 if ($2 ne '-');
  
  $line2 =~ s/: (.*)//;
  my $rest = $1;
  $rest =~ s/ \(\d+ of \d+\)//;
  $detailsRef->[4] = $rest;
}


sub logDetails
{
  my ($sensor, $addRef) = @_;
  if ($addRef->[0] eq "yes")
  {
    $detailsSummaryFirst{$sensor}[$addRef->[2] + $addRef->[3]]++;
  }
  else
  {
    $detailsSummaryLater{$sensor}[$addRef->[2] + $addRef->[3]]++;
  }
}


sub countHeaderTXT
{
  my $s = sprintf "%-10s", "Sensor";
  for my $i (0 .. $#detailsNames)
  {
    $s .= sprintf "%6s", $detailsNames[$i];
  }
  return $s . "\n";
}


sub countHeaderCSV
{
  my $s = "Sensor";
  for my $i (0 .. $#detailsNames)
  {
    $s .= $SEPARATOR . $detailsNames[$i];
  }
  return $s . "\n";
}


sub countHist
{
  my ($histRef, $index) = @_;
  my $sum = 0;
  for my $i ($detailsLower[$index] .. $detailsUpper[$index])
  {
    if (defined $histRef->[$i])
    {
      $sum += $histRef->[$i];
    }
  }
  return $sum;
}


sub countDetailsTXT
{
  my $detailsRef = pop;
  my $s = "";
  for my $ss (sort keys %carsSummary)
  {
    $s .= sprintf "%-10s", $ss;
    for my $i (0 .. $#detailsNames)
    {
      $s .= sprintf "%6s", countHist(\@{$detailsRef->{$ss}}, $i);
    }
    $s .= "\n";
  }
  $s .= "\n";
}


sub countDetailsCSV
{
  my $detailsRef = pop;
  my $s = "";
  for my $ss (sort keys %carsSummary)
  {
    $s .= $ss;
    for my $i (0 .. $#detailsNames)
    {
      $s .= $SEPARATOR . countHist(\@{$detailsRef->{$ss}}, $i);
    }
    $s .= "\n";
  }
  $s .= "\n";
}


sub summaryDetails
{
  my ($detailsRef, $format) = @_;

  if ($format == $FORMAT_TXT)
  {
    return countHeaderTXT() .  countDetailsTXT($detailsRef);
  }
  else
  {
    return countHeaderCSV() .  countDetailsCSV($detailsRef);
  }
}


sub fracHeaderTXT
{
  my $s = sprintf "%-8s%6s%6s%6s%6s\n",
    "Percent", "All", "Err", "Dev", "Exc";
  return $s;
}


sub fracHeaderCSV
{
  my $s = 
    "Percent" . $SEPARATOR .
    "All" . $SEPARATOR .
    "Err" . $SEPARATOR .
    "Dev" . $SEPARATOR .
    "Exc" . "\n";
  return $s;
}


sub fracDetailsTXT
{
  my $s = "";
  for my $i (0 .. $#fracHist)
  {
    $s .= sprintf "%-8s%6d%6d%6d%6d\n",
      $i, 
      $fracHist[$i][3],
      $fracHist[$i][0],
      $fracHist[$i][1],
      $fracHist[$i][2];
  }
  return $s;
}


sub fracDetailsCSV
{
  my $s = "";
  for my $i (0 .. $#fracHist)
  {
    $s .= 
      $i . $SEPARATOR .
      $fracHist[$i][3] . $SEPARATOR .
      $fracHist[$i][0] . $SEPARATOR .
      $fracHist[$i][1] . $SEPARATOR .
      $fracHist[$i][2] . "\n";
  }
  return $s;
}


sub summaryFrac
{
  my $format = pop;
  if ($format == $FORMAT_TXT)
  {
    return fracHeaderTXT() .  fracDetailsTXT();
  }
  else
  {
    return fracHeaderCSV() .  fracDetailsCSV();
  }
}
