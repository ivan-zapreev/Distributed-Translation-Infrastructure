#!/usr/bin/perl -w

use strict;
use warnings;
use Getopt::Long "GetOptions";
use File::Basename;
use Cwd 'abs_path';
use PerlIO::gzip;

my $script_abs_name=abs_path($0);
my $scripts_location=dirname($script_abs_name);
my $pro_location=$scripts_location."/../PRO";
my $script_name=basename($0);
print STDERR "$script_name pid=$$\n";

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
my $trg_language;
my $trace;
my $use_node_scores=0;
my $use_srilm_c=0;
my $src_dep_tuples_file;
my $src_dep_info_file;

my $_HELP;
$_HELP = 1
    unless &GetOptions(
    "conf=s" => \$config_file,
    "src=s" => \$src_file,
    "src-con-parse=s" => \$src_con_parse_file,
    "ref=s" => \$ref_stem,
    "src-language=s" => \$src_language,
    "trg-language=s" => \$trg_language,
    "nbest-size=i" => \$nbest_size,
    "no-parallel=i" => \$no_parallel,
    "decoder=s" => \$decoder,
    "delete-files" => \$delete_files,
    "node-scoring|node-score|node-scores" => \$use_node_scores,
    "srilm_c|srilm-c" => \$use_srilm_c,
    "experiment-dir=s" => \$experiment_dir,
    "mert-script=s" => \$mert_script,
    "restart=s" => \$restart_iteration,
    "use-local-dir=s" => \$local_dir,
    "mert-server" => \$use_mert_server,
    "filter-phrase-table" => \$filter_phrase_table,
    "trace=i" => \$trace,
    "help|h" => \$_HELP,
    "src-dep-tuples=s" => \$src_dep_tuples_file,
    "src-dep-info=s" => \$src_dep_info_file
    );

if($_HELP) {
    print "Options:
  --conf=str : oister configuation file
  --src=str : source text file (plain, 1 sentence per line)
  --src-con-parse=str : source constituency parse file (1 tree per line)
  --ref=str : stem of the plain references
  --src-language=str : source language
  --nbest-size=int : size of the nbest-list (default=1000)
  --no-parallel=int : number of parallel processes
  --help : print this message.\n\n";
    exit(-1);
}

$nbest_size=100 if(!defined($nbest_size));
$delete_files=defined($delete_files) ? 1 : 0;
$no_parallel=0 if(!defined($no_parallel));
$mert_script='perl-mert' if(!defined($mert_script));
$filter_phrase_table=defined($filter_phrase_table) ? 1 : 0;
$restart_iteration=0 if(!defined($restart_iteration));
$local_dir||='';
$use_mert_server||=0;
$experiment_dir='.' if(!defined($experiment_dir));
$trg_language='english' if(!defined($trg_language));

my $mert;

if($mert_script=~/^PRO/) {
    $mert=$pro_location."/PRO-optimization-procedure.pl";
} else {
    print STDERR "Unsupported optimized option, we only support PRO!\n";
    exit(-1);
}

my $mert_work_dir="$experiment_dir/mert-work";
my $err_batch_dir="batches_errlogs";
unshift (@INC, $scripts_location);
require 'load_config_file.pl';

if(-e "$mert_work_dir" && !$restart_iteration) {
    print STDERR "rm -rf $mert_work_dir\n";
    system("rm -rf $mert_work_dir");
}
if(-e "$err_batch_dir") {
    print STDERR "rm -rf $err_batch_dir\n";
    system("rm -rf $err_batch_dir");
}

#normalize ref_stem argument
$ref_stem=~s/\.[0-9]+\.$/\./;

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
    $lambdas = &return_initial_lambda_string($config_file, $decoder);
}

my $decoder_wrapper="$scripts_location/decoder-rescorer-wrapper.pl";

my $decoder_parameters="--conf=$config_file --src=$src_file --src-language=$src_language --mert --decoder=$decoder --no-parallel=$no_parallel --mert-script=$mert_script";
$decoder_parameters.=' --delete-files' if($delete_files);
$decoder_parameters.=" --experiment-dir=$experiment_dir" if(defined($experiment_dir));
$decoder_parameters.=' --filter-phrase-table' if($filter_phrase_table);
$decoder_parameters.=" --use-local-dir=$local_dir" if($local_dir ne '');
$decoder_parameters.=' --mert-server' if($use_mert_server);
$decoder_parameters.=" --src-con-parse=$src_con_parse_file" if(defined($src_con_parse_file));
$decoder_parameters.=" --trace=$trace" if(defined($trace));
$decoder_parameters.=" --node-scoring" if($use_node_scores);
$decoder_parameters.=" --srilm_c" if($use_srilm_c);
$decoder_parameters.=" --trg-language=$trg_language" if(defined($trg_language));
$decoder_parameters.=" --src-dep-tuples=$src_dep_tuples_file" if(defined($src_dep_tuples_file));
$decoder_parameters.=" --src-dep-info=$src_dep_info_file" if(defined($src_dep_info_file));

my $restart_iter='';
$restart_iter=" $restart_iteration" if($restart_iteration);

my $no_parallel_argument;
if($mert_script=~/cmert/) {
    $no_parallel_argument=" $no_parallel";
} else {
    $no_parallel_argument="";
}

my $lexfeat_arg='';
my $pro_parallel_arg=' 1';
if($mert_script=~/^PRO/) {
    $lexfeat_arg="$lexfeature_file";
}
if($mert_script=~/^PRO\-([0-9]+)/) {
    $pro_parallel_arg="$1";
}

my $optimizer_call="$mert --work-dir=$mert_work_dir --src-file=$src_file --ref-stem=$ref_stem --nbest-size=$nbest_size --decoder-wrapper=$decoder_wrapper --decoder-parameters=\'$decoder_parameters\' --lambda-string=\'$lambdas\' --lexfeature-file=$lexfeat_arg --num-parallel=$pro_parallel_arg";
print STDERR "$optimizer_call\n";
system($optimizer_call);
