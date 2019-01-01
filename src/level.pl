#!perl

use strict;
use warnings;

# Summarize the detector hits in a sensor??.txt file

my ($file, $filebase);
if ($#ARGV == 0)
{
  $file = $ARGV[0];
  $filebase = $file;
  $filebase =~ s/.txt//;
}
else
{
  print "Usage: summ.pl sensor00.txt";
  exit;
}

my @hits;

open my $fh, "<", $file or die "Cannot open $: $!";
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
    }
  }
}

close $fh;

print "Sensor $filebase\n";
for (my $i = 0; $i <= $#hits; $i++)
{
  printf "%2d  %6d\n", $i, $hits[$i];
}
print "\n";
exit;


