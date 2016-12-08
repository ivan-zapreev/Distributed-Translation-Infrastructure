#!/usr/bin/perl -w

use strict;
use warnings;

my($previous_sparse_decoding_weights_file,@rescored_nbest_files)=@ARGV;

if(@ARGV<2) {
    print STDERR "\nusage: gather-sparse-decoding-features.pl <sparse-decoding-weights> <nbest-list-files>\n\n";
    exit(-1);
}

my %feature_name2id;
my @features;
my @feature_scores;

my $first_feature=1;
my $current_feature_id=-1;
my $empty_scores_string='0';
open(F,"<$previous_sparse_decoding_weights_file")||die("can't open file $previous_sparse_decoding_weights_file: $!\n");
while(defined(my $line=<F>)) {
    $line=~s/[\s\t]*\n//;
    my($feature_id,$feature_name,$scores_string)=split(/ \|\|\| /o,$line);

    $feature_name2id{$feature_name}=$feature_id;
    $features[$feature_id]=$feature_name;
    $feature_scores[$feature_id]=$scores_string;

    if($first_feature) {
	my @scores=split(/ /,$scores_string);
	for(my $i=0; $i<@scores; $i++) {
	    $scores[$i]=0;
	}
	$empty_scores_string=join(' ',@scores);
	$first_feature=0;
    }

    if($feature_id>$current_feature_id) {
	$current_feature_id=$feature_id;
    }
}
close(F);


for(my $i=0; $i<@rescored_nbest_files; $i++) {
    my $nbest_file=$rescored_nbest_files[$i];

    open(F,"<$nbest_file")||die("Can't open $nbest_file: $!\n");
    while(defined(my $line=<F>)) {
	$line=~s/[\s\t]*\n//;
	my($sent_id,$bleu_component_string,$translation,$scores_string)=split(/ \|\|\| /o,$line);
	
	my @feature_value_pairs=split(/ /,$scores_string);
	for(my $j=0; $j<@feature_value_pairs; $j++) {
	    if($feature_value_pairs[$j]=~/^[0-9]+\:[0-9\-e\.]+$/) {
	    } elsif($feature_value_pairs[$j]=~/^([^ ]+)\:([0-9\-e\.])+$/) {
		my $feature_name=$1;
		my $value=$2;

		if(!exists($feature_name2id{$feature_name})) {
		    $current_feature_id++;
		    
		    $feature_name2id{$feature_name}=$current_feature_id;
		    $features[$current_feature_id]=$feature_name;
		    $feature_scores[$current_feature_id]=$empty_scores_string;
		}
	    }
	}
    }
    close(F);
}


for(my $i=0; $i<=$current_feature_id; $i++) {
    if(defined($features[$i])) {
	print "$i ||| $features[$i] ||| $feature_scores[$i]\n";
    }
}
