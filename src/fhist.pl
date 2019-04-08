#!perl

use strict;
use warnings;

# Extract histogram of peak fixes at front of good trains.

my $SEPARATOR = ";";

my $REGRESS_THRESHOLD = 3.;

if ($#ARGV < 0)
{
  print "Usage: fhist.pl [CSV] sensor??.txt > file\n";
  exit;
}

my $FORMAT_TXT = 0;
my $FORMAT_CSV = 0;

my $format;
if (lc($ARGV[0]) eq 'csv')
{
  $format = $FORMAT_CSV;
  shift @ARGV;
}
else
{
  $format = $FORMAT_TXT;
}

my (%histTrain, %histSum);
my (%dhistTrain, %dhistSum);
my @indiv;

for my $file (@ARGV)
{
  my ($sname, $date, $time);
  my $fileno = -1;
  my $state = -1;
  my $true = "";
  
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
      $line = <$fh>;
      chomp $line;
      $line =~ s///g;
      next if ($line eq "");

      my @a = split /\s+/, $line;
      next unless ($a[0] eq $true && $a[2] < $REGRESS_THRESHOLD);

      my $tag = "$a[3]_$a[4]";

      $histTrain{$true}{$tag}++;
      $histSum{$tag}++;

      $dhistTrain{$true}{$a[4]-$a[3]}++;
      $dhistSum{$a[4]-$a[3]}++;

      if ($a[4]-$a[3] >= 4)
      {
        push @indiv, detail($sname, $time, $fileno, $true, $a[2], $tag);
      }

      $state = 3;
    }
  }
  close $fh;
}

print header(\%histSum), "\n";
print entries(\%histTrain, \%histSum), "\n";

print header(\%dhistSum), "\n";
print entries(\%dhistTrain, \%dhistSum), "\n";

for my $line (@indiv)
{
  print $line;
}

exit;


sub headerTXT
{
  my $sumRef = pop;
  my $st = sprintf "%-16s", "Train";

  for my $t (sort keys %$sumRef)
  {
    $st .=  sprintf "%5s", $t;
  }
  return $st;
}


sub headerCSV
{
  my $sumRef = pop;
  my $st = "Train";

  for my $t (sort keys %$sumRef)
  {
    $st .=  $SEPARATOR . $t;
  }
  return $st;
}


sub header
{
  if ($format == $FORMAT_TXT)
  {
    return headerTXT(pop);
  }
  else
  {
    return headerCSV(pop);
  }
}


sub entriesTXT
{
  my ($trainRef, $sumRef) = @_;

  my $st = "";
  for my $train (sort keys %$trainRef)
  {
    $st .= sprintf "%-16s", $train;
    for my $t (sort keys %$sumRef)
    {
      if (defined $trainRef->{$train}{$t})
      {
        $st .= sprintf "%5d", $trainRef->{$train}{$t};
      }
      else
      {
        $st .= sprintf "%5s", "";
      }
    }
    $st .= "\n";
  }

  $st .= sprintf "%-16s", "SUM";
  for my $t (sort keys %$sumRef)
  {
    $st .=  sprintf "%5d", $sumRef->{$t};
  }

  return $st . "\n";
}


sub entriesCSV
{
  my ($trainRef, $sumRef) = @_;

  my $st = "";
  for my $train (sort keys %$trainRef)
  {
    $st .= $train;
    for my $t (sort keys %$sumRef)
    {
      if (defined $trainRef->{$train}{$t})
      {
        $st .= $SEPARATOR . $trainRef->{$train}{$t};
      }
      else
      {
        $st .= $SEPARATOR;
      }
    }
    $st .= "\n";
  }

  $st .= "SUM";
  for my $t (sort keys %$sumRef)
  {
    $st .=  $SEPARATOR . $sumRef->{$t};
  }

  return $st . "\n";
}


sub entries
{
  if ($format == $FORMAT_TXT)
  {
    return entriesTXT(@_);
  }
  else
  {
    return entriesCSV(@_);
  }
}


sub detailTXT
{
  my ($sname, $time, $fno, $true, $dist, $tag) = @_;

  return sprintf "%-6s  %-6s  %4s %-16s  %-4s %-4s\n",
    $sname, $time, $fno, $true, $dist, $tag;
}


sub detailCSV
{
  my ($sname, $time, $fno, $true, $dist, $tag) = @_;

  return $sname . $SEPARATOR .
    $time . $SEPARATOR .
    $fno . $SEPARATOR .
    $true . $SEPARATOR .
    $dist . $SEPARATOR .
    $tag;
}


sub detail
{
  if ($format == $FORMAT_TXT)
  {
    return detailTXT(@_);
  }
  else
  {
    return detailCSV(@_);
  }
}

