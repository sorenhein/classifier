#!perl

use strict;
use warnings;

# Extract data from overview.txt

if ($#ARGV != 0)
{
  print "Usage: reover.pl overview.txt > out.txt";
  exit;
}

my $sensor = "NONE";
my ($excepts, $series, $peaks);
my ($sum_excepts, $sum_series, $sum_peaks);

my $state = 0;

open my $fh, "<", $ARGV[0] or die "Cannot open $ARGV[0]: $!";
while (my $line = <$fh>)
{
  chomp $line;
  $line =~ s///g;

  if ($line =~ /^(sensor..).txt/)
  {
    print_line() unless ($sensor eq "NONE");

    $sensor = $1;
    $line = <$fh>;
    if ($line =~ /^\s*(\d+)/)
    {
      $excepts = $1;
      $state = 1;
    }
    else
    {
      die "No except number in '$line'";
    }
    next;
  }

  if ($state == 1)
  {
    if ($line =~ /^Sum\s+(\d+)/)
    {
      $series = $1;
      $state = 2;
    }
    elsif ($line =~ /^Sum\s+\-/)
    {
      $series = 0;
      $state = 2;
    }
  }
  elsif ($state == 2)
  {
    if ($line =~ /^Sum\s+(\d+)\s+(\d+)/)
    {
      $peaks = $2;
      $state = 3;
    }
  }
}

close $fh;
print_line();
print_sum();
exit;


sub print_line
{
  printf "%-12s%8d%8d%8d\n", $sensor, $excepts, $series, $peaks;
  $sum_excepts += $excepts;
  $sum_series += $series;
  $sum_peaks += $peaks;
}

sub print_sum
{
  print "-" x 36, "\n";
  printf "%-12s%8d%8d%8d\n", "", $sum_excepts, $sum_series, $sum_peaks;
}
