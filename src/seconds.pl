#!perl

use strict;
use warnings;

# Outputs scatter data of (first, second) distance by match and no match.

my $SEPARATOR = ";";

my $equiv_file = "../../../mini_dataset_v012/equiv.csv";

if ($#ARGV < 0)
{
  print "Usage: distances.pl sensor??.txt > file\n";
  exit;
}

my (@good, @bad, %equiv);
read_equiv($equiv_file, \%equiv);

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

      my ($f1, $f2);
      $line = <$fh>;
      $line =~ /^(\S+)\s+(\d+\.\d+)\s+(\d+\.\d+)/;
      $f1 = $1;
      my $d1 = $3;

      $line = <$fh>;
      next if ($line =~ /^\s*$/);
      $line =~ /^(\S+)\s+(\d+\.\d+)\s+(\d+\.\d+)/;
      $f2 = $1;
      my $d2 = $3;


      if ($f1 eq $trueTrain || 
            (defined $equiv{$f1} && defined $equiv{$f1}{$trueTrain}))
      {
        @{$good[$#good+1]} = ($d1, $d2);
      }
      else
      {
        @{$bad[$#bad+1]} = ($d1, $d2);
      }
    }
  }
  close $fh;
}

print "Good\n";
for my $i (0 .. $#good)
{
  print $good[$i][0], $SEPARATOR, $good[$i][1], "\n";
}
print "\n";


print "Bad\n";
for my $i (0 .. $#bad)
{
  print $bad[$i][0], $SEPARATOR, $bad[$i][1], "\n";
}
print "\n";


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


