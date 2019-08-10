#!perl

use strict;
use warnings;

# Provide summary and detail of sensor??.txt files.

my $SEPARATOR = ";";

my $FORMAT_TXT = 0;
my $FORMAT_CSV = 1;

my $sumSensor = "total";

if ($#ARGV < 0)
{
  print "Usage: summarize.pl [CSV] sensor??.txt > file\n";
  exit;
}

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

my @orders;
setOrder();

my (%overall, %warnings, %deviations, %partials, 
  %models, %recognizers, %alignments, %exceptions);

my (@overallHdr, @warningsHdr, @deviationsHdr, @partialsHdr, 
  @modelsHdr, @recognizersHdr, @alignmentsHdr, @exceptionsHdr);
setHeaders();

my %headerMap;
setHeaderMap();

my %hashMap;
setHashMap();

for my $file (@ARGV)
{
  # Per-sensor local house-keeping.
  my @details;
  my ($sname, $date, $time);
  my $fileno = -1;
  
  my $sensor = $file;
  $sensor =~ s/.txt//;

  open my $fh, "<", $file or die "Cannot open $file: $!";

  while (my $line = <$fh>)
  {
    chomp $line;
    $line =~ s///g;

    next unless length($line) >= 6;
    next unless substr($line, 0, 6) eq "Stats:";

    $line =~ /^Stats: (.+)$/;
    my $stats = $1;
    next unless defined $hashMap{$stats};

    $line = <$fh>; # Dashes
    $line = <$fh>; # Empty
    parseStats($fh, \%{$hashMap{$stats}{$sensor}});

    transferStats(\%{$hashMap{$stats}{$sensor}},
      \%{$hashMap{$stats}{$sumSensor}});
  }
  close $fh;
}

printStats();


sub setOrder
{
  @orders = ('Overall', 'Warnings', 'Deviations', 'Partials',
    'Model counts', 'Recognizers', 'Alignment', 'Exceptions');
}


sub setHeaderMap
{
  $hashMap{"Overall"} = \%overall;
  $hashMap{"Warnings"} = \%warnings;
  $hashMap{"Deviations"} = \%deviations;
  $hashMap{"Partials"} = \%partials;
  $hashMap{"Model counts"} = \%models;
  $hashMap{"Recognizers"} = \%recognizers;
  $hashMap{"Alignment"} = \%alignments;
  $hashMap{"Exceptions"} = \%exceptions;
}


sub setHeaders
{
  @overallHdr = ("count", "good", "error", "except", "warn");
  @warningsHdr = (
    'first, 1-4', 
    'first, 5+',
    'mid, 1-4', 
    'mid, 5+',
    'last, 1-4', 
    'last, 5+');
  @deviationsHdr = ("<= 1", "1-3", "3-10", "10-100", "100+");
  @partialsHdr = (
    'Trains with warnings',
    'Trains with partials',
    'Cars with partials',
    'Partial peak count');
  @modelsHdr = ("0", "1", "2", "3", "4", "5+");
  @recognizersHdr = (
    'by 1234 order', 
    'by great quality',
    'by emptiness', 
    'by pattern', 
    'by spacing');
  @alignmentsHdr = (
    'match', 
    'miss early', 
    'miss within', 
    'miss late',
    'spurious early', 
    'spurious within', 
    'spurious late');
  @exceptionsHdr = (
    'Unknown sample rate',
    'Trace file not read',
    'Truth train not known',
    'No alignment matches',
    'No peaks in structure',
    'No nested intervals',
    'No peak scale found when labeling',
    'Not all cars detected');
}


sub setHashMap
{
  $headerMap{"Overall"} = \@overallHdr;
  $headerMap{"Warnings"} = \@warningsHdr;
  $headerMap{"Deviations"} = \@deviationsHdr;
  $headerMap{"Partials"} = \@partialsHdr;
  $headerMap{"Model counts"} = \@modelsHdr;
  $headerMap{"Recognizers"} = \@recognizersHdr;
  $headerMap{"Alignment"} = \@alignmentsHdr;
  $headerMap{"Exceptions"} = \@exceptionsHdr;
}


sub parseStats
{
  my ($fh, $hashRef) = @_;
  while (my $line = <$fh>)
  {
    chomp $line;
    $line =~ s///g;
    return if $line eq "";

    if ($line =~ /^(.*\S)\s+(\d+)$/)
    {
      $hashRef->{$1} += $2;
    }
    else
    {
      print "PARSE ERROR\n";
    }
  }
}


sub transferStats
{
  my ($hashRef, $sumRef) = @_;
  for my $k (keys %$hashRef)
  {
    $sumRef->{$k} += $hashRef->{$k};
  }
}


sub printStatsCSV
{
  print "Sensor" . $SEPARATOR;
  for my $order (@orders)
  {
    print $SEPARATOR;
    for my $h (@{$headerMap{$order}})
    {
      print $h . $SEPARATOR;
    }
  }
  print "\n";

  for my $sensor (sort keys %overall)
  {
    print $sensor . $SEPARATOR;
    for my $order (@orders)
    {
      print $SEPARATOR;
      for my $h (@{$headerMap{$order}})
      {
        print $hashMap{$order}{$sensor}{$h} . $SEPARATOR;
      }
    }
    print "\n";
  }
}


sub printStatsTXT
{
  printf "%10s", "Sensor";
  for my $order (@orders)
  {
    print "  ";
    for my $h (@{$headerMap{$order}})
    {
      print "%-6s", substr($h, 0, 6);
    }
  }
  print "\n";

  for my $sensor (sort keys %overall)
  {
    printf "%10s", $sensor;
    for my $order (@orders)
    {
      print "  ";
      for my $h (@{$headerMap{$order}})
      {
        printf "%-6d", $hashMap{$order}{$sensor}{$h};
      }
    }
    print "\n";
  }
}


sub printStats
{
  if ($format == $FORMAT_CSV)
  {
    printStatsCSV();
  }
  else
  {
    printStatsTXT();
  }
}

