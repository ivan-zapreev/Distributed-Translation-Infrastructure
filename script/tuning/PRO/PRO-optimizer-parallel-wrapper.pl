#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";
use warnings;
use File::Basename;

my $_HELP=0;
my $work_dir;
my $nbest_pool;
my $iteration;
my $no_parallel;
my $lattice_oracle_scored;
my $model_nbest_scored;
my $model_nbest_lai_scored;
my $config_file;

GetOptions(
    "conf=s" => \$config_file,
    "work-dir=s" => \$work_dir,
    "nbest-pool=s" => \$nbest_pool,
    "iteration|iter=i" => \$iteration,
    "num-parallel|no-parallel=s" => \$no_parallel,
    "lattice-oracle-scored=s" => \$lattice_oracle_scored,
    "model-nbest-scored=s" => \$model_nbest_scored,
    "lattice-lai-scored=s" => \$model_nbest_lai_scored,
    "help" => \$_HELP
	   );

if(!defined($nbest_pool)) {
    $_HELP=1;
}

if($_HELP) {
    print "\nOptions:
   --help : print this message.\n\n";
    exit(-1);
}

my $script_name = basename($0);
print STDERR "$script_name pid=$$\n";

$no_parallel||=1;

my $OISTERHOME=$ENV{'OISTERHOME'};

my %to_finish_files;
for(my $i=0; $i<$no_parallel; $i++) {
    my $finished_file="$work_dir/pro_finished.$i";
    my $pro_call;
    if(defined($lattice_oracle_scored) && defined($model_nbest_scored)) {
        if(defined($model_nbest_lai_scored)) {
            $pro_call="nohup sh -c \'$OISTERHOME/tuning/PRO/PRO-optimizer-lattice.pl --work-dir=$work_dir --nbest-pool=$nbest_pool --sample-id=$i --lattice-oracle-scored=$lattice_oracle_scored --model-nbest-scored=$model_nbest_scored --lattice-lai-scored=$model_nbest_lai_scored --conf=$config_file > $work_dir/run$iteration.weights$i\; touch $finished_file 2> $work_dir/pro_err.$i\' &";
        } else {
            $pro_call="nohup sh -c \'$OISTERHOME/tuning/PRO/PRO-optimizer-lattice.pl --work-dir=$work_dir --nbest-pool=$nbest_pool --sample-id=$i --lattice-oracle-scored=$lattice_oracle_scored --model-nbest-scored=$model_nbest_scored > $work_dir/run$iteration.weights$i\; touch $finished_file 2> $work_dir/pro_err.$i\' &";
        }
    } else {
        $pro_call="nohup sh -c \'$OISTERHOME/tuning/PRO/PRO-optimizer.pl $work_dir $nbest_pool $i > $work_dir/run$iteration.weights$i\; touch $finished_file 2> $work_dir/pro_err.$i\' &";
    }
    $to_finish_files{$finished_file}=1;
    print STDERR "$pro_call\n";
    system($pro_call);
}

my $finished=0;
while(!$finished) {
    my $all_files_present=1;
    foreach my $to_finish_file (keys %to_finish_files) {
        if(!( -e "$to_finish_file")) {
            $all_files_present=0;
            last;
        }
    }
    if($all_files_present) {
        sleep(10);
        $finished=1;
    } else {
        sleep(5);
    }
}

#combine feature weights;
my @feature_weights;
my @feature_counts;
for(my $i=0; $i<$no_parallel; $i++) {
    open(F,"<$work_dir/run$iteration.weights$i")||die("can't open file $work_dir/run$iteration.weights$i: $!\n");
    while(defined(my $line=<F>)) {
        if($line=~/^\s*[F]([0-9]+)[\s\t]+([0-9\-e\+\.]+)\s*\n/) {
            my $feature_id=$1;
            my $feature_weight=$2;
            $feature_weights[$feature_id]+=$feature_weight;
            $feature_counts[$feature_id]++;
        }
    }
    close(F);
}

for(my $i=0; $i<@feature_weights; $i++) {
    next if(!defined($feature_weights[$i]));
#    $feature_weights[$i]/=$feature_counts[$i];
    $feature_weights[$i]/=$no_parallel;
    print "F$i $feature_weights[$i]\n";
}

