#!perl

use strict;
use warnings;

# Extract data from sensor??.txt for a specific train, e.g.
# ICE4_DEU_48_N.

# Indexed first by input, then by output train.
my (%output, %sum, %count, %seen);

my $DIR = "cross";
my $AVG_FILE = "avg.csv";

for my $file (@ARGV)
{
  open my $fi, "<", $file or die "Cannot open $: $!";

  while (my $line = <$fi>)
  {
    chomp $line;
    $line =~ s///g;

    if ($line =~ /^SPECTRAIN /)
    {
      my @a = split / /, $line;
      my $trainIn = $a[1];
      my $trainOut = $a[2];

      my $header = <$fi>;
      chomp $header;
      $header =~ s///g;
      $output{$trainIn}{$trainOut}[0] .= ";" . $header;
      $seen{$trainIn}{$trainOut}++;

      while ($line = <$fi>)
      {
        chomp $line;
        $line =~ s///g;
        last if ($line =~ /^ENDSPEC/);

        my @b = split /;/, $line;
        my $pno = $b[0];
        my $pos;
        if (defined $b[1])
        {
          $pos = $b[1];
          $sum{$trainIn}{$trainOut}[$pno] += $pos;
          $pos =~ s/\./,/g;
          $count{$trainIn}{$trainOut}[$pno]++;
        }
        else
        {
          $pos = "";
        }

        if (! defined $output{$trainIn}{$trainOut}[$pno+1])
        {
          $output{$trainIn}{$trainOut}[$pno+1] = $pno;
        }
        $output{$trainIn}{$trainOut}[$pno+1] .= ";" . $pos;
      }
    }
  }
  close $fi;
}

my $favgout = "$DIR/" . $AVG_FILE;
open my $fa, ">", $favgout or die "Cannot open $: $!";

# Write the header for the average file.
# Calculate the averages.

my %averages;
my $h = "";
my $mlen = 0;
for my $k1 (sort keys %output)
{
  my @c = split /_/, $k1;
  for my $k2 (sort keys %{$output{$k1}})
  {
    my @d = split /_/, $k2;

    # $h .= ";" . $c[-1] . $d[-1] . "(" . $seen{$k1}{$k2} . ")";
    # $h .= ";" . $c[-1] . $d[-1] . "flip(" . $seen{$k1}{$k2} . ")";
    $h .= ";" . $c[0] . $d[0] . "(" . $seen{$k1}{$k2} . ")";

    for my $i (0 .. $#{$sum{$k1}{$k2}})
    {
      if (defined $count{$k1}{$k2}[$i] && $count{$k1}{$k2}[$i] > 0)
      {
        $averages{$k1}{$k2}[$i] .= $sum{$k1}{$k2}[$i] / $count{$k1}{$k2}[$i];
      }
      else
      {
        $averages{$k1}{$k2}[$i] .= 0;
      }
    }

    if ($#{$averages{$k1}{$k2}} > $mlen)
    {
      $mlen = $#{$averages{$k1}{$k2}};
    }
  }
}

if ($h eq "")
{
  die "No SPECTRAIN/ENDSPEC present\n";
}
print $fa "$h\n";

for my $i (0 .. $mlen)
{
  print $fa $i;
  for my $k1 (sort keys %output)
  {
    my @c = split /_/, $k1;

    for my $k2 (sort keys %{$output{$k1}})
    {
      my @d = split /_/, $k2;
      # my $s = sprintf(";%7.4f", $averages{$k1}{$k2}[$i]);
      my $s = sprintf(";%d", 1000. * $averages{$k1}{$k2}[$i]);
      $s =~ s/\./,/;
      print $fa $s;
    }
  }
  print $fa "\n";
}
close $fa;

for my $k1 (sort keys %output)
{
  my @c = split /_/, $k1;
  for my $k2 (sort keys %{$output{$k1}})
  {
    my @d = split /_/, $k2;
    my $fout = "$DIR/" . $c[0] . $d[0] . ".csv";

    open my $fo, ">", $fout or die "Cannot open $: $!";
    for my $l (@{$output{$k1}{$k2}})
    {
      print $fo "$l\n";
    }
    close $fo;

  }
}

exit;

