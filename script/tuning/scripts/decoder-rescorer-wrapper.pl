#!/usr/bin/perl -w

use strict;
use warnings;
use Getopt::Long "GetOptions";
use File::Basename;
use PerlIO::gzip;
use Cwd 'abs_path';

my $nbest_size;
my $lambda_string;
my $prefix;
my $parameters;
my $src_file;
my $nbest_file;

my $_HELP;
$_HELP = 1
unless &GetOptions(
    "nbest-size=i" => \$nbest_size,
    "decoder-parameters=s" => \$parameters,
    "lambda-string=s" => \$lambda_string,
    "prefix=s" => \$prefix,
    "help|h" => \$_HELP
);

if($_HELP) {
    print "Options:
    --nbest-size=int : size of the nbest-list
    --external-path=str : path to 3rd party software directory
    --decoder-parameters=str : list of parameters between quotation marks
    --lambda-string=str : comma-separated list of lambda parameters
    --prefix=str : working directory+iteration-run (e.g., mert-work/run3)
    --help : print this message.\n\n";
    exit(-1);
}

my $script_abs_name=abs_path($0);
my $scripts_location=dirname($script_abs_name);
my $script_name = basename($0);
print STDERR "$script_name pid=$$\n";

print STDERR "parameters=$parameters\n";
print STDERR "Lambdas=$lambda_string\n";

my $use_node_scores=1;

my($decoder)=$parameters=~/\-\-?decoder[\= ]+([^ ]+)/;
my($config_file)=$parameters=~/\-\-?conf[\= ]+([^ ]+)/;
my($mert_script)=$parameters=~/\-\-?mert-script[\= ]+([^ ]+)/;
my($no_parallel)=$parameters=~/\-\-?no-parallel[\= ]+([^ ]+)/;
my($trg_language)=$parameters=~/\-\-?trg-lang(?:uage)?[\= ]+([^ ]+)/;

$trg_language='english' if(!defined($trg_language)
    || $trg_language eq '');

my $feature_id2name_file="$config_file.feature_id2name";

my $carmel=$scripts_location."/get-k-best.pl";

$mert_script='cmert-closest' if(!defined($mert_script));
$no_parallel=1 if(!defined($no_parallel));

my $delete_tmp_src_file=0;

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

unshift (@INC, $scripts_location);
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


open(FEATURES_MAPPING_FILE, "<$feature_id2name_file");
my %features2id = ();
my %features_specifiers = ();
while(my $line = <FEATURES_MAPPING_FILE>)
{
    chomp($line);
    my($index, $feature_name) = split(/\t/, $line);

    if($feature_name =~ /^([^\[]+)\[(\d+)\]$/)
    {
        $features_specifiers{$1} = $2 + 1; #This feature specifier would have multiple values
    }
    else
    {
        $features_specifiers{$feature_name} = 0;
    }
    $features2id{$feature_name} = $index;
}
close(FEATURES_MAPPING_FILE);


my %data;
my %parameters;
my %features;
my %feature_order;

my @config_buffer=&buffer_config_file($config_file);

my $current_feature=0;

for(my $i=0; $i<@config_buffer && $current_feature<@lambdas; $i++) {
    my $config_buffer_copy = $config_buffer[$i];
    chomp($config_buffer_copy);
    $config_buffer_copy =~ s/^\s+|\s+$//g;
    foreach my $feature_specifier (keys(%features_specifiers))
    {
        if($config_buffer_copy =~ m/^[\s\t]*$feature_specifier=([^\s\t]*)[\s\t]*$/)
        {

            $current_feature = $current_feature + $features_specifiers{$feature_specifier};
            my $value_string;
            if($features_specifiers{$feature_specifier} > 0)
            {
                my @lambdas_underprecisioned = ();
                my @currentFeatureSpec_values = @lambdas[$features2id{"$feature_specifier\[0\]"} .. $features2id{"$feature_specifier\[0\]"} + $features_specifiers{$feature_specifier} - 1];
                $value_string = join("|", @currentFeatureSpec_values);
            }
            else
            {
                $current_feature++;
                $value_string = $lambdas[$features2id{$feature_specifier}];
            }
            my $value = "$feature_specifier=$value_string";
            $config_buffer[$i] =~ s/^([\s\t]*)$feature_specifier=([^\s\t]*)([\s\t]*)$/$1$value$3/;
            last;
        }
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
    print STDERR "nohup sh -c \'$carmel $prefix.trans.lattices.batch.$batch $prefix.trans.feature_scores.batch.$batch $feature_id2name_file $nbest_size 2>&1 1> $prefix.trans.lattices.batch.$batch.err; touch $prefix.trans.lattices.batch.$batch.done\' \&\n";
    system("nohup sh -c \'$carmel $prefix.trans.lattices.batch.$batch $prefix.trans.feature_scores.batch.$batch $feature_id2name_file $nbest_size 2>&1 1> $prefix.trans.lattices.batch.$batch.err; touch $prefix.trans.lattices.batch.$batch.done\' \&");
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

system("sync");

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
    unlink("$prefix.trans.lattices.batch.$batch") if($delete_files);
}
close(ALL);

open(ALL,">$prefix.trans.lattices.nbest");
for(my $batch=0; $batch<$no_parallel; $batch++) {
    open(NBEST,"<$prefix.trans.lattices.batch.$batch.nbest");
    while(defined(my $line=<NBEST>)) {
        print ALL $line;
    }
    close(NBEST);
    unlink("$prefix.trans.lattices.batch.$batch.nbest") if($delete_files);
}
close(ALL);

if($use_node_scores) {
    open(ALL,">$prefix.trans.lattices.nbest.rescored");
    open(ALLFEAT,">$prefix.trans.feature_scores");
    for(my $batch=0; $batch<$no_parallel; $batch++) {
        open(NBEST,"<$prefix.trans.lattices.batch.$batch.nbest.rescored");
        while(defined(my $line=<NBEST>)) {
            print ALL $line;
        }
        close(NBEST);
        unlink("$prefix.trans.lattices.batch.$batch.nbest.rescored");
        open(SCORES,"<$prefix.trans.feature_scores.batch.$batch");
        while(defined(my $line=<SCORES>)) {
            print ALLFEAT $line;
        }
        close(SCORES);
        unlink("$prefix.trans.feature_scores.batch.$batch") if($delete_files);
    }
    close(ALL);
}

system("sync");


if(!$use_node_scores) {
    my $rescore_parameters=$parameters;
    $rescore_parameters.=" --rescore --nbest-file=$prefix.trans.lattices.nbest";

    print STDERR "Start rescoring...\n";
    print STDERR "$decoder $rescore_parameters\n";
    system("$decoder $rescore_parameters");
    print STDERR " done.\n";
}


my $process_pipeline="$scripts_location/rescore-remove-nil-lines.pl | $scripts_location/nbest-extract.pl | $scripts_location/postprocess1_no-untok | $scripts_location/nbest-replace.pl";
if($trg_language!~/^(english|dutch|german|italian)$/) {
    $process_pipeline="$scripts_location/rescore-remove-nil-lines.pl | $scripts_location/nbest-extract.pl | $scripts_location/postprocess1_no-untok_no-rm | $scripts_location/nbest-replace.pl";
}

print STDERR "cat $prefix.trans.lattices.nbest.rescored | $process_pipeline $prefix.trans.lattices.nbest.rescored\n";
system("cat $prefix.trans.lattices.nbest.rescored | $process_pipeline $prefix.trans.lattices.nbest.rescored");

#delete files to save disk-space:
if($delete_files) {
    print STDERR "rm $prefix.trans.lattices.nbest\n";
    unlink("$prefix.trans.lattices.nbest");
    print STDERR "rm $prefix.trans.lattices.nbest.rescored\n";
    unlink("$prefix.trans.lattices.nbest.rescored");

}

if($delete_tmp_src_file) {
    print STDERR "rm $$.tmp.src\n";
    unlink("$$.tmp.src");
}
