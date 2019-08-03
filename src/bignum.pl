#!perl

use strict;
use warnings;

# Extract large numbers (initialization issues).

if ($#ARGV < 0)
{
  print "Usage: bignum.pl sensor??.txt > file\n";
  exit;
}

for my $file (@ARGV)
{
  my ($sname, $date);
  my $fileno = -1;
  
  my $sensor = $file;
  my $time;
  $sensor =~ s/.txt//;

  open my $fh, "<", $file or die "Cannot open $file: $!";
  my $lno = 0;

  while (my $line = <$fh>)
  {
    chomp $line;
    $line =~ s///g;
    $lno++;

    if ($line =~ /^File/)
    {
      $line =~ /^File ([0-9_]*)_001_channel1.dat:/;
      my @a = split /_/, $1;

      $fileno++;
      $date = $a[0];
      $time = $a[1];
    }
    elsif ($line =~ /\d\d\d\d\d\d/)
    {
      print "Sensor $sensor date $date time $time: line number $lno\n";
      print "$line\n\n";
    }
  }
  close $fh;
}

exit;

