#!/usr/bin/perl -w

use strict;
use warnings;

if(@ARGV<1) {
    print STDERR "\nusage: initialize-sparse-weights.pl <sparse-feature-weights-file> <value (def=0.1)>\n\n";
    exit(-1);
}

my($file,$init_value)=@ARGV;

$init_value||=0.01;

open(F,"<$file")||die("can't open file $file: $!\n");
my @buffer;
while(defined(my $line=<F>)) {
    chomp($line);
    my($id,$feature_name,@old_values)=split(/ \|\|\| /o,$line);
    push(@buffer,"$id ||| $feature_name ||| $init_value\n");
}
close(F);
open(F,">$file")||die("can't open file $file: $!\n");
for(my $i=0; $i<@buffer; $i++) {
    print F $buffer[$i];
}
close(F);
