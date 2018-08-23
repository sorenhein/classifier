#!perl

use strict;
use warnings;

if ($#ARGV != 0)
{
  print "Usage: addup.pl file.txt";
  exit;
}

open my $fh, "<", $ARGV[0] or die "Cannot open $ARGV[0]: $!";
my $sum = 0;
while (my $line = <$fh>)
{
  chomp $line;
  $line =~ s///g;

  $line =~ /Active intervals (.*)$/;
  my $count = $1;
  $sum += $count;
}

print "Sum $sum\n";

close $fh;
