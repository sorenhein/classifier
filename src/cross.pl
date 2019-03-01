#!perl

use strict;
use warnings;

# Extract data from sensor??.txt for a specific train, e.g.
# ICE4_DEU_48_N.

# Indexed first by input, then by output train.
my %output;

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

      while ($line = <$fi>)
      {
        chomp $line;
        $line =~ s///g;
        last if ($line =~ /^ENDSPEC/);

        my @b = split /;/, $line;
        my $pno = $b[0];
        my $pos = $b[1] || "";
        $pos =~ s/\./,/g;

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

for my $k1 (keys %output)
{
  my @c = split /_/, $k1;
  for my $k2 (keys %{$output{$k1}})
  {
    my @d = split /_/, $k2;
    my $fout = "cross/" . $c[-1] . $d[-1] . ".csv";

    open my $fo, ">", $fout or die "Cannot open $: $!";
    for my $l (@{$output{$k1}{$k2}})
    {
      print $fo "$l\n";
    }
    close $fo;
  }
}

exit;

