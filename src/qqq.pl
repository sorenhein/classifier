#!perl

use strict;
use warnings;

# Count PeakMinima qualities.

if ($#ARGV < 0)
{
  print "Usage: minlist.pl sensor??.txt > file\n";
  exit;
}

my (%rangeAgree, %rangeOffGood, %rangeOffBad);
my %qkeys;

for my $file (@ARGV)
{
  my ($sname, $date);
  my $fileno = -1;
  
  my $sensor = $file;
  my $time;
  $sensor =~ s/.txt//;

  open my $fh, "<", $file or die "Cannot open $file: $!";

  my $qqq;
  my $qindex;
  my ($rangeLo, $rangeHi);
  my ($trainTrue, $trainRegress);
  my $dist;

  while (my $line = <$fh>)
  {
    chomp $line;
    $line =~ s///g;

    if ($line =~ /^File/)
    {
      $line =~ /^File.*\/([0-9_]*)_001_channel1.dat:/;
      my @a = split /_/, $1;

      $fileno++;
      $date = $a[0];
      $time = $a[1];
    }
    elsif ($line =~ /^QQQ (\d+)/)
    {
      $qqq = $1;
    }
    elsif ($line =~ /^QM /)
    {
      $line = <$fh>;
      $line = <$fh>;
      $line = <$fh>;
      $line =~ /^index (\d+)/;
      $qindex = $1;
    }
    elsif ($line =~ /^Guessing wheel distance/)
    {
      $line =~ / (\d+)-(\d+)/;
      $rangeLo = $1;
      $rangeHi = $2;
    }
    elsif ($line =~ /^True train/)
    {
      $line =~ /^True train (\w+)/;
      $trainTrue = $1;
    }
    elsif ($line =~ /^Regression alignment/)
    {
      $line = <$fh>;
      $line =~ /^(\w+)\s+(\d+\.\d+)/;
      $trainRegress = $1;
      $dist = $2;

      $qkeys{$qqq} = 1;
      if ($qindex >= $rangeLo && $qindex <= $rangeHi)
      {
        $rangeAgree{$qqq}{count}++;
        $rangeAgree{$qqq}{dist} += $dist;
        $rangeAgree{$qqq}{wrong} += ($trainTrue ne $trainRegress);
      }
      elsif ($dist <= 5)
      {
        $rangeOffGood{$qqq}{count}++;
        $rangeOffGood{$qqq}{dist} += $dist;
        $rangeOffGood{$qqq}{wrong} += ($trainTrue ne $trainRegress);
      }
      else
      {
        $rangeOffBad{$qqq}{count}++;
        $rangeOffBad{$qqq}{dist} += $dist;
        $rangeOffBad{$qqq}{wrong} += ($trainTrue ne $trainRegress);
      }
    }
  }
  close $fh;
}

printf "%10s | %6s%8s%6s | %6s%8s%6s | %6s%8s%6s\n",
  "QQQ", 
  "num", "avg", "wrong",
  "num", "avg", "wrong",
  "num", "avg", "wrong";

for my $q (sort keys %qkeys)
{
  my $line = sprintf "%10s | ", $q;
  if (defined $rangeAgree{$q})
  {
    $line .= entry(\%{$rangeAgree{$q}});
  }
  else
  {
    $line .= sprintf "%6s%8s%6s", "-", "-", "-";
  }
  $line .= " | ";

  if (defined $rangeOffGood{$q})
  {
    $line .= entry(\%{$rangeOffGood{$q}});
  }
  else
  {
    $line .= sprintf "%6s%8s%6s", "-", "-", "-";
  }
  $line .= " | ";

  if (defined $rangeOffBad{$q})
  {
    $line .= entry(\%{$rangeOffBad{$q}});
  }
  else
  {
    $line .= sprintf "%6s%8s%6s", "-", "-", "-";
  }
  print "$line\n";
}


sub entry
{
  my $ref = pop;
  return sprintf "%6d%8.2f%6d",
      $ref->{count}, 
      $ref->{dist} / $ref->{count},
      $ref->{wrong};
}

exit;

