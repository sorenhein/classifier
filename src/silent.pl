#!perl

use strict;
use warnings;

# Extract the silent fails.

my $SEPARATOR = ";";
my $EMPTY = ";;;;;;";

my $REGRESS_THRESHOLD = 10.;
my $COMPS = 3;

if ($#ARGV < 0)
{
  print "Usage: silent.pl sensor??.txt > file\n";
  exit;
}

print header(), "\n";

for my $file (@ARGV)
{
  my ($sname, $date, $time);
  my $fileno = -1;
  my $state = -1;
  my $true = "";
  my $hasWarn = 0;
  
  my $sensor = $file;
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
      $sname = $a[2];
      $state = 0;
      $hasWarn = 0;
    }
    elsif ($state == 0 && $line =~ /^WARNFINAL/)
    {
      $hasWarn = 1;
    }
    elsif ($state == 0 && $line =~ /^True train/)
    {
      $line =~ /train (.+) at/;
      $true = $1;
      $state = 1;
    }
    elsif ($state == 1 && $line =~ /^Matching/)
    {
      # The first block is less precise.
      $state = 2;
    }
    elsif ($state == 2 && $line =~ /^Matching/)
    {
      next if $hasWarn;

      my @segs;
      $segs[$_] = $EMPTY for 0 .. $COMPS-1;

      for my $i (0 .. $COMPS-1)
      {
        $line = <$fh>;
        chomp $line;
        $line =~ s///g;

        last if ($line eq "");
        my @a = split /\s+/, $line;

        # Select mismatches.
        # last if ($i == 0 && $a[0] eq $true);

        # Select fits.
        last if ($i == 0 && $a[0] ne $true);

        # Select large distances.
        last if ($i == 0 && $a[2] < $REGRESS_THRESHOLD);

        setSegment($true, \@a, \$segs[$i]);
      }

      if ($fileno >= 0 && $segs[0] ne $EMPTY)
      {
        print entry($sensor, $time, $fileno, \@segs), "\n";
      }

      $state = 3;
    }
  }
  close $fh;
}

exit;


sub header
{
  my $st = 
    "Sensor" . $SEPARATOR .
    "Time" . $SEPARATOR .
    "Fno" . $SEPARATOR;

  for my $i (0 .. $COMPS-1)
  {
    $st .=
      "Match" . $SEPARATOR .
      "Dfull" . $SEPARATOR .
      "Dpart" . $SEPARATOR .
      "Add" . $SEPARATOR .
      "Del" . $SEPARATOR .
      "" . $SEPARATOR;
  }
  return $st;
}


sub setSegment
{
  my ($true, $listRef, $segRef) = @_;

  $listRef->[1] =~ s/\./,/;
  $listRef->[2] =~ s/\./,/;

  $$segRef = 
    $listRef->[0] . $SEPARATOR .
    $listRef->[1] . $SEPARATOR .
    $listRef->[2] . $SEPARATOR .
    $listRef->[3] . $SEPARATOR .
    $listRef->[4] . $SEPARATOR;

  if ($listRef->[0] eq $true)
  {
    $$segRef .= "*";
  }

  $$segRef .= $SEPARATOR;
}


sub entry
{
  my ($sensor, $time, $fileno, $segRef) = @_;
  my $st =
    $sensor . $SEPARATOR .
    $time . $SEPARATOR .
    $fileno . $SEPARATOR .
    $segRef->[0] .
    $segRef->[1] .
    $segRef->[2];
  return $st;
}

