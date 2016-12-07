#!/usr/bin/perl -w

use strict;
use warnings;

my($nbest_size,$lambda_string,$prefix,$parameters,$src_file,$nbest_file)=@ARGV;

print STDERR "oister-decoder-mert-wrapper.pl pid=$$\n";
print STDERR "parameters=$parameters\n";
print STDERR "Lambdas=$lambda_string\n";

my($decoder)=$parameters=~/\-\-?decoder[\= ]+([^ ]+)/;
my($config_file)=$parameters=~/\-\-?conf[\= ]+([^ ]+)/;
my($mert_script)=$parameters=~/\-\-?mert-script[\= ]+([^ ]+)/;
my($no_parallel)=$parameters=~/\-\-?no-parallel[\= ]+([^ ]+)/;


my $OISTERHOME=$ENV{'OISTERHOME'};
my $carmel="$OISTERHOME/tuning/scripts/get-nbest-carmel-parallel.pl";
my $mteval_home="$OISTERHOME/resources/mteval";


#my $oister="$oister_home/bin/oister.pl";

$mert_script='cmert-closest' if(!defined($mert_script));
$no_parallel=0 if(!defined($no_parallel));

my $delete_tmp_src_file=0;

if(0 && !defined($src_file)) {
    $src_file="$$.tmp.src";
    open(S,">$src_file");
    while(defined(my $line=<STDIN>)) {
	print S $line;
    }
    close(S);
    $delete_tmp_src_file=1;
}

my $mert_mode=$parameters=~/--mert/ ? 1 : 0;
my $delete_files=$parameters=~/--delete-files/ ? 1 : 0;

$parameters=~s/--decoder=[^ ]+/ /;
$parameters=~s/--delete-files/ /;
$parameters=~s/--mert-script=[^ ]+/ /;
$parameters.=" --src=$src_file" if($parameters!~/--src=[^ ]+/);
$parameters.=" --trg=$prefix.trans";
$parameters.=" --nbest-size=$nbest_size";

$parameters=~s/ +/ /g;
$parameters=~s/^[\s\t]*(.*?)[\s\t]*$/$1/;

print STDERR "mert-mode=$mert_mode\n";
print STDERR "delete-file=$delete_files\n";


my $main_dir="$OISTERHOME/src";
unshift (@INC, "$main_dir");
require 'load_config_file.pl';

$lambda_string=~s/^[\s\t]*(.*?)[\s\t]*$/$1/;
my @lambdas=split(/ +/,$lambda_string);


if($mert_script=~/cmert/) {
    my $max_lambda=0;
    for(my $i=0; $i<@lambdas; $i++) {
	$max_lambda=abs($lambdas[$i]) if(abs($lambdas[$i])>=$max_lambda);
    }
    for(my $i=0; $i<@lambdas; $i++) {
	$lambdas[$i]/=$max_lambda;
    }
}

my %data;
my %parameters;
my %features;
my %feature_order;

my @config_buffer=&buffer_config_file($config_file);

my $current_feature=0;
for(my $i=0; $i<@config_buffer && $current_feature<@lambdas; $i++) {
    if($config_buffer[$i]=~/^feature:.*([\s\t])opt=([^\[]+)\[/) {
	$config_buffer[$i]=~s/([\s\t])opt=[^\[]+\[/$1opt=$lambdas[$current_feature]\[/;
	$current_feature++;
    }
}

my $copy_config_file=$config_file;
my $iteration=0;
if($config_file=~/\.([0-9]+)$/) {
    $iteration=$1;
    $iteration++;
    $copy_config_file=~s/\.([0-9]+)$//;
}

system("cp $config_file $copy_config_file.$iteration");

my $config_file_string=join("",@config_buffer);
open(C,">$config_file");
print C $config_file_string;
close(C);

#call decoder:
print STDERR "$decoder $parameters\n";
system("$decoder $parameters");

#call carmel:
my %carmel_batches;
for(my $batch=0; $batch<$no_parallel; $batch++) {
    print STDERR "nohup sh -c \"$carmel $prefix.trans.lattices.batch.$batch $nbest_size >& $prefix.trans.lattices.batch.$batch.err\" \&\n";
    system("nohup sh -c \"$carmel $prefix.trans.lattices.batch.$batch $nbest_size >& $prefix.trans.lattices.batch.$batch.err\" \&");
    unlink("$prefix.trans.lattices.batch.$batch.err") if($delete_files);
}

while((keys %carmel_batches)>0) {
    foreach my $finished_file (keys %carmel_batches) {
	if(-e "$finished_file") {
	    delete($carmel_batches{$finished_file});
	    unlink("$finished_file");
	}
    }
    sleep(5);
}

my $finished=0;
while(!$finished) {
    sleep(5);
    for(my $batch=0; $batch<$no_parallel; $batch++) {
	if(-e "$prefix.trans.lattices.batch.$batch.done") {
	    $finished=1;
	} else {
	    $finished=0;
	    last;
	}
    }
}
for(my $batch=0; $batch<$no_parallel; $batch++) {
    unlink("$prefix.trans.lattices.batch.$batch.err") if($delete_files);
    unlink("$prefix.trans.lattices.batch.$batch.done");
}

open(ALL,">$prefix.trans.lattices");
for(my $batch=0; $batch<$no_parallel; $batch++) {
    open(INDLATTICE,"<$prefix.trans.lattices.batch.$batch");
    while(defined(my $line=<INDLATTICE>)) {
	print ALL $line;
    }
    close(INDLATTICE);
    unlink("$prefix.trans.lattices.batch.$batch");
}
close(ALL);

open(ALL,">$prefix.trans.lattices.nbest");
for(my $batch=0; $batch<$no_parallel; $batch++) {
    open(NBEST,"<$prefix.trans.lattices.batch.$batch.nbest");
    while(defined(my $line=<NBEST>)) {
	print ALL $line;
    }
    close(NBEST);
    unlink("$prefix.trans.lattices.batch.$batch.nbest");
}
close(ALL);

my $rescore_parameters=$parameters;
$rescore_parameters.=" --rescore --nbest-file=$prefix.trans.lattices.nbest";
#$rescore_parameters.=" --rescore --nbest-file=$prefix.trans.lattices.nbest --decoder=$decoder";
# rescored file will be=$prefix.trans.lattices.nbest.rescored
#print STDERR "$decoder $rescore_parameters\n";
#system("$decoder $rescore_parameters");

print STDERR "Start rescoring...\n";
print STDERR "$decoder $rescore_parameters\n";
system("$decoder $rescore_parameters");
print STDERR " done.\n";

my $process_pipeline="$mteval_home/Programs/postprocess/rescore-remove-nil-lines.pl | $OISTERHOME/scripts/nbest-extract.pl | $mteval_home/Programs/postprocess/postprocess1_no-untok | $OISTERHOME/scripts/nbest-replace.pl";

if(defined($nbest_file)) {
    print STDERR "cat $prefix.trans.lattices.nbest.rescored | $process_pipeline $prefix.trans.lattices.nbest.rescored > $nbest_file\n";
#    system("cat $prefix.trans.lattices.nbest.rescored | $mteval_home/Programs/postprocess/rescore-remove-nil-lines.pl | $mteval_home/Programs/nbest-extract.pl | $mteval_home/Programs/postprocess/postprocess1 | $mteval_home/Programs/postprocess/normalize-v11a.pl | $oister_home/nbest-replace.pl $prefix.trans.lattices.nbest.rescored > $nbest_file");
    system("cat $prefix.trans.lattices.nbest.rescored | process_pipeline $prefix.trans.lattices.nbest.rescored > $nbest_file");
} else {
    # print nbest-output to STDOUT
    if($no_parallel>0) {
	for(my $batch_id=0; $batch_id<$no_parallel; $batch_id++) {
	    if(-e "$prefix.trans.lattices.nbest.rescored.batch.$batch_id.done") {
		unlink("$prefix.trans.lattices.nbest.rescored.batch.$batch_id.done");
	    }
	    if(-e "$prefix.trans.lattices.nbest.rescored.batch.$batch_id.norm") {
		unlink("$prefix.trans.lattices.nbest.rescored.batch.$batch_id.norm");
	    }
	    if(!(-e "$prefix.trans.lattices.nbest.rescored.batch.$batch_id")) {
		print STDERR "Error: file \"$prefix.trans.lattices.nbest.rescored.batch.$batch_id\" does not exist.\n";
		next;
	    } else {
		print STDERR "cat $prefix.trans.lattices.nbest.rescored.batch.$batch_id | $process_pipeline $prefix.trans.lattices.nbest.rescored.batch.$batch_id 1> $prefix.trans.lattices.nbest.rescored.batch.$batch_id.norm 2> $prefix.trans.lattices.nbest.rescored.batch.$batch_id.norm.err\' \&\n";
		system("nohup sh -c \'cat $prefix.trans.lattices.nbest.rescored.batch.$batch_id | $process_pipeline $prefix.trans.lattices.nbest.rescored.batch.$batch_id 1> $prefix.trans.lattices.nbest.rescored.batch.$batch_id.norm 2> $prefix.trans.lattices.nbest.rescored.batch.$batch_id.norm.err\' \&");
	    }
	}
	#wait for all batches to finish
	my $finished=0;
	while(!$finished) {
	    sleep(5);
	    for(my $batch_id=0; $batch_id<$no_parallel; $batch_id++) {
		if(-e "$prefix.trans.lattices.nbest.rescored.batch.$batch_id.done") {
		    $finished=1;
		} else {
		    $finished=0;
		    last;
		}
	    }
	}
	for(my $batch_id=0; $batch_id<$no_parallel; $batch_id++) {
	    unlink("$prefix.trans.lattices.nbest.rescored.batch.$batch_id");
	    unlink("$prefix.trans.lattices.nbest.rescored.batch.$batch_id.done");
	    system("cat $prefix.trans.lattices.nbest.rescored.batch.$batch_id.norm");
	    unlink("$prefix.trans.lattices.nbest.rescored.batch.$batch_id.norm");
	    unlink("$prefix.trans.lattices.nbest.rescored.batch.$batch_id.norm.err");	    
	}

    } else {
	print STDERR "cat $prefix.trans.lattices.nbest.rescored | $process_pipeline $prefix.trans.lattices.nbest.rescored\n";
	system("cat $prefix.trans.lattices.nbest.rescored | $process_pipeline $prefix.trans.lattices.nbest.rescored");
    }
}

#delete files to save disk-space:
if($delete_files) {
#    print STDERR "rm $prefix.trans.lattices\n";
#    unlink("$prefix.trans.lattices");

    print STDERR "rm $prefix.trans.lattices.nbest\n";
    unlink("$prefix.trans.lattices.nbest");
    print STDERR "rm $prefix.trans.lattices.nbest.rescored\n";
    unlink("$prefix.trans.lattices.nbest.rescored");

}

if($delete_tmp_src_file) {
    print STDERR "rm $$.tmp.src\n";
    unlink("$$.tmp.src");
}
