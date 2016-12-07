#!/usr/bin/perl -w

use strict;
use warnings;

my($work_dir,$src_file,$ref_stem,$nbest_size,$decoder_wrapper,$decoder_parameters,$lambda_string,$lexfeature_file,$no_pro_parallel,$decay_factor,$restart_iter)=@ARGV;

if(@ARGV==0) {
    print STDERR "\nusage: PRO-optimization-procedure.pl ...\n\n";
    exit(-1);
}


print STDERR "PRO-optimization-procedure.pl pid=$$\n";

my $OISTERHOME=$ENV{'OISTERHOME'};
my $max_n=4;
my $Psi=0.1;
my $min_no_added=1;
my $max_no_iterations=30;
my $bleu_method='chiang';
my $lattice_bleu=1;
my $burn_in=0;
$decay_factor||=1;
$decay_factor=~s/^\./0\./;

my $use_loss_augmented_inference=1;

my $mteval_home="$OISTERHOME/resources/mteval";
my $nbest_pool="$work_dir/pool.nbest.scored";

if(!(-e "$work_dir")) {
    my $call="mkdir $work_dir";
    print STDERR "$call\n";
    system($call);
}

my $src_file_lai="$work_dir/src_file.lai.txt";
system("cat $src_file $src_file > $src_file_lai");

my $src_con_parse_file_lai;
if($decoder_parameters=~/--src-con-parse=([^ ]+)/) {
    my $src_con_parse_file=$1;
    $src_con_parse_file_lai="$work_dir/src_con_parse_file.lai.txt";
    system("cat $src_con_parse_file $src_con_parse_file > $src_con_parse_file_lai");
}

my $config_file;
if($decoder_parameters=~/--conf=([^ ]+)/) {
    $config_file=$1;    
}

my $sparse_decoding_feature_file;
if(defined($config_file)) {
    open(F,"<$config_file")||die("can't open $config_file: $!\n");
    while(defined(my $line=<F>)) {
        if($line=~/^data:sparse\_decoding\_feature\_weights=([^ ]+)[\s\t]*\n/) {
            $sparse_decoding_feature_file=$1;
        }
    }
    close(F);
}

my $largest_lex_feature_id=-1;
if($lexfeature_file ne 'nil') {
    open(F,"<$lexfeature_file")||die("can't open $lexfeature_file: $!\n");
    while(defined(my $line=<F>)) {
	chomp($line);
	my($id,$name,$scores_string)=split(/ \|\|\| /o,$line);
	if($id>$largest_lex_feature_id) {
	    $largest_lex_feature_id=$id;
	}
    }
    close(F);
}

system("touch $nbest_pool");

my $delete_files=0;
if($decoder_parameters=~/\-delete\-files/) {
    $delete_files=1;
}

my $no_parallel=1;
if($decoder_parameters=~/\-no\-parallel[= ]([0-9]+)/) {
    $no_parallel=$1;
}

my @ref_file_list;
for(my $i=0; $i<20; $i++) {
    if(-e "$ref_stem$i" && !(-z "$ref_stem$i")) {
	push(@ref_file_list,"$ref_stem$i");
    }
}
my $ref_files_argument_string=join(' ',@ref_file_list);

my $stopping_criterion_true=0;

my @lambdas_general=split(/\;/,$lambda_string);
for(my $i=0; $i<@lambdas_general; $i++) {
    $lambdas_general[$i]=~s/\,.*$//;
}
$lambda_string=join(' ',@lambdas_general);

my $no_lambdas_general=@lambdas_general;

my $iteration=1;
while(!$stopping_criterion_true) {

    my @nbest_sparse_files;


#    my $Psi_annealed=&max($Psi,(1/($iteration+3)));
    my $Psi_annealed=$Psi;

    print STDERR "Psi_annealed=$Psi_annealed\n";

    # step 1+2: translate with decoder + rescore nbest lists with decoder
    my $decoder_translate_and_rescore_call="$decoder_wrapper $nbest_size \"$lambda_string\" $work_dir/run$iteration \"$decoder_parameters\" < $src_file > $work_dir/run$iteration.nbest";
    print STDERR "$decoder_translate_and_rescore_call\n";
    system($decoder_translate_and_rescore_call);

    # step 3: score nbest lists with objective function (e.g., BLEU)

    my $nbest_objective_function_call;
    if($bleu_method eq 'chiang') {
#	my $o_score_call="cat $work_dir/run$iteration.nbest | $HOME/scripts/compute-BLEU-components-window.pl $src_file $ref_files_argument_string > $work_dir/run$iteration.nbest.O_scores";
	my $o_score_call="cat $work_dir/run$iteration.trans | $OISTERHOME/resources/mteval/Programs/postprocess/postprocess1_no-untok | $OISTERHOME/tuning/scripts/compute-BLEU-components-window.pl $src_file $ref_files_argument_string > $work_dir/run$iteration.nbest.O_scores";
	print STDERR "$o_score_call\n";
	system("$o_score_call");


	$nbest_objective_function_call="$OISTERHOME/tuning/scripts/scorer-parallel-wrapper-window.pl \"$OISTERHOME/tuning/scripts/score-nbest-BLEU-window.pl $src_file $work_dir/run$iteration.nbest.O_scores $ref_files_argument_string\" $work_dir/run$iteration.nbest $src_file \"$ref_files_argument_string\" $no_parallel > $work_dir/run$iteration.nbest.scored";
	
    } elsif($bleu_method eq 'liang') {
	$nbest_objective_function_call="$OISTERHOME/tuning/scripts/scorer-parallel-wrapper.pl \"$OISTERHOME/tuning/scripts/score-nbest-BLEU.pl $src_file $ref_files_argument_string\" $work_dir/run$iteration.nbest $no_parallel > $work_dir/run$iteration.nbest.scored";
    }

    print STDERR "$nbest_objective_function_call\n";
    system($nbest_objective_function_call);
    push(@nbest_sparse_files,"$work_dir/run$iteration.nbest.scored");

    my $corpus_bleu=&compute_corpus_bleu_1best("$work_dir/run$iteration.nbest.scored",$max_n);
    print STDERR "Best point: $lambda_string ||| $corpus_bleu\n";


    if($lattice_bleu && $bleu_method eq 'chiang') {
	my $mu=0.05;
	my $lattice_bleu_call="$OISTERHOME/tuning/scripts/bleu-score-lattice.pl $work_dir/run$iteration.trans.lattices $work_dir/run$iteration.nbest.O_scores $mu $src_file $ref_files_argument_string > $work_dir/run$iteration.trans.lattices_bleu";
	print STDERR "$lattice_bleu_call\n";
	system($lattice_bleu_call);

	my $carmel_call="$OISTERHOME/tuning/scripts/get-nbest-carmel-parallel.pl $work_dir/run$iteration.trans.lattices_bleu 10";
	print STDERR "$carmel_call\n";
	system($carmel_call);

	if($use_loss_augmented_inference) {
	    my $mu=1000;
	    my $lattice_bleu_call_lai="$OISTERHOME/tuning/scripts/bleu-score-lattice.pl $work_dir/run$iteration.trans.lattices $work_dir/run$iteration.nbest.O_scores $mu $src_file $ref_files_argument_string > $work_dir/run$iteration.trans.lattices_bleu_lai";
	    print STDERR "$lattice_bleu_call_lai\n";
	    system($lattice_bleu_call_lai);

	    my $carmel_call="$OISTERHOME/tuning/scripts/get-nbest-carmel-parallel.pl $work_dir/run$iteration.trans.lattices_bleu_lai 10";
	    print STDERR "$carmel_call\n";
	    system($carmel_call);

	    my $concatenate_nbestlist_call="$OISTERHOME/tuning/scripts/concatenate-nbest-lists.pl $work_dir/run$iteration.trans.lattices_bleu.nbest $work_dir/run$iteration.trans.lattices_bleu_lai.nbest 1> $work_dir/run$iteration.trans.lattices_bleu_max-lai.nbest";
	    print STDERR "$concatenate_nbestlist_call\n";
	    system($concatenate_nbestlist_call);
	    my $mv_call="mv $work_dir/run$iteration.trans.lattices_bleu_max-lai.nbest $work_dir/run$iteration.trans.lattices_bleu.nbest";
	    print STDERR "$mv_call\n";
	    system($mv_call);
	}

	
	#rescore bleu-lattice nbest list
	my $rescore_parameters=$decoder_parameters;
	my($decoder)=$rescore_parameters=~/\-\-?decoder[\= ]+([^ ]+)/;
	
	$rescore_parameters=~s/--decoder=[^ ]+/ /;
	$rescore_parameters=~s/--delete-files/ /;
	$rescore_parameters=~s/--mert-script=[^ ]+/ /;
	$rescore_parameters.=" --nbest-size=10";
	$rescore_parameters.=" --rescore --nbest-file=$work_dir/run$iteration.trans.lattices_bleu.nbest";

	if($use_loss_augmented_inference) {
	    $rescore_parameters=~s/--src\s*=\s*[^ ]+/ /;
	    $rescore_parameters.=" --src=$src_file_lai";
	    if(defined($src_con_parse_file_lai)) {
		$rescore_parameters.=" --src-con-parse=$src_con_parse_file_lai";
	    }
	}

	$rescore_parameters=~s/ +/ /g;
	$rescore_parameters=~s/^[\s\t]*(.*?)[\s\t]*$/$1/;

        # rescored file will be=$work_dir/run$iteration.trans.lattices_bleu.nbest.rescored
	my $rescore_lattice_bleu_call="$decoder $rescore_parameters >& $work_dir/oracle_rescore.err";
	print STDERR "$rescore_lattice_bleu_call\n";
	system($rescore_lattice_bleu_call);

	open(F,">$work_dir/run$iteration.trans.lattices_bleu.nbest.rescored");
        for(my $batch_id=0; $batch_id<$no_parallel; $batch_id++) {
	    open(B,"<$work_dir/run$iteration.trans.lattices_bleu.nbest.rescored.batch.$batch_id");
	    while(defined(my $line=<B>)) {
		print F $line;
	    }
	    close(B);
	    unlink("$work_dir/run$iteration.trans.lattices_bleu.nbest.rescored.batch.$batch_id");
	    unlink("$work_dir/run$iteration.trans.lattices_bleu.nbest.rescored.batch.$batch_id.done");
	}
	close(F);
	unlink("$work_dir/oracle_rescore.err") if($delete_files);

	my $process_pipeline="$mteval_home/Programs/postprocess/rescore-remove-nil-lines.pl | $OISTERHOME/scripts/nbest-extract.pl | $mteval_home/Programs/postprocess/postprocess1_no-untok | $OISTERHOME/scripts/nbest-replace.pl";

	my $rescore_clean_up_call="cat $work_dir/run$iteration.trans.lattices_bleu.nbest.rescored | $process_pipeline $work_dir/run$iteration.trans.lattices_bleu.nbest.rescored > $work_dir/run$iteration.trans.lattices_bleu.nbest.rescored.clean";
	print STDERR "$rescore_clean_up_call\n";
	system($rescore_clean_up_call);
	my $mv_call="mv $work_dir/run$iteration.trans.lattices_bleu.nbest.rescored.clean $work_dir/run$iteration.trans.lattices_bleu.nbest.rescored";
	print STDERR "$mv_call\n";
	system($mv_call);

	if($use_loss_augmented_inference) {
	    my $split_lai_nbest_call="$OISTERHOME/tuning/scripts/split-lai-nbest.pl $work_dir/run$iteration.trans.lattices_bleu.nbest.rescored $src_file $work_dir/run$iteration.trans.lattices_bleu.nbest.rescored_max $work_dir/run$iteration.trans.lattices_bleu_lai.nbest.rescored";
	    print STDERR "$split_lai_nbest_call\n";
	    system($split_lai_nbest_call);
	    my $mv_call="mv $work_dir/run$iteration.trans.lattices_bleu.nbest.rescored_max $work_dir/run$iteration.trans.lattices_bleu.nbest.rescored";
	    print STDERR "$mv_call\n";
	    system($mv_call);

	    my $nbest_objective_function_call="$OISTERHOME/tuning/scripts/scorer-parallel-wrapper-window.pl \"$OISTERHOME/tuning/scripts/score-nbest-BLEU-window.pl $src_file $work_dir/run$iteration.nbest.O_scores $ref_files_argument_string\" $work_dir/run$iteration.trans.lattices_bleu_lai.nbest.rescored $src_file \"$ref_files_argument_string\" $no_parallel > $work_dir/run$iteration.trans.lattices_bleu_lai.nbest.scored";
	    print STDERR "$nbest_objective_function_call\n";
	    system($nbest_objective_function_call);
	}

	my $nbest_objective_function_call="$OISTERHOME/tuning/scripts/scorer-parallel-wrapper-window.pl \"$OISTERHOME/tuning/scripts/score-nbest-BLEU-window.pl $src_file $work_dir/run$iteration.nbest.O_scores $ref_files_argument_string\" $work_dir/run$iteration.trans.lattices_bleu.nbest.rescored $src_file \"$ref_files_argument_string\" $no_parallel > $work_dir/run$iteration.trans.lattices_bleu.nbest.scored";
	print STDERR "$nbest_objective_function_call\n";
	system($nbest_objective_function_call);


	my $compute_corpus_bleu_from_oracle_call="cat $work_dir/run$iteration.trans.lattices_bleu.nbest.scored | $OISTERHOME/tuning/scripts/compute-corpus-bleu-with-highest-bleu-from-scored-nbest.pl";
	print STDERR "$compute_corpus_bleu_from_oracle_call\n";
	print STDERR "ORACLE LATTICE ";
	system($compute_corpus_bleu_from_oracle_call);
    }


    # step 4: combine nbest list with nbest pool

    my $no_added=0;
    if($bleu_method eq 'chiang') {
	$no_added=&combine_nbest_list_with_pool("$work_dir/run$iteration.nbest.scored",$nbest_pool,$iteration);
	my $nbest_objective_function_call="$OISTERHOME/tuning/scripts/scorer-parallel-wrapper-window.pl \"$OISTERHOME/tuning/scripts/score-nbest-BLEU-window-replace-score.pl $src_file $work_dir/run$iteration.nbest.O_scores $ref_files_argument_string\" $nbest_pool $src_file \"$ref_files_argument_string\" $no_parallel > $nbest_pool.tmp";
	print STDERR "$nbest_objective_function_call\n";
	system("$nbest_objective_function_call");
	my $move_call="mv $nbest_pool.tmp $nbest_pool";
	print STDERR "$move_call\n";
	system($move_call);

    } elsif($bleu_method eq 'liang') {
	$no_added=&combine_nbest_list_with_pool("$work_dir/run$iteration.nbest.scored",$nbest_pool,$iteration);
    }

    if($no_added<$min_no_added) {
	$stopping_criterion_true=1;
    }


    if($lattice_bleu && $bleu_method eq 'chiang') {
	push(@nbest_sparse_files,"$work_dir/run$iteration.trans.lattices_bleu.nbest.scored");
	if($use_loss_augmented_inference) {
	    push(@nbest_sparse_files,"$work_dir/run$iteration.trans.lattices_bleu_lai.nbest.scored");
	}
    }

    if(0 && defined($sparse_decoding_feature_file)) {
	my $nbest_sparse_files_string=join(' ',@nbest_sparse_files);
	my $gather_sparse_feature_call="$OISTERHOME/tuning/scripts/gather-sparse-decoding-features.pl $sparse_decoding_feature_file $nbest_sparse_files_string 1> $sparse_decoding_feature_file.next";
	print STDERR "$gather_sparse_feature_call\n";
	system($gather_sparse_feature_call);
	my $mv_call="mv $sparse_decoding_feature_file.next $sparse_decoding_feature_file";
	print STDERR "$mv_call\n";
	system($mv_call);
    }
    

    # for DEBUGGING ONLY:
#    $stopping_criterion_true=1;

    # step 5: recompute feature weights
#    my $pro_call="$HOME/scripts/PRO-optimizer.pl $work_dir $work_dir/run$iteration.nbest.scored > $work_dir/run$iteration.weights";

#    my $pro_call="$HOME/scripts/PRO-optimizer.pl $work_dir $nbest_pool > $work_dir/run$iteration.weights";


    if($lattice_bleu && $bleu_method eq 'chiang') {
	if($use_loss_augmented_inference) {
#	    my $pro_call="$OISTERHOME/tuning/PRO/PRO-optimizer-parallel-wrapper.pl $work_dir $nbest_pool $iteration $no_pro_parallel $work_dir/run$iteration.trans.lattices_bleu.nbest.scored $work_dir/run$iteration.nbest.scored $work_dir/run$iteration.trans.lattices_bleu_lai.nbest.scored > $work_dir/run$iteration.weights";
	    my $pro_call="$OISTERHOME/tuning/PRO/PRO-optimizer-parallel-wrapper.pl --work-dir=$work_dir --nbest-pool=$nbest_pool --iteration=$iteration --decay=$decay_factor --num-parallel=$no_pro_parallel --lattice-oracle-scored=$work_dir/run$iteration.trans.lattices_bleu.nbest.scored --model-nbest-scored=$work_dir/run$iteration.nbest.scored --lattice-lai-scored=$work_dir/run$iteration.trans.lattices_bleu_lai.nbest.scored --conf=$config_file > $work_dir/run$iteration.weights";
	    print STDERR "$pro_call\n";
	    system($pro_call);
	} else {
	    my $pro_call="$OISTERHOME/tuning/PRO/PRO-optimizer-parallel-wrapper.pl $work_dir $nbest_pool $iteration $no_pro_parallel $work_dir/run$iteration.trans.lattices_bleu.nbest.scored $work_dir/run$iteration.nbest.scored > $work_dir/run$iteration.weights";
	    print STDERR "$pro_call\n";
	    system($pro_call);
	}	
    } else {
	my $pro_call="$OISTERHOME/tuning/PRO/PRO-optimizer-parallel-wrapper.pl $work_dir $nbest_pool $iteration $no_pro_parallel > $work_dir/run$iteration.weights";
	print STDERR "$pro_call\n";
	system($pro_call);
    }

    my @feature_weights;
    print STDERR "Reading in weights file \'$work_dir/run$iteration.weights\'\n";
    open(F,"<$work_dir/run$iteration.weights")||die("can't open file $work_dir/run$iteration.weights: $!\n");
    while(defined(my $line=<F>)) {
	if($line=~/^\s*[F]([0-9]+)[\s\t]+([0-9\-e\+\.]+)\s*\n/) {
	    my $feature_id=$1;
	    my $feature_weight=$2;
	    $feature_weights[$feature_id]=$feature_weight;
	    print STDERR "feature_weights[$feature_id]=$feature_weights[$feature_id]\n";
	}
    }
    close(F);

    my @lambdas_general_prev=@lambdas_general;

    undef @lambdas_general;
    for(my $i=1; $i<=$no_lambdas_general; $i++) {
	if(!defined($feature_weights[$i])) {
	    $feature_weights[$i]=0;
	}

	my $interpolated_feature_weight=$Psi_annealed*$feature_weights[$i] + ( (1-$Psi_annealed)*$lambdas_general_prev[$i-1] );
#	my $interpolated_feature_weight=$Psi*$feature_weights[$i] + ( (1-$Psi)*$lambdas_general_prev[$i-1] );
	push(@lambdas_general,$interpolated_feature_weight);
#	push(@lambdas_general,$feature_weights[$i]);
	print STDERR "push i=$i $feature_weights[$i]\n";
    }
    $lambda_string=join(' ',@lambdas_general);

    if($lexfeature_file ne 'nil') {
	my @lexfeature_lines;
	open(F,"<$lexfeature_file")||die("can't open $lexfeature_file: $!\n");
	while(defined(my $line=<F>)) {
	    if($line=~/^([0-9]+) \|\|\| ([^\|]+) \|\|\| ([0-9\-\+e\. ]+)\s*\n/) {
		my $id=$1;
		my $name=$2;
		my $weights_string=$3;
		my @weights=split(/ +/,$weights_string);
		my $feature_id=$id+$no_lambdas_general+1;

		if(!defined($feature_weights[$feature_id])) {
		    $feature_weights[$feature_id]=0;
		}

		my $interpolated_feature_weight=$Psi_annealed*$feature_weights[$feature_id] + ( (1-$Psi_annealed)*$weights[-1] );
#		my $interpolated_feature_weight=$Psi*$feature_weights[$feature_id] + ( (1-$Psi)*$weights[-1] );


		push(@weights,$interpolated_feature_weight);
#		push(@weights,$feature_weights[$feature_id]);
		$weights_string=join(' ',@weights);
		my $new_line="$id ||| $name ||| $weights_string\n";
		push(@lexfeature_lines,$new_line);
	    }
	}
	close(F);
	open(F,">$lexfeature_file")||die("can't open $lexfeature_file: $!\n");
	for(my $i=0; $i<@lexfeature_lines; $i++) {
	    print F $lexfeature_lines[$i];
	}
	close(F);  
    }

    if(defined($sparse_decoding_feature_file)) {
	#TODO
#	my %printed_feature_ids;
	my @sparse_feature_lines;
#	my $largest_feature_id;
	open(F,"<$sparse_decoding_feature_file")||die("can't open $sparse_decoding_feature_file: $!\n");
	while(defined(my $line=<F>)) {
	    if($line=~/^s([0-9]+) \|\|\| ([^\|]+) \|\|\| ([0-9\-\+e\. ]+)\s*\n/) {
		my $id=$1;
		my $name=$2;
		my $weights_string=$3;
		my @weights=split(/ +/,$weights_string);
		my $feature_id=$id+$no_lambdas_general+$largest_lex_feature_id+1;

		if(!defined($feature_weights[$feature_id])) {
		    $feature_weights[$feature_id]=0;
		}

		my $interpolated_feature_weight=$Psi_annealed*$feature_weights[$feature_id] + ( (1-$Psi_annealed)*$weights[-1] );
#		my $interpolated_feature_weight=$Psi*$feature_weights[$feature_id] + ( (1-$Psi)*$weights[-1] );

		push(@weights,$interpolated_feature_weight);
#		push(@weights,$feature_weights[$feature_id]);
		$weights_string=join(' ',@weights);
		my $new_line="s$id ||| $name ||| $weights_string\n";
		push(@sparse_feature_lines,$new_line);
#		$printed_feature_ids{$feature_id}=1;
#		if(!defined($largest_feature_id) || $feature_id>$largest_feature_id) {
#		    $largest_feature_id=$feature_id;
#		}
	    }
	}
	close(F);

	#add features not present in previous iterations:
#	for(my $i=$largest_feature_id+1; $i<@feature_weights; $i++) {
#	}


	open(F,">$sparse_decoding_feature_file")||die("can't open $sparse_decoding_feature_file: $!\n");
	for(my $i=0; $i<@sparse_feature_lines; $i++) {
	    print F $sparse_feature_lines[$i];
	}
	close(F);  	
    }

    if($iteration>$max_no_iterations) {
	$stopping_criterion_true=1;
    }

    if($delete_files) {
	my $call="rm $nbest_pool.classifier_data\*";
	print STDERR "$call\n";
	system($call);

	$call="rm $nbest_pool.classifier_err_log\*";
	print STDERR "$call\n";
	system($call);

	$call="rm $work_dir/run$iteration.nbest.scored";
	print STDERR "$call\n";
	system($call);

	$call="rm $work_dir/run$iteration.nbest";
	print STDERR "$call\n";
	system($call);
	$call="rm $work_dir/pro_err.\*";
	print STDERR "$call\n";
	system($call);

#	$call="rm $work_dir/pro_finished.\*";
#	print STDERR "$call\n";
#	system($call);

	$call="rm $work_dir/run$iteration.weights\*";
	print STDERR "$call\n";
	system($call);

	$call="rm $work_dir/run$iteration.trans.lattices\*";
	print STDERR "$call\n";
	system($call);


#	$call="rm $work_dir/";
#	print STDERR "$call\n";
#	system($call);
    }

    my $rm_finished_call="rm $work_dir/pro_finished.\*";
    print STDERR "$rm_finished_call\n";
    system($rm_finished_call);


    if($iteration<=$burn_in) {
	print STDERR "Still in burn-in phase (burn-in=$burn_in). Removing pooled results from iteration=$iteration\n";
	unlink($nbest_pool);
	system("touch $nbest_pool");
    } else {
	print STDERR "Not in burn-in phase anymore. Pooled results from iteration=$iteration\n";
    }

    $iteration++;

}

sub compute_corpus_bleu_1best {
    my($nbest_file,$max_n)=@_;

    my @correct;
    my @total;
    my $corpus_ref_length=0;

    my $prev_sent_id;
    open(F,"<$nbest_file") || die("can't open $nbest_file: $!\n");
    while(defined(my $line=<F>)) {
	chomp($line);
	if($line=~/^([0-9]+) \|\|\| [^ ]+ [^ ]+ ([^\|]+) \|\|\|/) {
	    my $sent_id=$1;
	    my $bleu_component_scores_string=$2;

	    if(!defined($prev_sent_id)||$prev_sent_id ne $sent_id) {
		my @bleu_components=split(/ /,$bleu_component_scores_string);
		my $n=0;
		for(my $i=0; $i<@bleu_components-2; $i+=2) {
		    $n++;
		    $correct[$n]+=$bleu_components[$i];
		    $total[$n]+=$bleu_components[$i+1];
		}
		$corpus_ref_length+=$bleu_components[-1];
	    }	
	    $prev_sent_id=$sent_id;
	}
    }
    close(F);

    my $corpus_tst_length=$total[1];

    my $BP=&compute_brevaty_penalty($corpus_tst_length,$corpus_ref_length);

    my $prec=0;
    for(my $i=1; $i<=$max_n; $i++) {
	print STDERR "correct[$i]=$correct[$i] total[$i]=$total[$i]\n";
	$prec+=1/$max_n * log($correct[$i]/$total[$i]);
    }
    my $bleu=$BP*exp($prec);
    return $bleu;

}


sub compute_brevaty_penalty {
    my($c,$r)=@_;
    if($c>$r) {
        return 1;
    } else {
        return exp(1-($r/$c));
    }
}


sub combine_nbest_list_with_pool {
    my($nbest_list,$nbest_pool,$iteration)=@_;
    my $no_added=0;
    $iteration||=0;

    print STDERR "nbest_file=$nbest_list\n";
    print STDERR "pool_file=$nbest_pool\n";
    print STDERR "Combining nbest list with nbest pool ... ";

    if(0 && -z "$nbest_pool") {

	my $no_lines=`wc -l $nbest_list`;
	chomp($no_lines);
	$no_lines=~s/^([0-9]+).*$/$1/;

	my $move_call="cp $nbest_list $nbest_pool";
	print STDERR "$move_call\n";
	system($move_call);
	print STDERR "done.\n";
	return $no_lines;
    }


    my @nbest;
    my @translations;
    my @inserted_iterations;
    my @src_length_ratios;
    open(N,"<$nbest_list")||die("can't open file $nbest_list: $!\n");
    while(defined(my $line_n=<N>)) {
	if($line_n=~/^([0-9]+) \|\|\| ([^ ]+) ([^ ]+) ([^\|]+) \|\|\| (.+?) \|\|\| (.+)\n/) {
	    my $sent_id_n=$1;
	    my $obj_score=$2;
	    my $length_ratio=$3;
	    my $score_components=$4;
	    my $trans=$5;
	    my $feature_scores=$6;
	    my $key="$score_components ||| $feature_scores";
	    $src_length_ratios[$sent_id_n]=$length_ratio;
	    if(!exists($nbest[$sent_id_n]) || !exists($nbest[$sent_id_n]{$key})
	       || $obj_score>$nbest[$sent_id_n]{$key}) {	    
		$nbest[$sent_id_n]{$key}=$obj_score;
		$translations[$sent_id_n]{$key}=$trans;
		$inserted_iterations[$sent_id_n]{$key}=$iteration;
	    }
	}
    }
    close(N);

    my $prev_sent_id_p;
    open(P,"<$nbest_pool")||die("can't open file $nbest_pool: $!\n");
    open(T,">$nbest_pool.tmp")||die("can't open file $nbest_pool.tmp: $!\n");
    while(defined(my $line_p=<P>)) {
#	if($line_p=~/^([0-9]+) \|\|\| ([^ ]+) ([^ ]+) ([^\|]+) \|\|\| (.+?) \|\|\| (.+)\n/) {
	if($line_p=~/^([0-9]+) \|\|\| iter\-([0-9]+) \|\|\| ([^ ]+) ([^ ]+) ([^\|]+) \|\|\| (.+?) \|\|\| (.+)\n/) {
	    my $sent_id_p=$1;
	    my $inserted_iteration=$2;
	    my $obj_score=$3;
	    my $length_ratio=$4;
	    my $score_components=$5;
	    my $trans=$6;
	    my $feature_scores=$7;
	    my $key="$score_components ||| $feature_scores";
	    $src_length_ratios[$sent_id_p]=$length_ratio;

	    # current nbest list generates translation already in pool: update insertion iteration to current iteration
	    if(exists($nbest[$sent_id_p]) && exists($nbest[$sent_id_p]{$key})) {
		$inserted_iteration=$iteration;
	    }

	    if(!exists($nbest[$sent_id_p]) || !exists($nbest[$sent_id_p]{$key})
	       || $obj_score>$nbest[$sent_id_p]{$key}) {	    
		$nbest[$sent_id_p]{$key}=$obj_score;
		$translations[$sent_id_p]{$key}=$trans;
		$inserted_iterations[$sent_id_n]{$key}=$inserted_iteration;
		$no_added++;
	    }

	    if(defined($prev_sent_id_p) && $prev_sent_id_p!=$sent_id_p) {
		foreach my $key (keys %{ $nbest[$prev_sent_id_p] }) {
		    my($score_components,$feature_scores)=split(/ \|\|\| /o,$key);
		    print T "$prev_sent_id_p ||| iter\-$inserted_iterations[$prev_sent_id_p]{$key} ||| $nbest[$prev_sent_id_p]{$key} $src_length_ratios[$prev_sent_id_p] $score_components ||| $translations[$prev_sent_id_p]{$key} ||| $feature_scores\n";
		}
		foreach my $key (keys %{ $nbest[$prev_sent_id_p] }) {
		    delete($nbest[$prev_sent_id_p]{$key});
		    delete($translations[$prev_sent_id_p]{$key});
		    delete($inserted_iterations[$prev_sent_id_p]{$key});
		}
	    }

	    $prev_sent_id_p=$sent_id_p;
	}
    }
    if(defined($prev_sent_id_p)) {
	foreach my $key (keys %{ $nbest[$prev_sent_id_p] }) {
	    my($score_components,$feature_scores)=split(/ \|\|\| /o,$key);
	    print T "$prev_sent_id_p ||| iter\-$inserted_iterations[$prev_sent_id_p]{$key} ||| $nbest[$prev_sent_id_p]{$key} $src_length_ratios[$prev_sent_id_p] $score_components ||| $translations[$prev_sent_id_p]{$key} ||| $feature_scores\n";
	    $no_added++;
	}
	foreach my $key (keys %{ $nbest[$prev_sent_id_p] }) {
	    delete($nbest[$prev_sent_id_p]{$key});
	    delete($translations[$prev_sent_id_p]{$key});
	    delete($inserted_iterations[$prev_sent_id_p]{$key});
	}
    } else {
	$prev_sent_id_p=0;
	while($prev_sent_id_p<@nbest) {
	    foreach my $key (keys %{ $nbest[$prev_sent_id_p] }) {
		my($score_components,$feature_scores)=split(/ \|\|\| /o,$key);
		print T "$prev_sent_id_p ||| iter\-$inserted_iterations[$prev_sent_id_p]{$key} ||| $nbest[$prev_sent_id_p]{$key} $src_length_ratios[$prev_sent_id_p] $score_components ||| $translations[$prev_sent_id_p]{$key} ||| $feature_scores\n";
		$no_added++;
	    }
	    foreach my $key (keys %{ $nbest[$prev_sent_id_p] }) {
		delete($nbest[$prev_sent_id_p]{$key});
		delete($translations[$prev_sent_id_p]{$key});
		delete($inserted_iterations[$prev_sent_id_p]{$key});
	    }
	    $prev_sent_id_p++;	    
	}
    }

    close(P);
    close(T);

    my $move_call="mv $nbest_pool.tmp $nbest_pool";
    print STDERR "$move_call\n";
    system($move_call);
    print STDERR "done.\n";
    return $no_added;
}


sub max {
    return $_[0] if($_[0]>$_[1]);
    return $_[1];
}
