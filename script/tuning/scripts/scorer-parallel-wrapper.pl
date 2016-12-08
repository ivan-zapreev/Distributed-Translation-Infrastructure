#!/usr/bin/perl -w

use strict;
use warnings;
use File::Basename;

my($scorer_call,$nbest,$no_parallel)=@ARGV;

my $script_name = basename($0);
print STDERR "$script_name pid=$$\n";

if($no_parallel==0) {
    print STDERR "cat $nbest | $scorer_call\n";
    system("cat $nbest | $scorer_call");
} else {
    my $no_lines=`wc -l $nbest`;
    chomp($no_lines);
    $no_lines=~s/^([0-9]+).*$/$1/;
    my $batch_size=$no_lines/$no_parallel;
    $batch_size=~s/\.[0-9]+$//;
    $batch_size++;
    print STDERR "split -d -l $batch_size $nbest $nbest.batch.\n";
    system("split -d -l $batch_size $nbest $nbest.batch.");

    my %err_logs;
    for(my $batch_id=0; $batch_id<$no_parallel; $batch_id++) {
        my $batch_suffix=($batch_id<10) ? "0$batch_id" : $batch_id;
        $err_logs{"$nbest.batch.$batch_suffix.err"}=1;
        print STDERR "nohup sh -c \'cat $nbest.batch.$batch_suffix | $scorer_call 1> $nbest.batch.$batch_suffix.bleu 2> $nbest.batch.$batch_suffix.err\' \&\n";
        system("nohup sh -c \'cat $nbest.batch.$batch_suffix | $scorer_call 1> $nbest.batch.$batch_suffix.bleu 2> $nbest.batch.$batch_suffix.err\' \&");
    }
    # wait for scorers to finish

    my $finished=0;
    while(!$finished) {
        sleep(5);
        foreach my $errlog (keys %err_logs) {
            my $line=`tail -n 1 $errlog`;
            if($line=~/^finished/) {
            $finished=1;
            } else {
            $finished=0;
            last;
            }
        }
    }

    for(my $batch_id=0; $batch_id<$no_parallel; $batch_id++) {
        my $batch_suffix=($batch_id<10) ? "0$batch_id" : $batch_id;
        unlink("$nbest.batch.$batch_suffix.err");
        print STDERR "cat $nbest.batch.$batch_suffix.bleu\n";
        system("cat $nbest.batch.$batch_suffix.bleu");
        unlink("$nbest.batch.$batch_suffix");
        unlink("$nbest.batch.$batch_suffix.bleu");
    }
}

