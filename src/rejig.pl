#!perl

use strict;
use warnings;

if ($#ARGV != 0)
{
  print "Usage: rejig.pl file.txt > out.txt";
  exit;
}

open my $fh, "<", $ARGV[0] or die "Cannot open $ARGV[0]: $!";
while (my $line = <$fh>)
{
  chomp $line;
  $line =~ s///g;

  $line =~ /sensors.(......)/;
  my $sensor = $1;

  $line =~ /raw.(.*):/;
  my $base = $1;

  my $msum = 0;
  my $mmin = 9999.;
  my $mmax = 0.;

  my $ssum = 0;
  my $smin = 9999.;
  my $smax = 0.;

  my $csum = 0;

  for my $i (0 .. 9)
  {
    $line = <$fh>;
    chomp $line;
    $line =~ s///g;

    my @a = split ';', $line;
    $msum += $a[1];
    $mmax = abs($a[1]) if abs($a[1]) > $mmax;
    $mmin = abs($a[1]) if abs($a[1]) < $mmin;

    $ssum += $a[2];
    $smax = abs($a[2]) if abs($a[2]) > $smax;
    $smin = abs($a[2]) if abs($a[2]) < $smin;

    $csum += $a[3];
  }

  my $mavg = $msum / 10.;
  my $savg = $ssum / 10.;

  print "$sensor;$base;$mavg;$mmax;$mmin;$savg;$smax;$smin;$csum\n";
}

close $fh;
