#!/usr/bin/perl -w

while (<>) {
  @fields = split(/\|\|\|/);
  print "$fields[1]\n";
}
