#!/usr/bin/perl -w

use strict;
use warnings;

use Getopt::Long "GetOptions";

print STDERR "oister-mert-fork-no_v11-tok.pl pid=$$\n";

my $config_file;
my $src_file;
my $src_con_parse_file;
my $ref_stem;
my $nbest_size;
my $decoder;
my $delete_files;
my $no_parallel;
my $mert_script;
my $filter_phrase_table;
my $restart_iteration;
my $local_dir;
my $use_mert_server;
my $experiment_dir;
my $src_language;
my $trace;

GetOptions(
    "conf=s" => \$config_file,
    "src=s" => \$src_file,
    "src-con-parse=s" => \$src_con_parse_file,
    "ref=s" => \$ref_stem,
    "src-language=s" => \$src_language,
    "nbest-size=i" => \$nbest_size,
    "no-parallel=i" => \$no_parallel,
    "decoder=s" => \$decoder,
    "delete-files" => \$delete_files,
    "experiment-dir=s" => \$experiment_dir,
    "mert-script=s" => \$mert_script,
    "restart=s" => \$restart_iteration,
    "use-local-dir=s" => \$local_dir,
    "mert-server" => \$use_mert_server,
    "filter-phrase-table" => \$filter_phrase_table,
    "trace=i" => \$trace
    
    );

$nbest_size=100 if(!defined($nbest_size));
$delete_files=defined($delete_files) ? 1 : 0;
$no_parallel=0 if(!defined($no_parallel));
$mert_script='perl-mert' if(!defined($mert_script));
$filter_phrase_table=defined($filter_phrase_table) ? 1 : 0;
$restart_iteration=0 if(!defined($restart_iteration));
$local_dir||='';
$use_mert_server||=0;
$experiment_dir='.' if(!defined($experiment_dir));

my $mert;
my $OISTERHOME=$ENV{'OISTERHOME'};
#if($mert_script eq 'perl-mert') {
#    $mert="$HOME/scripts/DC-mert/src/mert.perl";
#} elsi

if($mert_script=~/^PRO/) {
    $mert="$OISTERHOME/tuning/PRO/PRO-optimization-procedure.DECAY.pl";
} elsif($mert_script eq 'cmert') {
    $mert="$OISTERHOME/resources/software/cmert/cmert-1.01-parallel-new-no_v11-tok/mert-driver";
} elsif($mert_script eq 'cmert-closest') {
    $mert="$OISTERHOME/resources/software/cmert/cmert-1.01-parallel-new-no_v11-tok-closest/mert-driver";
}

#} elsif($mert_script eq 'cmert-closest-weighted') {
#    $mert="$HOME/resources/mteval/Programs/cmert-1.01-parallel-new-no_v11-tok-closest_weighted/mert-driver-weighted";
#}

#} elsif($mert_script eq 'cmert-subsampling') {
#    $mert="$HOME/resources/mteval/Programs/cmert-1.01-parallel-new-subsampling/mert-driver";
##} elsif($mert_script eq 'cmert-closest') {
##    $mert='/home/christof/resources/mteval/Programs/cmert-1.01-parallel-new-closest/mert-driver';
#} elsif($mert_script eq 'cmert-rm-init') {
#    $mert="$HOME/resources/mteval/Programs/cmert-1.01-parallel-new-no_v11-tok-rm-init/mert-driver";
#}

my $mert_work_dir="$experiment_dir/mert-work";
my $err_batch_dir="batches_errlogs";
my $main_dir="$OISTERHOME/src";
unshift (@INC, "$main_dir");
require 'load_config_file.pl';

if(-e "$mert_work_dir" && !$restart_iteration) {
    print STDERR "rm -rf $mert_work_dir\n";
    system("rm -rf $mert_work_dir");
}
if(-e "$err_batch_dir") {
    print STDERR "rm -rf $err_batch_dir\n";
    system("rm -rf $err_batch_dir");
}



my $fail=0;
for(my $i=0; $i<100; $i++) {
    if(-e "$ref_stem$i") {
	open(F,"<$ref_stem$i");
	my $line_counter=0;
	while(defined(my $line=<F>)) {
	    $line_counter++;
	    if($line=~/^[\t\s]*\n/) {
		print STDERR "Error: line=$line_counter of \"$ref_stem$i\" is empty.\n";
		$fail=1;
	    }
	}
    }
}
if($fail) {
    print STDERR "Loading the references failed.\n";
    exit(-1);
}

my $lambdas;

my $lexfeature_file='nil';
if($restart_iteration) {
    print STDERR "Restarting from iteration $restart_iteration\n";
    $lambdas=&return_last_lambda_string($config_file);  
} else {
    system("cp $config_file $config_file.init");
    my @config_buffer;
    open(F,"<$config_file");
    while(defined(my $conf_line=<F>)) {
	push(@config_buffer,$conf_line);
	if($conf_line=~/^data:lexfeat_weights=([^ ]+)\s*\n/) {
	   $lexfeature_file=$1;
       } 
    }
    close(F);
    for(my $i=0; $i<@config_buffer; $i++) {
	if($config_buffer[$i]=~/^(.*[\s\t]+init=)([^ ]+)([\s\t]+opt=)[^ ]+([\s\t]*)\n/) {
	    my $left_context=$1;
	    my $init_val=$2;
	    my $middle_context=$3;
	    my $right_context=$4;
	    my $line="$left_context$init_val$middle_context$init_val$right_context\n";
	    $config_buffer[$i]=$line;
	}
    }
    open(F,">$config_file");
    print F join('',@config_buffer);
    close(F);

    $lambdas=&return_initial_lambda_string($config_file);
}

#die;

my $decoder_wrapper="$OISTERHOME/tuning/scripts/oister-decoder-mert-wrapper-fork-no_v11-tok.pl";
my $decoder_parameters="--conf=$config_file --src=$src_file --src-language=$src_language --mert --decoder=$decoder --no-parallel=$no_parallel --mert-script=$mert_script";
$decoder_parameters.=' --delete-files' if($delete_files);
$decoder_parameters.=" --experiment-dir=$experiment_dir" if(defined($experiment_dir));
$decoder_parameters.=' --filter-phrase-table' if($filter_phrase_table);
$decoder_parameters.=" --use-local-dir=$local_dir" if($local_dir ne '');
$decoder_parameters.=' --mert-server' if($use_mert_server);
$decoder_parameters.=" --src-con-parse=$src_con_parse_file" if(defined($src_con_parse_file));
$decoder_parameters.=" --trace=$trace" if(defined($trace));


my $restart_iter='';
$restart_iter=" $restart_iteration" if($restart_iteration);

#print STDERR "$mert $mert_work_dir $src_file $ref_stem $nbest_size $decoder_wrapper \"$decoder_parameters\" \"$lambdas\"$restart_iter\n";
my $no_parallel_argument;
if($mert_script=~/cmert/) {
    $no_parallel_argument=" $no_parallel";
} else {
    $no_parallel_argument="";
}

my $lexfeat_arg='';
my $pro_parallel_arg=' 1';
my $pro_decay_arg=' 1';
if($mert_script=~/^PRO/) {
    $lexfeat_arg=" $lexfeature_file";
}
if($mert_script=~/^PRO\-([0-9]+)\_decay\-([0-9\.]+)/) {
    $pro_parallel_arg=" $1";
    $pro_decay_arg=" $2";
} elsif($mert_script=~/^PRO\-([0-9]+)/) {
    $pro_parallel_arg=" $1";
}

#die;
print STDERR "$mert $mert_work_dir $src_file $ref_stem $nbest_size $decoder_wrapper \"$decoder_parameters\" \"$lambdas\"$lexfeat_arg$no_parallel_argument$pro_parallel_arg$pro_decay_arg$restart_iter\n";
system("$mert $mert_work_dir $src_file $ref_stem $nbest_size $decoder_wrapper \"$decoder_parameters\" \"$lambdas\"$lexfeat_arg$no_parallel_argument$pro_parallel_arg$pro_decay_arg$restart_iter");



