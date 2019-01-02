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

if ($#ARGV < 0)
{
  print "Usage: summarize.pl [CSV] sensor??.txt > file";
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
setCarTables();


# Summary hashes.
my (%errorsSummary, %carsSummary, %fullSummary, %imperfSummary);

# Totals.
my (@errorsTotal, @carsTotal, $fullTotal, @imperfTotal);
my $sensor;

for my $file (@ARGV)
{
  # Per-sensor local house-keeping.
  my @details;
  my ($sname, $date, $time);
  my $fileno = -1;
  
  $sensor = $file;
  $sensor =~ s/.txt//;
  print "$sensor\n";

  open my $fh, "<", $file or die "Cannot open $file: $!";

  open my $fo, ">", "summary/$file" or die "Cannot open $file: $!";
  print $fo summaryHeader($format), detailHeader($format), "\n";

  # Per-trace arrays.
  my (@errorsTrace, @carsTrace, $fullTrace, @imperfTrace);
  @errorsTrace = (0, 0, 0, 0);
  @imperfTrace = (0, 0, 0, 0);
  $fullTrace = 0;
  my $hasIssues = 0;
  my $numIssues = 0;
  my $regressError = 0.;
  while (my $line = <$fh>)
  {
    chomp $line;
    $line =~ s///g;

    if ($line =~ /^File/)
    {
      $line =~ /^File.*\/([0-9_]*)_001_channel1.dat:/;
      my @a = split /_/, $1;
      $date = $a[0];
      $time = $a[1];
      $sname = $a[2];

      if ($hasIssues)
      {
        if ($regressError > $REGRESS_THRESHOLD)
        {
          $errorsTrace[1]++;
        }
        $errorsTrace[3] += $numIssues;

        print $fo summaryLine($sensor, 
          $time,
          $fileno,
          \@errorsTrace, 
          \@carsTrace, 
          $fullTrace, 
          \@imperfTrace, 
          $format), "\n";

        transferStats(\@errorsTrace, \@carsTrace, 
          $fullTrace, \@imperfTrace);

        @errorsTrace = (0, 0, 0, 0);
        @carsTrace = ();
        $fullTrace = 0;
        @imperfTrace = (0, 0, 0, 0);

        $hasIssues = 0;
        $numIssues = 0;
      }

      $fileno++;
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
        print $fo summaryEmpty($format), detailLine(\@details, $format);
        $numIssues++;
      }
    }
    elsif ($line =~ /MISMATCH/)
    {
      $errorsTrace[0]++;
      $errorsTrace[3] += $numIssues;

      print $fo summaryLine($sensor, 
        $time,
        $fileno,
        \@errorsTrace, 
        \@carsTrace, 
        $fullTrace, 
        \@imperfTrace, 
        $format), "\n";

      transferStats(\@errorsTrace, \@carsTrace, 
        $fullTrace, \@imperfTrace);

      @errorsTrace = (0, 0, 0, 0);
      @carsTrace = ();
      $fullTrace = 0;
      @imperfTrace = (0, 0, 0, 0);

      $hasIssues = 0;
      $numIssues = 0;
    }
    elsif ($line =~ /^Regression alignment/)
    {
      $line = <$fh>;
      chomp $line;
      $line =~ s///g;
      my @a = split /\s+/, $line;
      $regressError = $a[2];
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
      print $fo summaryEmpty($format), $p;

      $errorsTrace[2]++;
      $errorsTrace[3] += $numIssues;

      print $fo summaryLine($sensor, 
        $time,
        $fileno,
        \@errorsTrace, 
        \@carsTrace, 
        $fullTrace, 
        \@imperfTrace, 
        $format), "\n";

      transferStats(\@errorsTrace, \@carsTrace, 
        $fullTrace, \@imperfTrace);

      @errorsTrace = (0, 0, 0, 0);
      @carsTrace = ();
      $fullTrace = 0;
      @imperfTrace = (0, 0, 0, 0);

      $hasIssues = 0;
      $numIssues = 0;
    }
  }
  close $fh;

  # Stragglers.
  if ($hasIssues)
  {
    if ($regressError > $REGRESS_THRESHOLD)
    {
      $errorsTrace[1]++;
    }
    $errorsTrace[3] += $numIssues;

    print $fo summaryLine($sensor, 
      $time,
      $fileno,
      \@errorsTrace, 
      \@carsTrace, 
      $fullTrace, 
      \@imperfTrace, 
      $format), "\n";

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
  print summaryLine($sensor, 
    "",
    "",
    \@{$errorsSummary{$sensor}},
    \@{$carsSummary{$sensor}},
    $fullSummary{$sensor},
    \@{$imperfSummary{$sensor}},
    $format);
  print "\n";
}

print dividerLine($format);
print "\n";

print summaryLine($sensor, 
  "",
  "",
  \@errorsTotal, 
  \@carsTotal, 
  $fullTotal, 
  \@imperfTotal, 
  $format);
print "\n";

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

  for my $i (0 .. $#carsTotal)
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
    "Trace" . $SEPARATOR .
    "Error" . $SEPARATOR . 
    "Dev" . $SEPARATOR .
    "Exc" . $SEPARATOR .
    "Warn" . $SEPARATOR .
    "Fskip" . $SEPARATOR .
    "Fspur" . $SEPARATOR .
    "Skip" . $SEPARATOR .
    "Spur" . $SEPARATOR;

  for my $i (0 .. $#carsTotal)
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
  my $str = sprintf "%4s%6s%4s%4s%s",
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
    return "-" x (94 + 6 * ($#carsTotal));
  }
  else
  {
    return "";
  }
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
  for my $i (0 .. $#carsTotal)
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
    $trace . $SEPARATOR .

    $errorsRef->[0] . $SEPARATOR .
    $errorsRef->[1] . $SEPARATOR .
    $errorsRef->[2] . $SEPARATOR .
    $errorsRef->[3] . $SEPARATOR .

    $imperfRef->[0] . $SEPARATOR .
    $imperfRef->[1] . $SEPARATOR .
    $imperfRef->[3] . $SEPARATOR .
    $imperfRef->[2] . $SEPARATOR;

  my $s = 0;
  for my $i (0 .. $#carsTotal)
  {
    $str .= $carsRef->[$i] . $SEPARATOR;
    $s += $carsRef->[$i];
  }

  my $reduced = $s;
  if (defined $carsRef->[$NUM_EMPTY_FNC])
  {
    $reduced -= $carsRef->[$NUM_EMPTY_FNC];
  }

  $str .=
    $full . $SEPARATOR .
    $full - $reduced . $SEPARATOR;
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
  my $str = sprintf "%4s%6s%4s%4s%s",
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
    return " " x 80;
  }
  else
  {
    return $SEPARATOR x 12;
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
  
  $line1 =~ /:(.*)/;
  $detailsRef->[4] = $1;
}

