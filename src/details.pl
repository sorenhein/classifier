#!perl

use strict;
use warnings;

# Provide details of sensor??.txt traces.

my $SEPARATOR = ";";

my $FORMAT_TXT = 0;
my $FORMAT_CSV = 1;

if ($#ARGV < 0)
{
  print "Usage: details.pl [CSV] sensor??.txt > file\n";
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

my (@details, @warnings, @deviations, @errors, @excepts);

for my $file (@ARGV)
{
  # Per-sensor local house-keeping.
  my ($sname, $date, $time, $tno);
  
  my $sensor = $file;
  $sensor =~ s/.txt//;
  my $trueTrain;
  my $matchCount = 0;
  my (@matchNames, @matchValues);

  open my $fh, "<", $file or die "Cannot open $file: $!";

  while (my $line = <$fh>)
  {
    chomp $line;
    $line =~ s///g;
    my $len = length($line);

    if ($len >= 5 && substr($line, 0, 5) eq "File ")
    {
      $line =~ /^File ([0-9_]*)_001_channel1.dat: number (\d+)/;
      $tno = $2;
      my @a = split /_/, $1;
      $date = $a[0];
      $time = $a[1];
      $sname = $a[2];
      $matchCount = 0;
      @matchNames = ();
      @matchValues = ();
    }
    elsif ($len > 9 && substr($line, 0, 5) eq "WARNF")
    {
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
        push @warnings, 
          detailLine($sensor, $time, $tno, \@details, $format);
      }
    }
    elsif ($len > 11 && substr($line, 0, 11) eq "True train ")
    {
      $line =~ /^True train (\S+) at/;
      $trueTrain = $1;
    }
    elsif ($len >= 18 && substr($line, 0, 18) eq "Matching alignment")
    {
      if ($matchCount == 0)
      {
        $matchCount = 1; # Skip the first one
      }
      else
      {
        my $i = 0;
        my $f;
        while ($line = <$fh>)
        {
          last if ($i == 3);
          $line =~ /^(\S+)\s+(\d+\.\d+)\s+(\d+\.\d+)/;
          $f = $1;
          last if ($3 > 10.);
          push @matchNames, $1;
          push @matchValues, $3;
          $i++;
        }

        # $line = <$fh>;
        # $line =~ /^(\S+)\s+(\d+\.\d+)\s+(\d+\.\d+)/;
        # my $bestTrain = $1;
        # my $devMatch = $3;
        if ($f eq $trueTrain && 
          (! @matchValues || $matchValues[0] > 10.))
        {
          push @deviations,
              detailLineCommon($sensor, $time, $tno, $format);
        }
      }
    }
    elsif ($len > 14 && substr($line, 0, 7) eq "DRIVER ")
    {
      push @errors,
          detailLineError($sensor, $time, $tno, 
            $trueTrain, \@matchNames, \@matchValues, $format);
    }
    elsif ($len > 9 && substr($line, 0, 9) eq "Exception")
    {
      $line = <$fh>; # Skip code
      $line = <$fh>; # Get text
      chomp $line;
      $line =~ s///g;
      push @excepts,
          detailLineCommon($sensor, $time, $tno, $format) . 
          $SEPARATOR . $line;
    }
  }
  close $fh;
}

if ($#warnings >= 0)
{
  print "Warnings\n";
  print $_ . "\n" for (@warnings);
  print "\n";
}

if ($#deviations >= 0)
{
  print "Large deviations\n";
  print $_ . "\n" for (@deviations);
  print "\n";
}

if ($#errors >= 0)
{
  print "Errors\n";
  print $_ . "\n" for (@errors);
  print "\n";
}

if ($#excepts >= 0)
{
  print "Exceptions\n";
  print $_ . "\n" for (@excepts);
  print "\n";
}

exit;


sub parseLines
{
  my ($line1, $line2, $detailsRef) = @_;

  if ($line1 =~ /^first/)
  {
    $detailsRef->[0] = "first";
  }
  elsif ($line1 =~ /^intra/)
  {
    $detailsRef->[0] = "mid";
  }
  else
  {
    $detailsRef->[0] = "last";
  }

  $line1 =~ /(\d+-\d+)/;
  $detailsRef->[1] = $1;

  $line2 =~ s/: (.*)//;
  $detailsRef->[2] = $1;
}


sub detailLineTXTCommon
{
  my ($sensor, $time, $tno, $names_ref, $values_ref) = @_;
  my $str = sprintf "%-12s%8s%6s  ",
    $sensor,
    $time,
    $tno;
  return $str;
}


sub detailLineCSVCommon
{
  my ($sensor, $time, $tno) = @_;
  my $str =
    $sensor . $SEPARATOR .
    $time . $SEPARATOR .
    $tno;

  return $str;
}


sub detailLineCSVError
{
  my ($sensor, $time, $tno, $trueTrain, $names_ref, $values_ref) = @_;
  my $str =
    $sensor . $SEPARATOR .
    $time . $SEPARATOR .
    $tno . $SEPARATOR .
    $SEPARATOR . 
    $trueTrain;
  for my $i (0 .. $#$names_ref)
  {
    $str .= $SEPARATOR . $names_ref->[$i] .
      $SEPARATOR . $values_ref->[$i];
  }

  return $str;
}


sub detailLineCommon
{
  my ($sensor, $time, $tno, $format) = @_;
  if ($format == $FORMAT_TXT)
  {
    return detailLineTXTCommon($sensor, $time, $tno);
  }
  else
  {
    return detailLineCSVCommon($sensor, $time, $tno);
  }
}


sub detailLineError
{
  my ($sensor, $time, $tno, $trueTrain, 
    $matchNames_ref, $matchValues_ref, $format) = @_;
  if ($format == $FORMAT_TXT)
  {
    return detailLineTXTCommon($sensor, $time, $tno);
  }
  else
  {
    return detailLineCSVError($sensor, $time, $tno, 
      $trueTrain, $matchNames_ref, $matchValues_ref);
  }
}


sub detailLineTXT
{
  my ($detailsRef) = @_;
  my $str = sprintf "  %-6s%-12s%s",
    $detailsRef->[0],
    $detailsRef->[1],
    $detailsRef->[2];
  return $str;
}


sub detailLineCSV
{
  my ($detailsRef) = @_;
  my $str =
    $detailsRef->[0] . $SEPARATOR .
    $detailsRef->[1] . $SEPARATOR .
    $detailsRef->[2];
  return $str;
}


sub detailLine
{
  my ($sensor, $time, $tno, $detailsRef, $format) = @_;
  my $s = detailLineCommon($sensor, $time, $tno, $format);
  if ($format == $FORMAT_TXT)
  {
    return $s . detailLineTXT($detailsRef);
  }
  else
  {
    return $s . detailLineCSV($detailsRef);
  }
}
