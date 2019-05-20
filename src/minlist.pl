#!perl

use strict;
use warnings;

# Extract list of certain piece patterns from PeakMinima.

if ($#ARGV < 0)
{
  print "Usage: minlist.pl sensor??.txt > file\n";
  exit;
}

# Line to match
# my $match = 'QM good -> good \(3\)';
my $match = 'QM good -> good \(\)';

for my $file (@ARGV)
{
  my ($sname, $date);
  my $fileno = -1;
  
  my $sensor = $file;
  my $time;
  $sensor =~ s/.txt//;

  open my $fh, "<", $file or die "Cannot open $file: $!";

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
    elsif ($line =~ /^$match/)
    {
      $line = <$fh>; # Empty
      my (@indices, @counts);
      while (1)
      {
        $line = <$fh>;
        last if ($line !~ /^Modality/);
        $line = <$fh>;
        $line =~ /index ([0-9-]+), MAX, (\d+)/;
        push @indices, $1;
        push @counts, $2;
      }

      printf "%8s #%2d %6s: %-20s   %-10s\n",
        $sensor, $fileno, $time, join(', ', @indices),
        join('-', @counts);
    }
  }
  close $fh;
}

exit;

