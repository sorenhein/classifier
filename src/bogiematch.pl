#!perl

use strict;
use warnings;

# Checks matches with simple bogie positions in sensor??.txt files.

if ($#ARGV < 0)
{
  print "Usage: bogiematch.pl sensor??.txt\n";
  exit;
}

my %hashMap;

for my $file (@ARGV)
{
  # Per-sensor local house-keeping.
  my @details;
  my ($sname, $date, $time);
  my $fileno = -1;
  
  my $sensor = $file;
  $sensor =~ s/.txt//;

  open my $fh, "<", $file or die "Cannot open $file: $!";

  while (my $line = <$fh>)
  {
    chomp $line;
    $line =~ s///g;

    $hashMap{$sensor}{nobogie}++ if 
      (length($line) >= 18 && $line eq 'Have 0 bogie times');
    $hashMap{$sensor}{noalign}++ if 
      (length($line) >= 14 && $line eq 'Have 0 matches');

    next unless length($line) >= 18;
    next unless substr($line, 0, 18) eq "True train (bogie)";

    $line =~ /^True train .bogie. (\w+)/;
    my $true = $1;
    $hashMap{$sensor}{count}++;

    $line = <$fh>; # Empty
    $line = <$fh>; # "Matching alignment"
    next unless $line =~ /^Matching/;
    $line = <$fh>; # Winner
    $line =~ /^(\w+)/;
    my $detected = $1;

    $hashMap{$sensor}{good}++ if $true eq $detected;
  }
  close $fh;
}

printf "%-10s%6s%6s%6s%6s%6s\n", 
  "Sensor", "count", "good", "nobog", "noal", "other";
my $sumcount = 0;
my $sumgood = 0;
my $sumbogie = 0;
my $sumalign = 0;
for my $s (sort keys %hashMap)
{
  my $g = ($hashMap{$s}{good} || 0);
  my $b = ($hashMap{$s}{nobogie} || 0);
  my $m = ($hashMap{$s}{noalign} || 0);
  my $other = $hashMap{$s}{count} - $g - $b - $m;
  printf "%-10s%6d%6d%6d%6d%6d\n", 
    $s, $hashMap{$s}{count}, $g, $b, $m, $other;
  $sumcount += $hashMap{$s}{count};
  $sumgood += $g;
  $sumbogie += $b;
  $sumalign += $m;
}

print "-" x 40, "\n";
printf "%-10s%6d%6d%6d%6d%6d\n", 
  "Total", $sumcount, $sumgood, $sumbogie, $sumalign,
    $sumcount - $sumgood - $sumbogie - $sumalign;

