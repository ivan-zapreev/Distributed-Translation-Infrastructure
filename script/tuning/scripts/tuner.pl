#!/usr/bin/perl -w

use strict;
use warnings;
use Getopt::Long "GetOptions";

print STDERR "oister-mert-fork-no_v11-tok.pl pid=$$\n";

BEGIN {
    if(!defined($ENV{'OISTERHOME'})
       || $ENV{'OISTERHOME'} eq '') {
        print STDERR "environment variable OISTERHOME must be set:\n";
        print STDERR "export OISTERHOME=/path/to/oister/distribution\n";
        exit(-1);
    }
}

BEGIN {
    my $release_info=`cat /etc/*-release`;
    $release_info=~s/\n/ /g;
    my $os_release;
    if($release_info=~/CentOS release 5\./) {
        $os_release='CentOS_5';
    } elsif($release_info=~/CentOS release 6\./) {
        $os_release='CentOS_6';
    }
    if($os_release eq 'CentOS_6') {
        unshift @INC, $ENV{"OISTERHOME"}."/lib/perl_modules/lib64/perl5"
    } else {
        unshift @INC, $ENV{"OISTERHOME"}."/resources/bin/lib64/perl5/site_perl/5.8.8/x86_64-linux-thread-multi"
    }
}

use PerlIO::gzip;
my $OISTERHOME=$ENV{'OISTERHOME'};

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
my $external_path;
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
    "external-path|ext-path=s" => \$external_path,
    "trace=i" => \$trace,
    "help|h" => \$_HELP,
    "src-dep-tuples=s" => \$src_dep_tuples_file,
    "src-dep-info=s" => \$src_dep_info_file
    );

if(!defined($external_path)) {
    print STDERR "  --external-path=str must be set\n";
    $_HELP=1;
}

if($_HELP) {
    print "Options:
  --conf=str : oister configuation file
  --src=str : source text file (plain, 1 sentence per line)
  --src-con-parse=str : source constituency parse file (1 tree per line)
  --ref=str : stem of the plain references
  --external-path=str : path to 3rd party software directory
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
    $mert="$OISTERHOME/tuning/PRO/PRO-optimization-procedure.pl";
} else {
    print STDERR "Unsupported optimized option, we only support PRO!\n";
    exit(-1);
}

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
    if($decoder =~ /.*oister\-decoder\.pl$/)
    {
        $lambdas=&return_initial_lambda_string($config_file);
    }
    else
    {
        $lambdas = &return_lambda_string_from_ivansConfig($config_file, $decoder);
    }
}

my $decoder_wrapper="$OISTERHOME/tuning/scripts/decoder-rescorer-wrapper.pl";

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

my $optimizer_call="$mert --work-dir=$mert_work_dir --src-file=$src_file --ref-stem=$ref_stem --nbest-size=$nbest_size --decoder-wrapper=$decoder_wrapper --external-path=$external_path --decoder-parameters=\'$decoder_parameters\' --lambda-string=\'$lambdas\' --lexfeature-file=$lexfeat_arg --num-parallel=$pro_parallel_arg";
print STDERR "$optimizer_call\n";
system($optimizer_call);

sub return_lambda_string_from_ivansConfig
{

    my ($config_file, $decoder) = @_;
    my $decoder_params = "--conf=$config_file --create-features-mapping";
    system("$decoder $decoder_params"); #This calls the decoder server to only create the feature mapping file
    my $features_mapping_file = "$config_file.feature_id2name";
    open(FEATURES_MAPPING_FILE, "<$features_mapping_file");
    my %features2id = ();
    my %features_specifiers = ();
    while(my $line = <FEATURES_MAPPING_FILE>)
    {
        chomp($line);
        my($index, $feature_name) = split(/\t/, $line);

        if($feature_name =~ /^([^\[]+)\[\d+\]$/)
        {
            $features_specifiers{$1} = 1; #This feature specifier would have multiple values
        }
        else
        {
            $features_specifiers{$feature_name} = 0;
        }
        $features2id{$feature_name} = $index;
    }
    close(FEATURES_MAPPING_FILE);
    my $featuresNumber = scalar(keys(%features2id));
    my @lambdas = ();
    $#lambdas = $featuresNumber -1;
    open(CONF_FILE, "<$config_file");
    while(my $line = <CONF_FILE>)
    {
        $line =~ s/^\s+|\s+$//g;
        if($line !~ /^\#.*$/)
        {
            foreach my $feature_sepcifier (keys(%features_specifiers))
            {
                 if($line =~ m/^$feature_sepcifier=(.*)/)
                 {
                    my $feature_str = $1;
                    #print STDERR "read feature string for $feature_sepcifier:$feature_str\n";
                    if($features_specifiers{$feature_sepcifier} == 1)
                    {
                        my @feature_values = split(/\|/, $feature_str);
                        #print STDERR "mapping number:", $features2id{"$feature_sepcifier\[0\]"}, "\n";
                        splice(@lambdas, $features2id{"$feature_sepcifier\[0\]"}, scalar(@feature_values), @feature_values);
                    }
                    else
                    {
                        $lambdas[$features2id{$feature_sepcifier}] = $feature_str;
                    }
                    last;
                 }
            }
        }
    }
    close(CONF_FILE);
    return join(";",@lambdas);
}
