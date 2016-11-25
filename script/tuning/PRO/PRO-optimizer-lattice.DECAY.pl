#!/usr/bin/perl -w

use strict;
use warnings;
use Getopt::Long "GetOptions";

my $_HELP=0;
my $work_dir;
my $sample_id;
my $nbest_file_scored;
my $config_file;
my $lattice_oracle_scored;
my $model_nbest_scored;
my $model_nbest_lai_scored;
my $decay_factor=1;
my $iteration=0;

GetOptions(
    "conf=s" => \$config_file,
    "work-dir=s" => \$work_dir,
    "sample-id=s" => \$sample_id,
    "nbest-pool=s" => \$nbest_file_scored,
    "lattice-oracle-scored=s" => \$lattice_oracle_scored,
    "model-nbest-scored=s" => \$model_nbest_scored,
    "lattice-lai-scored=s" => \$model_nbest_lai_scored,
    "decay=s" => \$decay_factor,
    "iteration=i" => \$iteration,
    "help" => \$_HELP
	   );

if(!defined($nbest_file_scored)) {
    $_HELP=1;
}


#my($work_dir,$nbest_file_scored,$sample_id,$lattice_oracle_scored,$model_nbest_scored,$model_nbest_lai_scored)=@ARGV;



if(!defined($sample_id)) {
    $sample_id='';
}

print STDERR "PRO-optimizer.pl pid=$$\n";

my $OISTERHOME=$ENV{'OISTERHOME'};

my $gamma_sample_max_size=5000;
my $xi_sample_size=100;
my $alpha_strategy='sigmoid';
my $alpha_step_threshold=0.05;
my $use_instance_weight=0;
my $max_iterations=100;
my $oracle_max=10;

my $sparse_decoding_feature_file;
our %sparse_decoding_name2id;
my $lex_feature_file;
my $largest_lex_feature_id=-1;
if(defined($config_file)) {
    open(F,"<$config_file")||die("can't open $config_file: $!\n");
    while(defined(my $line=<F>)) {
	if($line=~/^data:sparse\_decoding\_feature\_weights=([^ ]+)[\s\t]*\n/) {
	    $sparse_decoding_feature_file=$1;
	} elsif($line=~/^data:lexfeat_weights=([^ ]+)[\s\t]*\n/) {
	    $lex_feature_file=$1;
	}
    }
    close(F);

    if(defined($lex_feature_file)) {
	open(F,"<$lex_feature_file")||die("can't open $lex_feature_file: $!\n");
	while(defined(my $line=<F>)) {
	    chomp($line);
	    my($id,$name,$scores_string)=split(/ \|\|\| /o,$line);
	    if($id>$largest_lex_feature_id) {
		$largest_lex_feature_id=$id;
	    }
	}
	close(F);
    }

    if(defined($sparse_decoding_feature_file)) {
	open(F,"<$sparse_decoding_feature_file")||die("can't open $sparse_decoding_feature_file: $!\n");
	while(defined(my $line=<F>)) {
	    chomp($line);
	    my($id,$name,$scores_string)=split(/ \|\|\| /o,$line);
	    my($id_num)=$id=~/^s([0-9]+)$/;
	    $sparse_decoding_name2id{$id}=$id_num;
#	    $sparse_decoding_name2id{$name}=$id;
	}
	close(F);
    }
	
}

my $classifier_data_file="$nbest_file_scored.classifier_data$sample_id";
my $classifier_weights_file="$nbest_file_scored.classifier_weights$sample_id";
my $classifier_err_log="$nbest_file_scored.classifier_err_log$sample_id";

#my $classifier_call="$SMTAMS/software/MegaM/current/bin/megam_i686.opt -fvals -maxi $max_iterations binary $classifier_data_file 1> $classifier_weights_file 2> $classifier_err_log";
#-nobias
my $classifier_call="$OISTERHOME/resources/software/MegaM/current/bin/megam_i686.opt -nobias -fvals -maxi $max_iterations binary $classifier_data_file 2> $classifier_err_log";


my $prev_sent_id;
my @candidates_obj;
my @candidates_features;
my @candidates_instance_weights;
my @candidates_iteration;
my $instance_weight;
my @Xi;
open(C,">$classifier_data_file") || die("can't open file $classifier_data_file: $!\n");
open(F,"<$nbest_file_scored")||die("can't open file $nbest_file_scored: $!\n");
while(defined(my $line=<F>)) {
    chomp($line);
    my($sent_id,$inserted_iteration,$obj_func_string,$translation,$score_string)=split(/ \|\|\| /o,$line);
    $inserted_iteration=~s/^iter\-//;
    my @obj_scores=split(/ /,$obj_func_string);
    my $obj_score=$obj_scores[0];
    $instance_weight=$obj_scores[1];

    if(defined($prev_sent_id) && $sent_id!=$prev_sent_id) {

	&sample_pro($iteration,\@candidates_obj,\@candidates_features,\@candidates_iteration,\@{ $Xi[$prev_sent_id] },$instance_weight);
	&print_xi(\@{ $Xi[$prev_sent_id] },$prev_sent_id,$instance_weight);

	undef @candidates_obj;
	undef @candidates_features;
	undef @candidates_instance_weights;
	undef @candidates_iteration;
    }

    push(@candidates_obj,$obj_score);
    push(@candidates_features,$score_string);
    push(@candidates_instance_weights,$instance_weight);
    push(@candidates_iteration,$inserted_iteration);
    
    $prev_sent_id=$sent_id;
}
close(F);
&sample_pro($iteration,\@candidates_obj,\@candidates_features,\@candidates_iteration,\@{ $Xi[$prev_sent_id] },$instance_weight);
&print_xi(\@{ $Xi[$prev_sent_id] },$prev_sent_id,$instance_weight);
undef @candidates_obj;
undef @candidates_features;
undef @candidates_instance_weights;
undef @candidates_iteration;


if(defined($lattice_oracle_scored) && defined($model_nbest_scored)) {

    my $instance_weight_string='';
    if($use_instance_weight) {
	$instance_weight_string=" \$\$\$WEIGHT $instance_weight";
    }

    my @Xi_lattice;
    my @lattice_candidates_obj;
    my @lattice_candidates_features;
    my @lattice_candidates_instance_weights;
    open(F,"<$lattice_oracle_scored")||die("can't open file $lattice_oracle_scored: $!\n");
    while(defined(my $line=<F>)) {
	chomp($line);
	my($sent_id,$obj_func_string,$translation,$score_string)=split(/ \|\|\| /o,$line);
	my @obj_scores=split(/ /,$obj_func_string);
	my $obj_score=$obj_scores[0];
	my $instance_weight=$obj_scores[1];

	push(@{ $lattice_candidates_obj[$sent_id] },$obj_score);
	push(@{ $lattice_candidates_features[$sent_id] },$score_string);
	push(@{ $lattice_candidates_instance_weights[$sent_id] },$instance_weight);
    }
    close(F);

    my @nbest_candidates_obj;
    my @nbest_candidates_features;
    my @nbest_candidates_instance_weights;
    open(F,"<$model_nbest_scored")||die("can't open file $model_nbest_scored: $!\n");
    while(defined(my $line=<F>)) {
	chomp($line);
	my($sent_id,$obj_func_string,$translation,$score_string)=split(/ \|\|\| /o,$line);
	my @obj_scores=split(/ /,$obj_func_string);
	my $obj_score=$obj_scores[0];
	my $instance_weight=$obj_scores[1];

	push(@{ $nbest_candidates_obj[$sent_id] },$obj_score);
	push(@{ $nbest_candidates_features[$sent_id] },$score_string);
	push(@{ $nbest_candidates_instance_weights[$sent_id] },$instance_weight);
    }
    close(F);

    for(my $sent_id=0; $sent_id<@lattice_candidates_obj; $sent_id++) {
	
	for(my $i=0; $i<@{ $lattice_candidates_obj[$sent_id] } && $i<$oracle_max; $i++) {
	    for(my $j=0; $j<@{ $nbest_candidates_obj[$sent_id] } && $j<$oracle_max; $j++) {

		my $diff_string=&vec_diff($lattice_candidates_features[$sent_id][$i],$nbest_candidates_features[$sent_id][$j]);
		my $sign=&get_sign($lattice_candidates_obj[$sent_id][$i]-$nbest_candidates_obj[$sent_id][$j]);
		push(@{ $Xi_lattice[$sent_id] },"$sign$instance_weight_string $diff_string");

		my $diff_string_inv=&vec_diff($nbest_candidates_features[$sent_id][$j],$lattice_candidates_features[$sent_id][$i]);
		my $sign_inv=1-$sign;
		push(@{ $Xi_lattice[$sent_id] },"$sign_inv$instance_weight_string $diff_string_inv");
	    }
	}

	&print_xi(\@{ $Xi_lattice[$sent_id] },$sent_id,$instance_weight);
    }
}


if(defined($model_nbest_lai_scored) && defined($model_nbest_scored)) {
    my $instance_weight_string='';
    if($use_instance_weight) {
	$instance_weight_string=" \$\$\$WEIGHT $instance_weight";
    }

    my @Xi_lattice;
    my @lattice_candidates_obj;
    my @lattice_candidates_features;
    my @lattice_candidates_instance_weights;
    open(F,"<$model_nbest_lai_scored")||die("can't open file $model_nbest_lai_scored: $!\n");
    while(defined(my $line=<F>)) {
	chomp($line);
	my($sent_id,$obj_func_string,$translation,$score_string)=split(/ \|\|\| /o,$line);
	my @obj_scores=split(/ /,$obj_func_string);
	my $obj_score=$obj_scores[0];
	my $instance_weight=$obj_scores[1];

	push(@{ $lattice_candidates_obj[$sent_id] },$obj_score);
	push(@{ $lattice_candidates_features[$sent_id] },$score_string);
	push(@{ $lattice_candidates_instance_weights[$sent_id] },$instance_weight);
    }
    close(F);

    my @nbest_candidates_obj;
    my @nbest_candidates_features;
    my @nbest_candidates_instance_weights;
    open(F,"<$model_nbest_scored")||die("can't open file $model_nbest_scored: $!\n");
    while(defined(my $line=<F>)) {
	chomp($line);
	my($sent_id,$obj_func_string,$translation,$score_string)=split(/ \|\|\| /o,$line);
	my @obj_scores=split(/ /,$obj_func_string);
	my $obj_score=$obj_scores[0];
	my $instance_weight=$obj_scores[1];

	push(@{ $nbest_candidates_obj[$sent_id] },$obj_score);
	push(@{ $nbest_candidates_features[$sent_id] },$score_string);
	push(@{ $nbest_candidates_instance_weights[$sent_id] },$instance_weight);
    }
    close(F);

    for(my $sent_id=0; $sent_id<@lattice_candidates_obj; $sent_id++) {
	
	for(my $i=0; $i<@{ $lattice_candidates_obj[$sent_id] } && $i<$oracle_max; $i++) {
	    for(my $j=0; $j<@{ $nbest_candidates_obj[$sent_id] } && $j<$oracle_max; $j++) {

		my $diff_string=&vec_diff($lattice_candidates_features[$sent_id][$i],$nbest_candidates_features[$sent_id][$j]);
		my $sign=&get_sign($lattice_candidates_obj[$sent_id][$i]-$nbest_candidates_obj[$sent_id][$j]);
		push(@{ $Xi_lattice[$sent_id] },"$sign$instance_weight_string $diff_string");

		my $diff_string_inv=&vec_diff($nbest_candidates_features[$sent_id][$j],$lattice_candidates_features[$sent_id][$i]);
		my $sign_inv=1-$sign;
		push(@{ $Xi_lattice[$sent_id] },"$sign_inv$instance_weight_string $diff_string_inv");
	    }
	}

	&print_xi(\@{ $Xi_lattice[$sent_id] },$sent_id,$instance_weight);
    }
}


close(C);

print STDERR "$classifier_call\n";
system($classifier_call);


sub sample_pro {
    my($iteration,$candidates_obj,$candidates_features,$candidates_iteration,$Xi,$instance_weight)=@_;

    my $no_candidates=@$candidates_obj;
    
    if($no_candidates<2) {
	return;
    }

    my $instance_weight_string='';
    if($use_instance_weight) {
	$instance_weight_string=" \$\$\$WEIGHT $instance_weight";
    }


    my $gamma_sample_size=0;
    my %chosen_gamma_pairs;
    my $exhausted=0;
    my $no_tries=0;

    my %V;

    while($gamma_sample_size<$gamma_sample_max_size && !$exhausted) {

	if($no_tries>2000) {
	    $exhausted=1;
	}

	my $j=int(rand($no_candidates));
	my $j_prime=int(rand($no_candidates));

	my $pick_j=&prob_select($decay_factor**($iteration-$candidates_iteration->[$j]));
	my $pick_j_prime=&prob_select($decay_factor**($iteration-$candidates_iteration->[$j_prime]));

	if($pick_j==0||$pick_j_prime==0) {
	    $no_tries++;
	    next;
	}

	if($j==$j_prime 
	   || (exists($chosen_gamma_pairs{$j}) && exists($chosen_gamma_pairs{$j}{$j_prime}))
	   || (exists($chosen_gamma_pairs{$j_prime}) && exists($chosen_gamma_pairs{$j_prime}{$j}))) {
	    $no_tries++;
	    next;
	}
	$chosen_gamma_pairs{$j}{$j_prime}=1;
	$gamma_sample_size++;
	$no_tries=0;
    }

    my $avg_obj_diff=0;
    foreach my $j (keys %chosen_gamma_pairs) {
	foreach my $j_prime (keys %{ $chosen_gamma_pairs{$j} }) {
	    $avg_obj_diff+=abs($candidates_obj[$j]-$candidates_obj[$j_prime]);
	}
    }
    $avg_obj_diff/=$gamma_sample_size;

    
    foreach my $j (keys %chosen_gamma_pairs) {
	foreach my $j_prime (keys %{ $chosen_gamma_pairs{$j} }) {
	    my $insert=&alpha(abs($candidates_obj[$j]-$candidates_obj[$j_prime]),$alpha_strategy,$avg_obj_diff);
	    if($insert) {
		my $pair="$j $j_prime";
		$V{$pair}=1;
	    }
	}
    }

    my $c=0;
    foreach my $pair (sort {-1*($V{$a}<=>$V{$b})} (keys %V)) {
	$c++;
	last if($c>$xi_sample_size);

	my($j,$j_prime)=split(/ /,$pair);
	my $diff_string=&vec_diff($candidates_features->[$j],$candidates_features->[$j_prime]);
	my $sign=&get_sign($candidates_obj[$j]-$candidates_obj[$j_prime]);
	push(@$Xi,"$sign$instance_weight_string $diff_string");

	my $diff_string_inv=&vec_diff($candidates_features->[$j_prime],$candidates_features->[$j]);
#	my $sign_inv=&get_sign($candidates_obj[$j_prime]-$candidates_obj[$j]);
	my $sign_inv=1-$sign;
	push(@$Xi,"$sign_inv$instance_weight_string $diff_string_inv");	
    }

}

sub alpha {
    my($diff,$strategy,$avg_obj_diff)=@_;

    if($strategy eq 'step') {
	if($diff < $alpha_step_threshold) {
	    return 0;
	} else {
	    return 1;
	}
    } elsif($strategy eq 'sigmoid') {
	my $prob=1 / ( 1 + exp(-1* ($diff - $avg_obj_diff))  );
	my $random_num=rand();
	if($random_num<=$prob) {
	    return 1;
	} else {
	    return 0;
	}
    }
}

sub vec_diff {
    my($vec_string_j,$vec_string_j_prime)=@_;
    my @vec_j=split(/ /,$vec_string_j);
    my @vec_j_prime=split(/ /,$vec_string_j_prime);

    my %hash_j;
    my %hash_j_prime;
    my $largest_global_feature_index;
    for(my $i=0; $i<@vec_j; $i++) {
	if($vec_j[$i]!~/\:/) {
	    $largest_global_feature_index=$i;
	    my $id=$i+1;
	    $hash_j{$id}=$vec_j[$i];
	    $hash_j_prime{$id}=$vec_j_prime[$i];
	} else {
	    last;
	}
    }

    for(my $i=$largest_global_feature_index+1; $i<@vec_j; $i++) {
	if($vec_j[$i]=~/^([0-9]+)\:(.+)$/) {
	    my $lex_feature_id=$1;
	    my $lex_feature_value=$2;
	    my $id=$largest_global_feature_index+$lex_feature_id+2;
	    $hash_j{$id}=$lex_feature_value;
	} elsif($vec_j[$i]=~/^([^ ]+)\:(.+)$/) {
	    my $sparse_feature_name=$1;
	    my $sparse_feature_value=$2;
	    my $id=$sparse_decoding_name2id{$sparse_feature_name}
	    +$largest_global_feature_index+$largest_lex_feature_id+3;
	    $hash_j{$id}=$sparse_feature_value;
	}
    }

   for(my $i=$largest_global_feature_index+1; $i<@vec_j_prime; $i++) {
	if($vec_j_prime[$i]=~/^([0-9]+)\:(.+)$/) {
	    my $lex_feature_id=$1;
	    my $lex_feature_value=$2;
	    my $id=$largest_global_feature_index+$lex_feature_id+2;
	    $hash_j_prime{$id}=$lex_feature_value;
	    if(!exists($hash_j{$id})) {
		$hash_j{$id}=0;
	    }
	} elsif($vec_j_prime[$i]=~/^([^ ]+)\:(.+)$/) {
	    my $sparse_feature_name=$1;
	    my $sparse_feature_value=$2;
	    my $id=$sparse_decoding_name2id{$sparse_feature_name}
	    +$largest_global_feature_index+$largest_lex_feature_id+3;
	    $hash_j_prime{$id}=$sparse_feature_value;
	    if(!exists($hash_j{$id})) {
		$hash_j{$id}=0;
	    }
	}

    }
    foreach my $id (keys %hash_j) {
	if(!exists($hash_j_prime{$id})) {
	    $hash_j_prime{$id}=0;
	}
    }

    my @vec_diff;
    foreach my $id (sort {$a<=>$b} (keys %hash_j)) {
	my $vec_value="F$id " . ($hash_j{$id}-$hash_j_prime{$id});
 	push(@vec_diff,$vec_value);
   }

    return join(' ',@vec_diff);
}


sub vec_diff_OLD {
    my($vec_string_j,$vec_string_j_prime)=@_;
    my @vec_j=split(/ /,$vec_string_j);
    my @vec_j_prime=split(/ /,$vec_string_j_prime);

    my @vec_diff;
    for(my $i=0; $i<@vec_j; $i++) {
	my $id=$i+1;
	my $vec_value="F$id " . ($vec_j[$i]-$vec_j_prime[$i]);
	push(@vec_diff,$vec_value);
    }
    return join(' ',@vec_diff);
}


sub get_sign {
    return 1 if($_[0]>=0);
    return 0;
}

sub print_xi {
    my($xi,$sent_id,$instance_weight)=@_;

    for(my $i=0; $i<@$xi; $i++) {
#	print "$sent_id $xi->[$i]\n";
	print C "$xi->[$i]\n";
    }
}

sub prob_select {
    my($prob)=@_;

    my $random_num=rand();
    if($random_num<=$prob) {
	return 1;
    } else {
	return 0;
    }
}
