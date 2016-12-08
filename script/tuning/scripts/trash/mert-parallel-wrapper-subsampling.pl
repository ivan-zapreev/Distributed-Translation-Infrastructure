#!/usr/bin/perl -w

use strict;
use warnings;

my($mert_options,$initial_txt,$cumul_nbest_bleu,$no_parallel)=@ARGV;
$no_parallel=1 if(!defined($no_parallel));

#my $mert='/homes/christof/resources/mteval/Programs/cmert-1.01-parallel-new/mert-regularized';
my $mert='/home/christof/resources/mteval/Programs/cmert-1.01-parallel-new/mert';
my($work_dir)=$initial_txt=~/^(.*)\/?initial.txt$/;
$work_dir='.' if(!defined($work_dir)||$work_dir eq '');
my $no_samples=8;


exit (-1) if(!defined($initial_txt)||!defined($cumul_nbest_bleu));

my $subsampler='/home/christof/scripts/nbest-bleu-subsampler.pl';
my $mert_weight_combiner='/home/christof/scripts/combine-mert-points.pl';
my $combine_strategy='addition';


# create subsamples of (cumulated) nbest lists
my $call="$subsampler $cumul_nbest_bleu 0.632 $no_samples";
print STDERR "$call\n";
system($call);

my $nbest_sample_prefix=$cumul_nbest_bleu . '.sample_';
$nbest_sample_prefix=~s/^.*\/([^\/]+)$/$1/;

for(my $i=0; $i<$no_samples; $i++) {
    my $mert_call="nohup sh -c 'nice -10 $mert \"$mert_options\" $initial_txt $work_dir/$nbest_sample_prefix$i 1> $work_dir/final.$nbest_sample_prefix$i 2> $work_dir/err.$nbest_sample_prefix$i.log' \&";
    print STDERR "$mert_call\n";
    system($mert_call);
}

my $no_points=`wc -l $initial_txt`;
$no_points=~s/^[\s\t]*([0-9]+)[\s\t]+.*\n/$1/;
my $total_no_points=$no_points*$no_samples;

my $finished=0;
while(!$finished) {
    sleep(10);
    my $total_length=0;
    for(my $i=0; $i<$no_samples; $i++) {
        next if(!-e "$work_dir/final.$nbest_sample_prefix$i");
        my $length=`wc -l $work_dir/final.$nbest_sample_prefix$i`;
        $length=~s/^[\s\t]*([0-9]+)[\s\t]+.*\n/$1/;
        $total_length+=$length;
    }
    $finished=1 if($total_length==$total_no_points);
}


my $best_points_file="$work_dir/final.$nbest_sample_prefix" . 'best';
unlink("$best_points_file");
system("touch $best_points_file");
for(my $i=0; $i<$no_samples; $i++) {
    my $call="cat $work_dir/final.$nbest_sample_prefix$i | egrep -v 'nan|inf' | sort -nr -t\\| -k 4,4 | head -1 >> $best_points_file";
    print STDERR "$call\n";
    system($call);
}

for(my $i=0; $i<$no_samples; $i++) {
    unlink("$work_dir/$nbest_sample_prefix$i");
    unlink("$work_dir/err.$nbest_sample_prefix$i.log");
    unlink("$work_dir/final.$nbest_sample_prefix$i");
}

$call="cat $best_points_file | $mert_weight_combiner $combine_strategy | /home/christof/scripts/normalize-mert-weights.pl";
print STDERR "$call\n";
system($call);





