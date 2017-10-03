#!/usr/bin/perl -w

use strict;
use warnings;

while(defined(my $line=<STDIN>)) {
    if($line=~/ 0\.?0*\n/) {
    } elsif($line=~/\x00/) {
    } else {
	chomp($line);
	my @fields=split(/ /,$line);
	if(@fields==3) {
	    print $line, "\n";
	}
    }
}
