#!/usr/bin/perl -w

use strict;
use warnings;

my($nbest_file,$src_file,$nbest_file_max,$nbest_file_lai)=@ARGV;

my $num_src_sent=0;
open(F,"<$src_file")||die("Can't open file $src_file: $!\n");
while(defined(my $line=<F>)) {
    $num_src_sent++;
}
close(F);

open(M,">$nbest_file_max");
open(L,">$nbest_file_lai");

open(F,"<$nbest_file")||die("Can't open file $nbest_file: $!\n");
while(defined(my $line=<F>)) {
    $line=~s/^([0-9]+) \|\|\| \|\|\|/$1 \|\|\| NIL \|\|\|/;
    if($line=~/^([0-9]+) \|\|\| (.*) \|\|\| (.*)\n/) {
	my $sent_id=$1;
	my $trans=$2;
	my $scores=$3;

	if($sent_id<$num_src_sent) {
	    print M $line;
	} else {
	    $sent_id=$sent_id-$num_src_sent;
	    print L "$sent_id ||| $trans ||| $scores\n";
	}
    }
}
close(F);
close(M);
close(L);

