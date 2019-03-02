#!perl

use strict;
use warnings;

# Extract data from sensor??.txt to focus on partial first/last cars.

my $firstFlag = 0;
my $select = "PEAKPART   8";

my ($dir, $startTag, $endTag);
if ($firstFlag)
{
  $dir = "first";
  $startTag = "FIRSTCAR ";
  $endTag = "anywheeler";
}
else
{
  $dir = "last";
  $startTag = "LASTCAR ";
  $endTag = "lastwheeler";
}

for my $file (@ARGV)
{
  open my $fi, "<", $file or die "Cannot open $: $!";

  my $fout = "$dir/$file";
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
      if ($firstFlag)
      {
        # The first couple of peak groups (cars).
        my $dashes = 0;
        while ($line = <$fi>)
        {
          chomp $line;
          $line =~ s///g;
          $dashes++ if ($line =~ /^-----/);
          push @lines, $line . "\n";
          last if ($dashes == 2 || $line eq "");
        }
      }
      else
      {
        # The last couple of peak groups.
        my @tmp;
        while ($line = <$fi>)
        {
          chomp $line;
          $line =~ s///g;
          push @tmp, $line;
          last if $line eq "";
        }

        my $dashes = 0;
        my $j;
        for my $i (0 .. $#tmp)
        {
          $j = $#tmp - $i;
          $dashes++ if ($tmp[$j] =~ /^-----/);
          last if $dashes == 2;
        }

        for my $i ($j .. $#tmp)
        {
          push @lines, $tmp[$i] . "\n";
        }
      }

      push @lines, "\n";
      $state = 2;
    }
    elsif ($state == 2 && $line =~ /^$startTag/)
    {
      push @lines, $line . "\n";
      while ($line = <$fi>)
      {
        chomp $line;
        $line =~ s///g;
        $partFlag = 1 if ($line =~ /^$select/);
        last if ($line =~ /$endTag/);
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
