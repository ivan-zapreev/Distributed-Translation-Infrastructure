#!/usr/bin/perl -w

use strict;
use warnings;

if(@ARGV!=2) {
    print STDERR "usage: lattice-branchings.pl <lattice-bleu-file> <feature_scores_file>\n";
    exit(-1);
}

my($lattice_file,$feature_scores_file)=@ARGV;


open(F,"<$lattice_file")||die("can't open file $lattice_file: $!\n");
open(FEAT,"<$feature_scores_file") || die("can't open $feature_scores_file: $!\n");

while(defined(my $line=<F>)) {
    if($line=~/^<SENT ID=([0-9]+)>/) {

        my $sent_id=$1;
        my $first_line=1;
        my $start_node=0;
        my $end_node;

	next if($sent_id<20);
	last if($sent_id>20);

        my %edge_future_modelcost;
        my %edge_future_bleu;
	my %edge_srclength;
	my %edge_transoutput;
	my %edge_src_span;
	my %node_feature_scores;
        my $covervecs_line;

	my $found_feat=0;
	while(!$found_feat && defined(my $line_feat=<FEAT>)) {
	    if($line_feat=~/^<SENT ID=$sent_id>/) {
		$found_feat=1;
		while(defined($line_feat=<FEAT>) && $line_feat!~/^<\/SENT>/) {
		    chomp($line_feat);
		    my($node_id,@feature_values)=split(/ /,$line_feat);
		    for(my $i=0; $i<@feature_values; $i++) {
			my($feature_id,$feature_value)=split(/\=/,$feature_values[$i]);
			$node_feature_scores{$node_id}{$feature_id}=$feature_value;
		    }
		}
	    }
	}


        while(defined($line=<F>) && $line!~/^<\/SENT>/) {
            if($line=~/^<COVERVECS>/) {
                $covervecs_line=$line;
                $line=~s/^<COVERVECS>//;
                $line=~s/<\/COVERVECS>\n//;
                my @spans=split(/ /,$line);
                for(my $i=0; $i<@spans; $i++) {
                    my($node_to,$node_from,$src_from,$src_to)=$spans[$i]=~/^([0-9]+)\-([0-9]+)\:([0-9]+)\:([0-9]+)$/;
                    $edge_srclength{$node_from}{$node_to}=$src_to-$src_from+1;
		    $edge_src_span{$node_from}{$node_to}="$src_from\-$src_to";
		}
	    } else {
                if($first_line) {
                    $first_line=0;
                    my($node_to,$from_string)=$line=~/^([0-9]+)[\t\s]+(.+)\n/;
                    $end_node=$node_to;
                    my @from_tuples=split(/ /,$from_string);
                    for(my $i=0; $i<@from_tuples; $i++) {
                        my($node_from,$cost)=split(/\|\|\|\|\|\|/o,$from_tuples[$i]);
		    }
		} else {
                    my($node_to,$from_string)=$line=~/^([0-9]+)[\t\s]+(.+)\n/;
                    $from_string.=' ';
                    while($from_string=~s/([0-9]+)\|\|\|([^\|]+)\|\|\|([0-9\-\.e]+) / /) {
                        my $node_from=$1;
                        my $trans=$2;
                        my $cost=$3;
			my($future_bleu,$future_model)=$cost=~/^(.+?)\.(\-.+)$/;
                        $edge_transoutput{$node_from}{$node_to}=$trans;
			$edge_future_bleu{$node_from}{$node_to}=$future_bleu;
			$edge_future_modelcost{$node_from}{$node_to}=$future_model;
		    }
		}
	    }
	}
        # finished reading in all edges.
	&find_branchings(\%edge_future_bleu,\%edge_future_modelcost,\%edge_transoutput,\%edge_src_span,\%node_feature_scores);
    }
}


sub find_branchings {
    my($edge_future_bleu,$edge_future_modelcost,$edge_transoutput,$edge_src_span,$node_feature_scores)=@_;
    

    my $num_branchings=0;

    my %visited;

    foreach my $node_from (sort {$a<=>$b} (keys %$edge_future_bleu)) {
	if( (keys %{ $$edge_future_bleu{$node_from} })>1) {

	    foreach my $node_to1 (keys %{ $$edge_future_bleu{$node_from} }) {
		foreach my $node_to2 (keys %{ $$edge_future_bleu{$node_from} }) {
		    next if($node_to1==$node_to2);

		    if($$edge_future_bleu{$node_from}{$node_to1}>$$edge_future_bleu{$node_from}{$node_to2}
		       && $$edge_future_modelcost{$node_from}{$node_to2} > $$edge_future_modelcost{$node_from}{$node_to1}) {

			my $key_string="$$edge_src_span{$node_from}{$node_to1}\t$$edge_src_span{$node_from}{$node_to2}\t$$edge_transoutput{$node_from}{$node_to1}\t$$edge_transoutput{$node_from}{$node_to2}\t$node_to1\t$node_to2";

			if(exists($visited{$key_string})) {
			    next;
			} else {
			    $visited{$key_string}=1;
			}

			$num_branchings++;
			my @lines;
			push(@lines,"pair=$num_branchings");
			push(@lines,"edge_transoutput{$node_from}{$node_to1}($$edge_src_span{$node_from}{$node_to1})=$$edge_transoutput{$node_from}{$node_to1}");
			push(@lines,"edge_transoutput{$node_from}{$node_to2}($$edge_src_span{$node_from}{$node_to2})=$$edge_transoutput{$node_from}{$node_to2}");
			push(@lines,"edge_future_bleu{$node_from}{$node_to1}=$$edge_future_bleu{$node_from}{$node_to1}");
			push(@lines,"> edge_future_bleu{$node_from}{$node_to2}=$$edge_future_bleu{$node_from}{$node_to2}");
			push(@lines,"edge_future_modelcost{$node_from}{$node_to1}=$$edge_future_modelcost{$node_from}{$node_to1}");
			push(@lines,"< edge_future_modelcost{$node_from}{$node_to2}=$$edge_future_modelcost{$node_from}{$node_to2}");
			push(@lines,&get_sparse_vector_string(\%{ $$node_feature_scores{$node_to1} }));
			push(@lines,&get_sparse_vector_string(\%{ $$node_feature_scores{$node_to2} }));
			my %sparse_vector_diff;
			&compute_sparse_vector_diff(\%{ $$node_feature_scores{$node_to1} },\%{ $$node_feature_scores{$node_to2} },\%sparse_vector_diff);
			push(@lines,&get_sparse_vector_string(\%sparse_vector_diff));

			print join("\n",@lines), "\n\n";
		    }
		}
	    }
	}
    }
}

sub compute_sparse_vector_diff {
    my($features1,$features2,$diff_vec)=@_;

    foreach my $id (keys %$features1) {
	if(exists($$features2{$id})) {
	    $$diff_vec{$id}=$$features1{$id}-$$features2{$id};
	} else {
	    $$diff_vec{$id}=$$features1{$id};
	}
    }
    foreach my $id (keys %$features2) {
	if(!exists($$features1{$id})) {
	    $$diff_vec{$id}=-1*$$features2{$id};
	}
    }
}




sub get_sparse_vector_string {
    my($features)=@_;

    my @parts;
    foreach my $id (sort {$a<=>$b} (keys %$features)) {
	my $num=sprintf("%.2f",$$features{$id});
	push(@parts,"$id=$num");
    }
    return join(' ',@parts);

}
