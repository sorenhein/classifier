#!perl

use strict;
use warnings;

# Provide distances of sensor??.txt traces by sensor and train type.
# Can then be read into Python.

my $SEPARATOR = ";";

my $equiv_file = "../../../mini_dataset_v012/equiv.csv";

if ($#ARGV < 0)
{
  print "Usage: distances.pl sensor??.txt > file\n";
  exit;
}

my %equiv;
read_equiv($equiv_file, \%equiv);

my %data;

for my $file (@ARGV)
{
  # Per-sensor local house-keeping.
  my ($sname, $date, $time, $tno);
  
  my $sensor = $file;
  $sensor =~ s/.txt//;
  my $trueTrain;
  my $matchCount = 0;
  my (@matchNames, @matchValues);

  open my $fh, "<", $file or die "Cannot open $file: $!";

  while (my $line = <$fh>)
  {
    chomp $line;
    $line =~ s///g;
    my $len = length($line);

    if ($len >= 5 && substr($line, 0, 5) eq "File ")
    {
      $line =~ /^File ([0-9_]*)_001_channel1.dat: number (\d+)/;
      $tno = $2;
      my @a = split /_/, $1;
      $date = $a[0];
      $time = $a[1];
      $sname = $a[2];
      $matchCount = 0;
    }
    elsif ($len > 11 && substr($line, 0, 11) eq "True train ")
    {
      $line =~ /^True train (\S+) at/;
      $trueTrain = $1;
    }
    elsif ($len >= 18 && substr($line, 0, 18) eq "Matching alignment")
    {
      if ($matchCount == 0)
      {
        $matchCount = 1; # Skip the first one
        next;
      }

      my $f;
      $line = <$fh>;
      $line =~ /^(\S+)\s+(\d+\.\d+)\s+(\d+\.\d+)/;
      next if ($3 > 10.);
      $f = $1;
      my $d = $3;

      my $speed;
      while ($line = <$fh>)
      {
        if ($line =~ /^Speed/)
        {
          $line =~ /(\d+\.\d+) km/;
          $speed = $1;
          last;
        }
      }

      if ($f eq $trueTrain || 
            (defined $equiv{$f} && defined $equiv{$f}{$trueTrain}))
      {
        if (! defined $data{$sname}{$f})
        {
          $data{$sname}{$f}[0][0] = $speed;
          $data{$sname}{$f}[0][1] = $d;
        }
        else
        {
          my $l = $#{$data{$sname}{$f}};
          $data{$sname}{$f}[$l+1][0] = $speed;
          $data{$sname}{$f}[$l+1][1] = $d;
        }
      }
    }
  }
  close $fh;
}

for my $s (sort keys %data)
{
  for my $t (sort keys %{$data{$s}})
  {
    print "$s\n$t\n";
    for my $i (0 .. $#{$data{$s}{$t}})
    {
      print $data{$s}{$t}[$i][0] . "," . $data{$s}{$t}[$i][1] . "\n";
    }
    print "\n";
  }
}


sub read_equiv
{
  my ($equiv_file, $equiv_ref) = @_;
  open my $fh, "<", $equiv_file or die "Cannot open $equiv_file: $!";

  while (my $line = <$fh>)
  {
    chomp $line;
    $line =~ s///g;
    my @a = split /,/, $line;
    for my $c (@a)
    {
      for my $d (@a)
      {
        next if $c eq $d;
        $equiv_ref->{$c}{$d} = 1;
        $equiv_ref->{$d}{$c} = 1;
      }
    }
  }
}


