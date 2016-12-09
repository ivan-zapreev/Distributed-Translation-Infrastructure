#!/usr/bin/perl -w

use strict;
use warnings;

while(defined(my $line=<STDIN>)) {
    if($line=~/^([0-9]+ \|\|\| NIL \|\|\|) (.*)\n/) {
        my $translation=$1;
        my $score_string=$2;
        my @scores=split(/ +/,$score_string);
        for(my $i=0; $i<@scores; $i++) {
            $scores[$i]=0;
        }
        $score_string=join(" ",@scores);
        print "$translation $score_string\n";
    } else {
        print $line;
    }
}
