#!/usr/bin/perl -w

use strict;
use warnings;

my($lattice_file,$O_score_file,$mu,$src_file,@ref_files)=@ARGV;


if(@ARGV<4) {
    print STDERR "\nusage: bleu-lattice-score.pl <lattice-file> <O-score-file> <src-file> <ref-files>\n\n";
    exit(-1);
}

my @O_scores;
&read_O_scores($O_score_file,\@O_scores);


my $max_n=4;
my $use_loss_augmented_inference=1;
#my $mu=0.015;

#my $mu=0.05;
#my $mu=1000;

my $bp_strategy='closest';
my $remove_OOV=1;
my $add_s_tag=1;
my $base=100;

my @ref_ngrams;
my @ref_lengths;
&get_ref_ngrams(\@ref_ngrams,\@ref_files,\@ref_lengths);

my @src_lengths;
open(F,"<$src_file");
my $sent_id=-1;
while(defined(my $line=<F>)) {
    chomp($line);
    $sent_id++;
    my @tokens=split(/ +/,$line);
    $src_lengths[$sent_id]=@tokens;
}
close(F);


open(F,"<$lattice_file")||die("can't open file $lattice_file: $!\n");
while(defined(my $line=<F>)) {
#    if($line=~/^<SENT ID=([0-9]+)>/) {
    if($line=~/^<SENT ID=(740)>/) {
	my $sent_id=$1;
	my $first_line=1;
	my $start_node=0;
	my $end_node;

	next if($sent_id<730);

	my %edge_modelcost;
	my %edge_trans;
	my %edge_transoutput;
	my %edge_inv;
	my %edge_srclength;
	my %edge_trglength;
	my %edge_bleu;
	my %nodes;

	my $covervecs_line;
	# read in all edge information:
	while(defined($line=<F>) && $line!~/^<\/SENT>/) {
	    if($line=~/^<COVERVECS>/) {
		$covervecs_line=$line;
		$line=~s/^<COVERVECS>//;
		$line=~s/<\/COVERVECS>\n//;
		my @spans=split(/ /,$line);
		for(my $i=0; $i<@spans; $i++) {
		    my($node_to,$node_from,$src_from,$src_to)=$spans[$i]=~/^([0-9]+)\-([0-9]+)\:([0-9]+)\:([0-9]+)$/;
		    $edge_srclength{$node_from}{$node_to}=$src_to-$src_from+1;
		    $nodes{$node_from}=1;
		    $nodes{$node_to}=1;
		}
	    } else {
		if($first_line) {
		    $first_line=0;
		    my($node_to,$from_string)=$line=~/^([0-9]+)\t(.+)\n/;
		    $end_node=$node_to;
		    my @from_tuples=split(/ /,$from_string);
		    for(my $i=0; $i<@from_tuples; $i++) {
			my($node_from,$cost)=split(/\|\|\|\|\|\|/o,$from_tuples[$i]);
			$cost=&log_base(exp($cost),$base);
			$edge_modelcost{$node_from}{$node_to}=$cost;
			$edge_trans{$node_from}{$node_to}='';
			$edge_transoutput{$node_from}{$node_to}='';
			$edge_inv{$node_to}{$node_from}=1;
			$edge_trglength{$node_from}{$node_to}=0;
			$edge_srclength{$node_from}{$node_to}=0;
			$nodes{$node_from}=1;
			$nodes{$node_to}=1;
		    }
		} else {
		    my($node_to,$from_string)=$line=~/^([0-9]+)\t(.+)\n/;
		    $from_string.=' ';
		    while($from_string=~s/([0-9]+)\|\|\|([^\|]+)\|\|\|([0-9\-\.e]+) / /) {
#		    while($from_string=~s/([0-9]+)\|\|\|(.*?)\|\|\|([0-9\-\.e]+) / /) {
			my $node_from=$1;
			my $trans=$2;
			my $cost=$3;

			$edge_transoutput{$node_from}{$node_to}=$trans;
			if($remove_OOV) {
			    my @trans_tokens=split(/ /,$trans);
			    my @trans_tokens_noOOV;
			    for(my $i=0; $i<@trans_tokens; $i++) {
				if($trans_tokens[$i]=~/^[0-9A-Za-z\~\`\@\#\$\%\^\&\*\(\)\-\_\=\+\[\{\]\}\\\|\;\:\'\"\,\<\.\>\/\?]+$/) {
				    push(@trans_tokens_noOOV,$trans_tokens[$i]);
				}
			    }
			    $trans=join(' ',@trans_tokens_noOOV);
			    $trans='' if(!defined($trans));
			}

#			print STDERR "cost log_e=$cost ";
			$cost=&log_base(exp($cost),$base);
#			print STDERR "cost log_100=$cost\n";
			$edge_modelcost{$node_from}{$node_to}=$cost;
			$edge_trans{$node_from}{$node_to}=$trans;
			print STDERR "edge_trans{$node_from}{$node_to}=$trans\n";
			$edge_inv{$node_to}{$node_from}=1;
			my @tokens=split(/ /,$trans);
			$edge_trglength{$node_from}{$node_to}=@tokens;
			$nodes{$node_from}=1;
			$nodes{$node_to}=1;
		    }
		}
	    }
	}

	# finished reading in all edges.

	# determine number of incoming edges:
	my %node_incomremain;
	my %node_incomremain_arg;
	foreach my $node (keys %nodes) {
	    if(exists($edge_inv{$node})) {
		my $num=(keys %{ $edge_inv{$node} });
		$node_incomremain{$node}=$num;
		foreach my $node_from (keys %{ $edge_inv{$node} }) {
		    if($node_from==0 || exists($edge_inv{$node_from})) {
			$node_incomremain_arg{$node}{$node_from}=1;
		    }
		}
		$num=(keys %{ $node_incomremain_arg{$node} });
		$node_incomremain{$node}=$num;
	    } else {
		$node_incomremain{$node}=0;
	    }
	}

	my %node_hist;
	my %node_O;
	my %node_bleu;
	my %node_max;
	my %node_maxarg;
	if($add_s_tag) {
	    @{ $node_hist{0} }=( '<s>' );
	} else {
	    @{ $node_hist{0} }=();
	}
	@{ $node_O{0} }=@{ $O_scores[$sent_id] };
	$node_max{0}=0;
	if($node_O{0}[-1]==0) {
	    $node_bleu{0}=0;
	} else {
	    $node_bleu{0}=&compute_O_bleu_vec(\@{ $node_O{0} });
	}

	my $finished=0;
	my %visited;
	my %nodes_with_hist;
	my @queue=( $start_node );
	while(@queue>0) {
	    my %queue_next;
	    for(my $i=0; $i<@queue; $i++) {
		my $node_from=$queue[$i];
		if($node_from==10317) {
		    print STDERR "FROM node_from==10317\n";
		}
		foreach my $node_to (keys %{ $edge_trans{$node_from} }) {
		    print STDERR " to=$node_to\n";
		    print STDERR "node_from=$node_from node_to=$node_to\n";
		    if(!exists($nodes_with_hist{$node_to})) {
			&build_hist(\@{ $node_hist{$node_from} },$edge_trans{$node_from}{$node_to},\@{ $node_hist{$node_to} });

			if($node_to==79009) {
			    print STDERR "to: node_hist{$node_to}=", join('_',@{ $node_hist{$node_to} }), "\n";
			}
			if($node_from==79009) {
			    print STDERR "from: node_hist{$node_from}=", join('_',@{ $node_hist{$node_from} }), "\n";
			}

			if(!defined($node_hist{$node_from})) {
			    print STDERR "undef from: node_hist{$node_from}\n";
			}
			if(!defined($node_hist{$node_to})) {
			    print STDERR "undef to: node_hist{$node_to}\n";
			}

#			my $hist_string=join("\_",@{ $node_hist{$node_to} });
#			print STDERR "node_hist{$node_to}=$hist_string\n";
			$nodes_with_hist{$node_to}=1;
		    }
		    my @tokens_to=split(/ /,$edge_trans{$node_from}{$node_to});
		    my @ngrams_total;
		    my @ngrams_correct;
		    my @edge_counts;
		    my @counts_to;
		    if($edge_srclength{$node_from}{$node_to}==0) {
			$edge_bleu{$node_from}{$node_to}=0;
			$node_bleu{$node_to}=$node_bleu{$node_from};
			@{ $node_O{$node_to} }=@{ $node_O{$node_from} };
		    } else {
			&get_edge_counts($sent_id,\@{ $node_hist{$node_from} },\@tokens_to,$edge_srclength{$node_from}{$node_to},\@edge_counts);
#			print STDERR join(' ',@edge_counts), "\n";
			
			my $bleu_to=&compute_O_bleu($sent_id,$edge_srclength{$node_from}{$node_to},\@{ $node_O{$node_from} },\@edge_counts,\@counts_to);
			my $edge_bleu_cost=$bleu_to-$node_bleu{$node_from};
			my $bleu_combined=$edge_bleu_cost-$mu*($edge_bleu_cost-$edge_modelcost{$node_from}{$node_to});

			$edge_bleu{$node_from}{$node_to}=$bleu_combined;
			if(!exists($node_bleu{$node_to}) || $bleu_to>$node_bleu{$node_to}) {
			    $node_bleu{$node_to}=$bleu_to;
			    @{ $node_O{$node_to} }=@counts_to;
			}
#			$edge_bleu{$node_from}{$node_to}=&compute_O_bleu($sent_id,$edge_srclength{$node_from}{$node_to},\@{ $node_O{$node_from} },\@edge_counts,\@counts_to);
#			print  "edge_bleu{$node_from}{$node_to}=$edge_bleu{$node_from}{$node_to}\n";
		    }
		    
		    $node_incomremain{$node_to}--;
		    delete($node_incomremain_arg{$node_to}{$node_from});
		    print STDERR "node_incomremain{$node_to}=$node_incomremain{$node_to}\n";
		    if($node_incomremain{$node_to}==0) {
			$queue_next{$node_to}=1;
			print STDERR "queue_next{$node_to}\n";

			foreach my $node_from (keys %{ $edge_inv{$node_to} }) {
			    if(!defined($node_max{$node_to}) 
			       || $node_max{$node_from}+$edge_bleu{$node_from}{$node_to}>$node_max{$node_to}) {
				$node_max{$node_to}=$node_max{$node_from}+$edge_bleu{$node_from}{$node_to};
				$node_maxarg{$node_to}=$node_from;
			    }
			}

		    }
		}
		$visited{$node_from}=1;
	    }
	    undef @queue;
	    foreach my $node_to (keys %queue_next) {
		if(!exists($visited{$node_to})) {
		    push(@queue,$node_to);
		}
	    }
	}

	foreach my $node (keys %nodes) {
	    if(!exists($nodes_with_hist{$node})) {
		print STDERR "undef: nodes_with_hist{$node}\n";
	    }
	}

	my $min_node;
	foreach my $node (keys %node_incomremain) {
	    if($node_incomremain{$node}>0 && (!defined($min_node) || $node<$min_node)) {
		$min_node=$node;
	    }
	}
	if(defined($min_node)) {
	    print STDERR "min_node=$min_node\n";
	    foreach my $node_from (keys %{ $node_incomremain_arg{$min_node} }) {
		print STDERR "missing: $node_from\n";
	    }
	}
	

#	die;
	# print re-scored graph:
	my %visited_print;
	@queue=( $end_node );
	print "<SENT ID=$sent_id>\n";
	print STDERR "<SENT ID=$sent_id>\n";

	while(@queue>0) {
	    my %queue_next;
	    for(my $i=0; $i<@queue; $i++) {
		my $node_to=$queue[$i];
		next if($node_to==0);
		my @from_parts;
		foreach my $node_from (keys %{ $edge_inv{$node_to} }) {
		    next if($node_from!=0 && !exists($edge_inv{$node_from}));
#		    print STDERR "node_from=$node_from edge_trans{$node_from}{$node_to}=$edge_trans{$node_from}{$node_to} edge_bleu{$node_from}{$node_to}=$edge_bleu{$node_from}{$node_to}\n";

		if(!defined($node_hist{$node_to})) {
		    next;
		    print STDERR "Problem sent_id=$sent_id node_hist{$node_to}\n";
		}

		    my $hist_string=join("\_",@{ $node_hist{$node_from} });
		    my $O_string=join("\,",@{ $node_O{$node_from} });

#		    push(@from_parts,"$node_from:$node_bleu{$node_from}:$hist_string:($O_string)\|\|\|$edge_trans{$node_from}{$node_to}\|\|\|$edge_bleu{$node_from}{$node_to}");
		    my $cost=&log_nulldef($base**$edge_bleu{$node_from}{$node_to});
		    push(@from_parts,"$node_from\|\|\|$edge_transoutput{$node_from}{$node_to}\|\|\|$cost");

		    $queue_next{$node_from}=1;
		}

		my $hist_string=join("\_",@{ $node_hist{$node_to} });
		my $O_string=join("\,",@{ $node_O{$node_to} });

		if(!defined($hist_string)) {
		    print STDERR "Problem sent_id=$sent_id\n";
		}

#		print "$node_to:$node_bleu{$node_to}:$hist_string:($O_string)\t", join(' ',@from_parts), "\n";
		print "$node_to\t", join(' ',@from_parts), "\n";

		$visited_print{$node_to}=1;
	    }
	    undef @queue;
	    foreach my $node_from (sort {$a<=>$b} (keys %queue_next)) {
		if(!exists($visited_print{$node_from})) {
		    push(@queue,$node_from);
		}
	    }
	}
	print $covervecs_line;
	print "<\/SENT>\n";

	#read out viterbi path:
	if(0) {
	    my $maxarg=$end_node;
	    my @viterbi_args;
	    my @viterbi_costs;
	    while(defined($maxarg)) {
		my $maxarg_from=$node_maxarg{$maxarg};
		if(defined($maxarg_from)) {
		    unshift(@viterbi_args,$edge_trans{$maxarg_from}{$maxarg});
		    unshift(@viterbi_costs,$edge_bleu{$maxarg_from}{$maxarg});
		}
		$maxarg=$maxarg_from;	    
	    }
	    my $viterbi_trans=join(' ',@viterbi_args);
	    print STDERR "viterbi_path=$viterbi_trans\n";
	}
#	die;

    }
}
close(F);

sub compute_O_bleu_vec {
    my($O_scores)=@_;
    my @prec_n;
    for(my $n=0; $n<$max_n; $n++) {
	if($O_scores->[($n*2)+1]==0) {
	    $prec_n[$n]=0.01;
	} else {
	    $prec_n[$n]=$O_scores->[$n*2]/$O_scores->[($n*2)+1];
	}
	if($prec_n[$n]==0) {
	    $prec_n[$n]=0.01;
	}
    }

    my $prec=0;
    for(my $n=0; $n<$max_n; $n++) {
	$prec+= (1/$max_n) * log($prec_n[$n]);    
    }

    my $bp=&compute_brevaty_penalty($O_scores->[1],$O_scores->[-2]);
#    my $bleu=$O_scores->[-1]*$bp*exp($prec);
    my $bleu=$bp*exp($prec);
    return &log_nulldef($bleu);
#    return $bleu;
}

sub compute_O_bleu {
    my($sent_id,$edge_srclength,$O_from,$edge_counts,$O_comb)=@_;

#    my @O_comb;
#    for(my $i=0; $i<@{ $O_scores[$sent_id] }; $i++) {
#	$O_comb[$i]=$O_scores[$sent_id][$i]+$edge_counts->[$i];
#    }
    for(my $i=0; $i<@$O_from; $i++) {
	$O_comb->[$i]=$O_from->[$i]+$edge_counts->[$i];
    }

    my @prec_n;
    for(my $n=0; $n<$max_n; $n++) {
	if($O_comb->[($n*2)+1]==0) {
	    $prec_n[$n]=0.01;
	} else {
	    $prec_n[$n]=$O_comb->[$n*2]/$O_comb->[($n*2)+1];
	}
	if($prec_n[$n]==0) {
	    $prec_n[$n]=0.01;
	}
    }

    my $prec=0;
    for(my $n=0; $n<$max_n; $n++) {
	$prec+= (1/$max_n) * log($prec_n[$n]);
# for debugging only:
#	$prec+= (1/$max_n) * 1;
    }

    my $bp=&compute_brevaty_penalty($O_comb->[1],$O_comb->[-2]);
#    my $bleu=$O_comb->[-1]*$bp*exp($prec);
    my $bleu=$bp*exp($prec);
    return &log_nulldef($bleu);
#    return $bleu;
}

sub log_nulldef {
    if($_[0]==0) {
	return -100;
    } else {
	return log($_[0]);
    }
}

sub get_edge_counts {
    my($sent_id,$tokens_hist,$tokens_trans,$edge_srclength,$edge_counts)=@_;

    my @combined_tokens=@$tokens_hist;

    my $scaled_tstlength=@$tokens_trans * ($src_lengths[$sent_id]/$edge_srclength);
    my $ref_length=&compute_ref_seg_length($bp_strategy,$scaled_tstlength,\%{ $ref_lengths[$sent_id] });
    my $scaled_reflength=$ref_length*($edge_srclength/$src_lengths[$sent_id]);

    for(my $i=0; $i<@$tokens_trans; $i++) {
	push(@combined_tokens,$tokens_trans->[$i]);
    }
    for(my $i=@$tokens_hist; $i<@combined_tokens; $i++) {
	for(my $j=$i; $j>=0 && $j>$i-$max_n; $j--) {
	    my $ngram=join(' ',@combined_tokens[$j..$i]);
	    my $n=$i-$j+1;
	    $edge_counts->[(($n-1)*2)+1]++;
	    if(exists($ref_ngrams[$sent_id]{$ngram})) {
		$edge_counts->[($n-1)*2]++;
	    }
#	    print STDERR "edge_counts->[(($n-1)*2)]=$edge_counts->[(($n-1)*2)]\n";
	}
    }
    for(my $i=0; $i<$max_n*2; $i++) {
	if(!defined($edge_counts->[$i])) {
	    $edge_counts->[$i]=0;
	}
    }
    if(0) {
	print STDERR "hist=", join(' ',@$tokens_hist), " trans=", join(' ',@$tokens_trans), "\n";
	print STDERR "combined: ", join(' ',@combined_tokens), "\n";
	print STDERR "edge_counts: ", join(',',@$edge_counts), "\n\n";
    }
#    print STDERR "edge_counts->[0]=$edge_counts->[0] scaled_reflength=$scaled_reflength\n";
    $edge_counts->[$max_n*2]=$scaled_reflength;
    $edge_counts->[($max_n*2)+1]=$edge_srclength;    
}


sub build_hist {
    my($node_from_hist,$trans,$new_hist)=@_;
    my @tokens=split(/ /,$trans);
    while(@tokens>$max_n-1) {
	shift(@tokens);
    }
    for(my $i=@$node_from_hist-1; $i>=0 && @tokens<$max_n-1; $i--) {
	unshift(@tokens,$node_from_hist->[$i]);
    }

    for(my $i=0; $i<@tokens; $i++) {
	$new_hist->[$i]=$tokens[$i];
    }
}



sub get_ref_ngrams {
    my($ref_ngrams,$ref_files,$ref_lengths)=@_;

    my @ref_ngrams_raw;
    my $ref=-1;
    for(my $i=0; $i<@$ref_files; $i++) {
	$ref++;
	open(F,"<$ref_files->[$i]");
	my $sent_id=-1;
	while(defined(my $line=<F>)) {
	    chomp($line);
	    $sent_id++;
	    my @tokens=split(/ +/,$line);
	    $ref_lengths->[$sent_id]{$ref}=@tokens;

	    if($add_s_tag) {
		unshift(@tokens,'<s>');
	    }
	    for(my $i=0; $i<@tokens; $i++) {
		for(my $j=$i; $j<@tokens && $j<$i+$max_n; $j++) {
		    my $ngram=join(' ',@tokens[$i..$j]);
		    $ref_ngrams_raw[$sent_id]{$ngram}{$ref}++;
		}
	    }
	}
	close(F);
    }

    for(my $sent_id=0; $sent_id<@ref_ngrams_raw; $sent_id++) {
	foreach my $ngram (keys %{ $ref_ngrams_raw[$sent_id] }) {
	    my $clipped_count;
	    foreach my $ref (keys %{ $ref_ngrams_raw[$sent_id]{$ngram} }) {
		if(!defined($clipped_count) || $ref_ngrams_raw[$sent_id]{$ngram}{$ref}>$clipped_count) {
		    $clipped_count=$ref_ngrams_raw[$sent_id]{$ngram}{$ref};
		}
	    }
	    $$ref_ngrams[$sent_id]{$ngram}=$clipped_count;
	}
    }
}


sub compute_ref_seg_length {
    my($brev_penality_strategy,$tst_length,$ref_lengths)=@_;

    if($brev_penality_strategy eq 'closest') {
	my $distance;
	my $min_dist_ref_length;
	foreach my $ref_id (keys %$ref_lengths) {
	    my $ref_id_length=$$ref_lengths{$ref_id};

	    if(!defined($distance)) {
		$distance=abs($tst_length-$ref_id_length);
		$min_dist_ref_length=$ref_id_length;
	    } elsif(abs($tst_length-$ref_id_length)<$distance) {
		$distance=abs($tst_length-$ref_id_length);
		$min_dist_ref_length=$ref_id_length;
	    } elsif(abs($tst_length-$ref_id_length)==$distance && $ref_id_length<$min_dist_ref_length) {
		$distance=abs($tst_length-$ref_id_length);
		$min_dist_ref_length=$ref_id_length;
	    }
	}
	return $min_dist_ref_length;
    } elsif($brev_penality_strategy eq 'shortest') {
	my $min_length;
	foreach my $ref_id (keys %$ref_lengths) {
	    my $ref_id_length=$$ref_lengths{$ref_id};
	    if(!defined($min_length) || $ref_id_length<$min_length) {
		$min_length=$ref_id_length;
	    }
	}
	return $min_length;
    } elsif($brev_penality_strategy eq 'longest') {
	my $max_length;
	foreach my $ref_id (keys %$ref_lengths) {
	    my $ref_id_length=$$ref_lengths{$ref_id};
	    if(!defined($max_length) || $ref_id_length>$max_length) {
		$max_length=$ref_id_length;
	    }
	}
	return $max_length;
    }
}



sub min {
    return $_[0] if($_[0]<$_[1]);
    return $_[1];
}

sub max {
    return $_[0] if($_[0]>$_[1]);
    return $_[1];
}

sub compute_brevaty_penalty {
    my($c,$r)=@_;
    if($c>$r) {
	return 1;
    } else {
	return exp(1-($r/$c));
    }
}

sub read_O_scores {
    my($O_file,$O_scores)=@_;

    open(F,"<$O_file")||die("can't open file $O_file: $!\n");
    while(defined(my $line=<F>)) {
        if($line=~/^([0-9]+) \|\|\| (.+)\n/) {
            my $sent_id=$1;
            my $O_string=$2;
            my @o_values=split(/ /,$O_string);
            for(my $i=0; $i<@o_values; $i++) {
                $O_scores->[$sent_id][$i]=$o_values[$i];
            }
        }
    }
    close(F);
    @{ $O_scores->[0] }=@{ $O_scores->[-1] };
}

sub log_base {
    my($val,$base)=@_;

    if($base eq 'e') {
	return log($val);
    } else {
	return log($val)/log($base);
    }
}
