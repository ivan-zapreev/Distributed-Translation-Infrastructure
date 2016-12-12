#!/usr/bin/perl -w

open (TEMPLATE, $ARGV[0]);

while (<TEMPLATE>) {
  chomp;
  @fields = split(/\|\|\|/);
  $line = <STDIN>;
  chomp($line);
  $new_line= "$fields[0] ||| $line ||| $fields[2]\n";
  $new_line=~s/ +/ /g;
  print $new_line;
}

system("touch $ARGV[0].done");
