#!perl

use strict;
use warnings;

# Extract data from sensor??.txt to focus on partial first cars.

my $select = "PEAKPART   1";

for my $file (@ARGV)
{
  open my $fi, "<", $file or die "Cannot open $: $!";

  my $fout = "first/$file";
  open my $fo, ">", $fout or die "Cannot open $: $!";

  my $state = 0;
  my $eno = 0;
  my $seg;
  my $firstFlag;
  my $partFlag;
  my @lines;
  while (my $line = <$fi>)
  {
    chomp $line;
    $line =~ s///g;

    if ($line =~ /^File /)
    {
      $line =~ /\/([0-9_]+)_001_channel1.dat/;
      $seg = $1;
      if ($eno)
      {
        dumplines($fo, $eno-1, $seg, \@lines) if $partFlag;
      }
      @lines = ();
      $eno++;
      $state = 1;
      $firstFlag = 0;
      $partFlag = 0;
    }
    elsif ($state == 1 && $line =~ /of PeakMinima$/)
    {
      my $dashes = 0;
      while ($line = <$fi>)
      {
        chomp $line;
        $line =~ s///g;
        $dashes++ if ($line =~ /^-----/);
        push @lines, $line . "\n";
        last if ($dashes == 2 || $line eq "");
      }
      push @lines, "\n";
      $state = 2;
    }
    elsif ($state == 2 && $line =~ /^FIRSTCAR /)
    {
      push @lines, $line . "\n";
      while ($line = <$fi>)
      {
        chomp $line;
        $line =~ s///g;
        $partFlag = 1 if ($line =~ /^$select/);
        last if ($line =~ /anywheeler/);
        push @lines, $line . "\n";
        last if ($line =~ /No dominant/);
      }
      push @lines, "\n";
      $firstFlag = 1;
      $state = 3;
    }
    elsif ($state == 3 && $line =~ /^Cars after range/)
    {
      while ($line = <$fi>)
      {
        chomp $line;
        $line =~ s///g;
        last if ($line =~ /^All selected/);
        last if ($line =~ /^Range/);
        push @lines, $line . "\n";
      }
      $state = 4;
    }
    elsif ($state == 4 && $line =~ /^True train/)
    {
      push @lines, $line . "\n";
      while ($line = <$fi>)
      {
        chomp $line;
        $line =~ s///g;
        last if ($line =~ /^File /);
        push @lines, $line . "\n";
      }
      dumplines($fo, $eno-1, $seg, \@lines) if $partFlag;

      if ($line && $line =~ /^File /)
      {
        $line =~ /\/([0-9_]+)_001_channel1.dat/;
        $seg = $1;
        @lines = ();
        $eno++;
        $state = 1;
        $firstFlag = 0;
        $partFlag = 0;
      }
    }
  }

  close $fi;
  close $fo;
}

exit;


sub dumplines
{
  my ($fo, $no, $seg, $lines_ref) = @_;
  print $fo "-" x 72 . "\n";
  print $fo "$seg, number $no\n\n";
  for my $line (@$lines_ref)
  {
    print $fo $line;
  }
}
