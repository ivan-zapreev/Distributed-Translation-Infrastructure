#!/usr/bin/perl -w

use strict;
use warnings;

my($err_log)=@ARGV;

my $oister_decoder_pid;
my $oister_decoder_mert_wrapper_pid;
my $oister_mert_pid;
my $get_nbest_carmel_pid;
my $mert_pid;
my $cmert_pid;
my $oister_decoder_batch;
my $oister_decoder_batch_pid;
my %oister_decoder_batches;
my %batch_errlog_files;
my $oister_pid;
my $pro_optimizer_pid;
my $pro_optimization_procedure_pid;
my $scorer_wrapper_pid;

open(E,"<$err_log");
while(defined(my $line=<E>)) {
    if($line=~/^oister-decoder.pl pid=([0-9]+)\n/) {
        $oister_decoder_pid=$1;
    } elsif($line=~/^oister.pl pid=([0-9]+)\n/) {
        $oister_pid=$1;
    } elsif($line=~/Forking off batch ([0-9]+) \(sentences [0-9]+\-[0-9]+\) pid=([0-9]+)\n/) {
        my $batch=$1;
        my $batch_pid=$2;
        $oister_decoder_batches{$batch}=$batch_pid;
    } elsif($line=~/^oister-decoder-mert-wrapper.pl pid=([0-9]+)\n/) {
        $oister_decoder_mert_wrapper_pid=$1;
    } elsif($line=~/^oister-mert.pl pid=([0-9]+)\n/) {
        $oister_mert_pid=$1;
    } elsif($line=~/^mert.perl pid=([0-9]+)\n/) {
        $mert_pid=$1;
    } elsif($line=~/^mert-driver pid=([0-9]+)\n/) {
        $cmert_pid=$1;
    } elsif($line=~/^scorer-parallel-wrapper pid=([0-9]+)\n/) {
        $scorer_wrapper_pid=$1;
    } elsif($line=~/^get-nbest-carmel.pl pid=([0-9]+)\n/) {
        $get_nbest_carmel_pid=$1;
    } elsif($line=~/^PRO-optimizer.pl pid=([0-9]+)\n/) {
        $pro_optimizer_pid=$1;
    } elsif($line=~/^PRO-optimization-procedure.pl pid=([0-9]+)\n/) {
        $pro_optimization_procedure_pid=$1;
    }
}
close(E);

print "\nAre you sure you want to kill the following processes?\n";
print "$oister_mert_pid (oister-mert.pl)\n" if(defined($oister_mert_pid));
print "$mert_pid (mert.perl)\n" if(defined($mert_pid));
print "$cmert_pid (mert-driver)\n" if(defined($cmert_pid));
print "$oister_decoder_mert_wrapper_pid (oister-decoder_wrapper.pl)\n" if(defined($oister_decoder_mert_wrapper_pid));
print "$get_nbest_carmel_pid (get-nbest-carmel.pl)\n" if(defined($get_nbest_carmel_pid));
print "$oister_decoder_pid (oister-decoder.pl)\n" if(defined($oister_decoder_pid));
print "$oister_pid (oister.pl)\n" if(defined($oister_pid));
print "$scorer_wrapper_pid (scorer_wrapper.pl)\n" if(defined($scorer_wrapper_pid));
print "$pro_optimizer_pid (PRO-optimizer.pl)\n" if(defined($pro_optimizer_pid));
print "$pro_optimization_procedure_pid (PRO-optimization-procedure.pl)\n" if(defined($pro_optimization_procedure_pid));
foreach $oister_decoder_batch (keys %oister_decoder_batches) {
    print "$oister_decoder_batches{$oister_decoder_batch} (oister-decoder.pl batch=$oister_decoder_batch)\n";
}

print "\n";
print "Answer (yes/no)? ";
my $answer=<STDIN>;
if($answer!~/^yes/) {
    print STDERR "Processes spared.\n";
    exit(-1);
} else {
    print STDERR "Kill! Kill! Kill! ...\n";
}
 
if(defined($oister_mert_pid)) {
    print STDERR "kill -9 $oister_mert_pid (oister-mert.pl)\n";
    system("kill -9 $oister_mert_pid");
}
if(defined($mert_pid)) {
    print STDERR "kill -9 $mert_pid (mert.perl)\n";
    system("kill -9 $mert_pid");
}
if(defined($cmert_pid)) {
    print STDERR "kill -9 $cmert_pid (mert-driver)\n";
    system("kill -9 $cmert_pid");
}
if(defined($oister_decoder_mert_wrapper_pid)) {
    print STDERR "kill -9 $oister_decoder_mert_wrapper_pid (oister-decoder_mert_wrapper.pl)\n";
    system("kill -9 $oister_decoder_mert_wrapper_pid");
}
if(defined($get_nbest_carmel_pid)) {
    print STDERR "kill -9 $get_nbest_carmel_pid (get-nbest-carmel.pl)\n";
    system("kill -9 $get_nbest_carmel_pid");
} 
if(defined($scorer_wrapper_pid)) {
    print STDERR "kill -9 $scorer_wrapper_pid (scorer-wrapper.pl)\n";
    system("kill -9 $scorer_wrapper_pid");
}
if(defined($oister_decoder_pid)) {
    print STDERR "kill -9 $oister_decoder_pid (oister-decoder.pl)\n";
    system("kill -9 $oister_decoder_pid");
}
if(defined($pro_optimizer_pid)) {
    print STDERR "kill -9 $pro_optimizer_pid (PRO-optimizer.pl)\n";
    system("kill -9 $pro_optimizer_pid");
}
if(defined($pro_optimization_procedure_pid)) {
    print STDERR "kill -9 $pro_optimization_procedure_pid (PRO-optimization-procedure.pl)\n";
    system("kill -9 $pro_optimization_procedure_pid");
}
foreach $oister_decoder_batch (keys %oister_decoder_batches) {
    print STDERR "kill -9 $oister_decoder_batches{$oister_decoder_batch} (oister-decoder.pl batch=$oister_decoder_batch)\n";
    system("kill -9 $oister_decoder_batches{$oister_decoder_batch}");
}
if(defined($oister_pid)) {
    print STDERR "kill -9 $oister_pid (oister.pl)\n";
    system("kill -9 $oister_pid");
}
