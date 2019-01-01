#!perl

use strict;
use warnings;

# Summarize the detector hits in sensor??.txt files

my @cumhits;
my %imperf;
if ($#ARGV < 0)
{
  print "Usage: summ.pl sensor??.txt > hits.txt";
  exit;
}

for my $file (@ARGV)
{
  my $filebase = $file;
  $filebase =~ s/.txt//;
  print "$filebase\n";

  my @hits;
  open my $fh, "<", $file or die "Cannot open $file: $!";
  while (my $line = <$fh>)
  {
    chomp $line;
    $line =~ s///g;

    if ($line =~ /^HITS/)
    {
      while (1)
      {
        $line = <$fh>;
        chomp $line;
        $line =~ s///g;
        last if $line =~ /^\s*$/;
  
        $line =~ /^(\d+)\s+(\d+)/;
        $hits[$1] += $2;
        $cumhits[$1] += $2;
      }
    }
    elsif ($line =~ /^IMPERF/)
    {
      $line =~ / (\d+)-(\d+), (\d+)-(\d+)/;
      $imperf{$filebase}[0] += $1;
      $imperf{$filebase}[1] += $2;
      $imperf{$filebase}[2] += $3;
      $imperf{$filebase}[3] += $4;
    }
  }

  close $fh;

  open my $fo, ">", "summary/hit$file" or die "Cannot open $file: $!";

  print $fo "Sensor $filebase\n";
  for (my $i = 0; $i <= $#hits; $i++)
  {
    printf $fo "%2d  %6d\n", $i, $hits[$i];
  }
  print $fo "\n";

  close $fo;
}

print "SUM\n";
for (my $i = 0; $i <= $#cumhits; $i++)
{
  printf "%2d  %6d\n", $i, $cumhits[$i];
}
print "\n";

print "IMPERF\n\n";

printf "%-10s %6s%6s%6s%6s\n", "Sensor", "FSkip", "FSpur", "Skip", "Spur";
for my $base (sort keys %imperf)
{
  printf "%-10s %6d%6d%6d%6s\n", $base, 
    $imperf{$base}[0],
    $imperf{$base}[1],
    $imperf{$base}[3],
    $imperf{$base}[2];
}
print "\n";

