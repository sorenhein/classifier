#!perl

use strict;
use warnings;

# Extract PeakPartial hit counts from sensor??.txt.

my (@num, @hits);

for my $file (@ARGV)
{
  open my $fi, "<", $file or die "Cannot open $: $!";

  my $sensor = $file;
  $sensor =~ s/\.txt//;

  while (my $line = <$fi>)
  {
    next unless $line =~ /^PP/;
    chomp $line;
    $line =~ s///g;

    if ($line =~ /^PPNUM/)
    {
      my @a = split / /, $line;
      for my $i (1 .. 16)
      {
        $num[$i-1] += $a[$i];
      }
    }
    elsif ($line =~ /^PPHIT/)
    {
      my @a = split / /, $line;
      for my $i (1 .. 16)
      {
        $hits[$i-1] += $a[$i];
      }
    }
  }

  close $fi;
}

my $sumNum = 0; 
my $sumHits = 0;

for my $i (0 .. 15)
{
  if ($num[$i] > 0)
  {
    printf "%04b %6d %6s %6.1f%%\n",
      $i, $num[$i], $hits[$i], 
        ($num[$i] > 0 ? 100. * $hits[$i] / $num[$i] : "-");
  }
  else
  {
    printf "%04b %6d %6s %7s\n",
      $i, $num[$i], $hits[$i], "-";
  }
  $sumNum += $num[$i];
  $sumHits += $hits[$i];
}
print "-" x 26 . "\n";
printf "%4s %6d %6s %6.1f%%\n",
  "Sum", $sumNum, $sumHits, 100. * $sumHits / $sumNum;

exit;

