#!/usr/bin/perl -w

use strict;
use warnings;
use File::Basename;

#my $script_name = basename($0);
#print STDERR "$script_name pid=$$\n";

my @max_bleu;
my @max_trans;
my @max_components;
my $max_n=4;

while(defined(my $line=<STDIN>)) {
    if($line=~/^([0-9]+) \|\|\| ([0-9\-\.e]+) [^ ]+ ([^\|]*) \|\|\| (.*?) \|\|\| /) {
        my($sent_id)=$1;
        my($bleu)=$2;
        my $component_string=$3;
        my($trans)=$4;

        $trans='NIL' if(!defined($trans));
        $trans=~s/^[\s\t]*(.*?)[\s\t]*$/$1/;

        if(!defined($max_bleu[$sent_id]) || $bleu>$max_bleu[$sent_id]) {
            $max_bleu[$sent_id]=$bleu;
            $max_trans[$sent_id]=$trans;
            $max_components[$sent_id]=$component_string;
        }
    }
}

my @corpus_components;
for(my $i=0; $i<@max_trans; $i++) {
    my @components=split(/ /,$max_components[$i]);
    for(my $j=0; $j<@components; $j++) {
        $corpus_components[$j]=0 if(!defined($corpus_components[$j]));
        $corpus_components[$j]+=$components[$j];
    }
}

my @prec_n;
for(my $n=0; $n<$max_n; $n++) {
    if($corpus_components[($n*2)+1]==0) {
        $prec_n[$n]=0.01;
    } else {
        $prec_n[$n]=$corpus_components[$n*2]/$corpus_components[($n*2)+1];
    }
    if($prec_n[$n]==0) {
        $prec_n[$n]=0.01;
    }
}

my $prec=0;
for(my $n=0; $n<$max_n; $n++) {
    $prec+= (1/$max_n) * log($prec_n[$n]);    
}

my $bp=&compute_brevaty_penalty($corpus_components[1],$corpus_components[-1]);
my $bleu=$bp*exp($prec);

print STDERR "BLEU=$bleu\n";

sub compute_brevaty_penalty {
    my($c,$r)=@_;
    if($c>$r) {
        return 1;
    } else {
        return exp(1-($r/$c));
    }
}
