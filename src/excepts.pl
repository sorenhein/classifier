#!perl

use strict;
use warnings;

# Tabulates exceptions.

if ($#ARGV < 0)
{
  print "Usage: minlist.pl sensor??.txt > file\n";
  exit;
}

my (%excepts, %sums);

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
    elsif ($line =~ /^Exception thrown/)
    {
      $line = <$fh>;
      $line = <$fh>;
      chomp $line;
      $line =~ s///g;

      $excepts{$sensor}{$line}++;
      $sums{$line}++;
    }
  }
  close $fh;
}

my $header = sprintf "%10s", "Sensor";
my $footer = "-" x 10;
my $i = 0;
for my $e (sort keys %sums)
{
  $header .= sprintf " %4s", "(" . $i . ")";
  $footer .= "-" x 5;
  $i++;
}
print "$header\n";

for my $s (sort keys %excepts)
{
  my $line = sprintf "%10s", $s;
  for my $e (sort keys %sums)
  {
    if (defined $excepts{$s}{$e})
    {
      $line .= sprintf " %4d", $excepts{$s}{$e};
    }
    else
    {
      $line .= sprintf " %4s", "-";
    }
  }
  print "$line\n";
}

print "$footer\n";

my $sum = sprintf "%10s", "";

for my $e (sort keys %sums)
{
  $sum .= sprintf " %4s", $sums{$e};
}

print "$sum\n\n";

$i = 0;
for my $e (sort keys %sums)
{
  printf "(%d)  %s\n", $i, $e;
  $i++;
}


exit;

