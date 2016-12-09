#!/usr/bin/perl -w

use strict;
use warnings;
use File::Basename;

my($work_dir,$nbest_file_scored,$sample_id)=@ARGV;

if(!defined($sample_id)) {
    $sample_id='';
}

my $script_name = basename($0);
print STDERR "$script_name pid=$$\n";

my $OISTERHOME=$ENV{'OISTERHOME'};

my $gamma_sample_max_size=5000;
my $xi_sample_size=50;
my $alpha_strategy='sigmoid';
my $alpha_step_threshold=0.05;
my $use_instance_weight=0;
my $max_iterations=100;

my $classifier_data_file="$nbest_file_scored.classifier_data$sample_id";
my $classifier_weights_file="$nbest_file_scored.classifier_weights$sample_id";
my $classifier_err_log="$nbest_file_scored.classifier_err_log$sample_id";

my $classifier_call="$OISTERHOME/resources/software/MegaM/current/bin/megam_i686.opt -nobias -fvals -maxi $max_iterations binary $classifier_data_file 2> $classifier_err_log";


my $prev_sent_id;
my @candidates_obj;
my @candidates_features;
my @candidates_instance_weights;
my $instance_weight;
my @Xi;
open(C,">$classifier_data_file") || die("can't open file $classifier_data_file: $!\n");
open(F,"<$nbest_file_scored")||die("can't open file $nbest_file_scored: $!\n");
while(defined(my $line=<F>)) {
    chomp($line);
    my($sent_id,$obj_func_string,$translation,$score_string)=split(/ \|\|\| /o,$line);
    my @obj_scores=split(/ /,$obj_func_string);
    my $obj_score=$obj_scores[0];
    $instance_weight=$obj_scores[1];

    if(defined($prev_sent_id) && $sent_id!=$prev_sent_id) {
        &sample_pro(\@candidates_obj,\@candidates_features,\@{ $Xi[$prev_sent_id] },$instance_weight);
        &print_xi(\@{ $Xi[$prev_sent_id] },$prev_sent_id,$instance_weight);

        undef @candidates_obj;
        undef @candidates_features;
        undef @candidates_instance_weights;
    }

    push(@candidates_obj,$obj_score);
    push(@candidates_features,$score_string);
    push(@candidates_instance_weights,$instance_weight);
    
    $prev_sent_id=$sent_id;
}
close(F);
&sample_pro(\@candidates_obj,\@candidates_features,\@{ $Xi[$prev_sent_id] },$instance_weight);
&print_xi(\@{ $Xi[$prev_sent_id] },$prev_sent_id,$instance_weight);
close(C);

print STDERR "$classifier_call\n";
system($classifier_call);

sub sample_pro {
    my ($candidates_obj,$candidates_features,$Xi,$instance_weight)=@_;

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

        if($no_tries>1000) {
            $exhausted=1;
        }

        my $j=int(rand($no_candidates));
        my $j_prime=int(rand($no_candidates));

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
        my $sign_inv=&get_sign($candidates_obj[$j_prime]-$candidates_obj[$j]);
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
        #print "$sent_id $xi->[$i]\n";
        print C "$xi->[$i]\n";
    }
}
