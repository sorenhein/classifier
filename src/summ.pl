#!perl

use strict;
use warnings;

# Summarize the errors in a sensor??.txt file

my ($file, $filebase);
if ($#ARGV == 0)
{
  $file = $ARGV[0];
  $filebase = $file;
  $filebase =~ s/.txt//;
}
else
{
  print "Usage: summ.pl sensor00.txt";
  exit;
}

my ($excepts, $series, $peaks);
my ($sum_excepts, $sum_series, $sum_peaks);

my $fileno = -1;
my @mismatches;
my ($sname, $date, $time);
my (@outmiss, @outwarn, @outexcept);

open my $fh, "<", $file or die "Cannot open $: $!";
while (my $line = <$fh>)
{
  chomp $line;
  $line =~ s///g;

  if ($line =~ /^File/)
  {
    if ($#mismatches >= 0)
    {
      push_misses(\@mismatches, \@outwarn);
    }

    $line =~ /^File.*\/([0-9_]*)_001_channel1.dat:/;
    my @a = split /_/, $1;
    $date = $a[0];
    $time = $a[1];
    $sname = $a[2];

    @mismatches = ();
    $fileno++;
  }
  elsif ($line =~ /^WARNFINAL/)
  {
    @mismatches = ();
    while (1)
    {
      $line = <$fh>;
      chomp $line;
      $line =~ s///g;
      last if $line =~ /^\s*$/;

      my $mline = $line;

      # Also get the profile lines.
      $line = <$fh>;
      chomp $line;
      $line =~ s///g;
      last if $line =~ /^\s*$/;
      $line =~ s/^PROFILE .*:/XX/;

      push @mismatches, $mline . $line;
    }
  }
  elsif ($line =~ /MISMATCH/)
  {
    push_misses(\@mismatches, \@outmiss);
    @mismatches = ();
  }
  elsif ($line =~ /^Exception/)
  {
    $line =~ /^Exception.*function (.*), line number (\d+)/;
    my $fnc = $1;
    my $lno = $2;
    $line = <$fh>;
    $line = <$fh>;
    chomp $line;
    $line =~ s///g;

    my $p = sprintf "%-10s  #%2d  ", $time, $fileno;
    push @outexcept, $p . $fnc . " line " . $lno . ": " . $line;
    @mismatches = ();
  }
}

if ($#mismatches >= 0)
{
  push_misses(\@mismatches, \@outwarn);
}

close $fh;

print "Sensor $filebase ($sname):\n\n";
print "Mismatches\n";
print "$_\n" for @outmiss;
print "\n";
print "Warnings\n";
print "$_\n" for @outwarn;
print "\n";
print "Exceptions\n";
print "$_\n" for @outexcept;
exit;


sub push_misses
{
  my ($mis_ref, $out_ref) = @_;

  my $p;
  if ($#$mis_ref == -1)
  {
    $p = sprintf "%-10s  #%2d  ", $time, $fileno;
    push @$out_ref, $p . "[no warning]";
    push @$out_ref, "";
    return;
  }

  my $first = 1;
  for my $o (@$mis_ref)
  {
    if ($first)
    {
      $p = sprintf "%-10s  #%2d  ", $time, $fileno;
      $first = 0;
    }
    else
    {
      $p = " " x 17;
    }
    push @$out_ref, $p . line_to_line($o);
  }
  push @$out_ref, "";
}


sub line_to_line
{
  my $text = pop;

  my @a = split ":", $text;
  my ($pos, $g1, $range, $g2);
  if ($a[0] =~ /^first/)
  {
    $pos = "first";
  }
  elsif ($a[0] =~ /^last/)
  {
    $pos = "last";
  }
  elsif ($a[0] =~ /^intra/)
  {
    $pos = "inner";
  }
  else
  {
    $pos = "";
  }

  if ($a[1] !~ /\((.*gap\)) (.*) \((.*gap)\)/)
  {
    print "Couldn't parse '$a[1]'\n";
  }

  $range = $2;

  if ($1 eq "gap")
  {
    $g1 = "+";
  }
  else
  {
    $g1 = "-";
  }

  if ($3 eq "gap")
  {
    $g2 = "+";
  }
  else
  {
    $g2 = "-";
  }

  $a[1] =~ /XX (.*)/;

  return sprintf "%-7s %-1s %-12s %-1s    %s", $pos, $g1, $range, $g2, $1;
}

