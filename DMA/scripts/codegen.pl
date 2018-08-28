#!/usr/bin/perl

use strict;
use Data::Dumper;
use File::Basename;

my $base = dirname $0;

my $xpmImage;

open FH, "<", "$base/dma.xpm" or die "Can't open dma.xpm for read: $!";
while( my $line = <FH> )
{
  if($line =~ /^\s*"(.*)"\s*(?:,|};)\s*$/ )
  {
    $xpmImage .= $1;
  }
}
close(FH);

if( $xpmImage =~ /^\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(.*)$/ )
{
  my $width = $1;
  my $height = $2;
  my $colors = $3;
  my $remaining = $5;
  my $data = substr($remaining, 10 * $colors);
  
  my $pattern = "(" . ('.' x $width) . ")";
  my $regex = qr/$pattern/;
  
  my @matches = ( $data =~ /$regex/g );
  
  my @decoded = map {
    my @arr = map { $_ eq ' ' ? 0 : 1 } ($_ =~ /./g);
    \@arr;
  } @matches;
  
  my @bytes;
  for( my $j=0; $j < $height; $j += 8 )
  {
    for( my $k=0; $k < $width; $k ++ )
    {
      my @arr = map { $decoded[$j+$_][$k] } reverse 0..7;
      my $bin = "0b" . join("", @arr);
      push @bytes, eval $bin;
    }
  }
  
  print "static const uint8_t dma_image [] = {\n";
  while(@bytes)
  {
    my @part = splice @bytes, 0, 16;
    @part = map {sprintf("0x%02X", $_) } @part;
    print "    " . join(", ", @part) . ",\n";
  }
  print "};\n\n";
  
  print "#define _ 0x001F\n";
  print "#define W 0x771F\n";
  
  print "static const uint16_t dma_image [] = {\n";
  for my $row (@decoded)
  {
    my @map = map { $_ ? 'W' : '_' } @$row;
    print "    " . join(",", @map) . ",\n";
  }
  
  print "};\n\n";  
}

