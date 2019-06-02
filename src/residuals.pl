#!perl

use strict;
use warnings;

# Combine two residual files in the cross directory.
# The N file is processed normally, the R file is flipped in time
# and also in sign.  The two are weighted together.

if ($#ARGV != 1)
{
  print "Usage: residuals.pl cross/file_N.txt cross/file_R.txt > cross/file.txt\n";
  exit;
}

my $file1 = $ARGV[0];
my $file2 = $ARGV[1];

my ($count1, @list1);
read_file($file1, \$count1, \@list1);

my ($count2, @list2);
read_file($file2, \$count2, \@list2);

if ($#list1 != $#list2)
{
  die "Different-length lists";
}

print "Index;N;R;Rflip;res\n";
my $l = $#list1;
for my $i (0 .. $l)
{
  my $flip = -$list2[$l-$i];
  my $weigh = int(($count1 * $list1[$i] + $count2 * $flip) / 
    ($count1 + $count2));

  print "$i;$list1[$i];$list2[$i];$flip;$weigh\n";
}


exit;

sub read_file
{
  my ($file, $count_ref, $list_ref) = @_;

  open my $fh, "<", $file or die "Cannot open $file: $!";

  my $line = <$fh>;
  $line =~ /\((\d+)\)/;
  $$count_ref = $1;

  while ($line = <$fh>)
  {
    chomp $line;
    $line =~ s///g;

    $line =~ /;([-0-9]+)/;
    push @$list_ref, $1;
  }
  close $fh;
}

