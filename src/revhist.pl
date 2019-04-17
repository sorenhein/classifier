#!perl

use strict;
use warnings;

# Extract histogram of cars including reversed ones in current models.

my $SEPARATOR = ";";

if ($#ARGV < 0)
{
  print "Usage: revhist.pl sensor??.txt > file\n";
  exit;
}

my %stats;
my @stat;
my $sensor;

for my $file (@ARGV)
{
  my ($sname, $date, $time);
  my $fileno = -1;
  
  $sensor = $file;
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

      if ($fileno > 0)
      {
        $stats{$sensor}[0]++;
        $stats{$sensor}[1] += $stat[0];
        $stats{$sensor}[2] += $stat[1];
        $stats{$sensor}[3] += $stat[2];
      }
    }
    elsif ($line =~ /^Cars after range/)
    {
      $line = <$fh>;
      my %list;
      while (1)
      {
        $line = <$fh>;
        last if ($line =~ /^\s*$/);
        next if ($line =~ /^\.\.\./);
        $line =~ s/^\s+//;

        my @a = split /\s+/, $line;
        $list{$a[9]}++;
      }

      undef @stat;
      $stat[2] = 0;
      for my $c (keys %list)
      {
        $stat[0]++;
        $stat[1] += $list{$c};
        $stat[2] += $list{$c} if ($c =~ /R/);
      }
    }
  }
  close $fh;
}

$stats{$sensor}[0]++;
$stats{$sensor}[1] += $stat[0];
$stats{$sensor}[2] += $stat[1];
$stats{$sensor}[3] += $stat[2];

print header(), "\n";
for my $s (sort keys %stats)
{
  print entry($s, \@{$stats{$s}}), "\n";
}

exit;


sub header
{
  my $st = sprintf "%-16s%12s%12s%12s%12s", 
    "Sensor", "Count", "Avg cars", "#types", "Avg rev";

  return $st;
}


sub entry
{
  my ($sensor, $eref) = @_;
  my $st = sprintf "%-16s%12d%12.2f%12.2f%12.2f",
    $sensor, 
    $eref->[0], 
    $eref->[2] / $eref->[0], 
    $eref->[1] / $eref->[0], 
    $eref->[3] / $eref->[0];

  return $st;
}

