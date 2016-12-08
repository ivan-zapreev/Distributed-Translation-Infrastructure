#!/usr/bin/perl -w

use strict;
use warnings;

my($mert_options,$initial_txt,$cumul_nbest_bleu,$no_parallel)=@ARGV;
$no_parallel=1 if(!defined($no_parallel));

my $OISTERHOME=$ENV{'OISTERHOME'};
#my $mert='/homes/christof/resources/mteval/Programs/cmert-1.01-parallel-new/mert-regularized';
my $mert="$OISTERHOME/resources/software/cmert/cmert-1.01-parallel-new-no_v11-tok/mert";

my($work_dir)=$initial_txt=~/^(.*)\/?initial.txt$/;
$work_dir='.' if(!defined($work_dir)||$work_dir eq '');

exit (-1) if(!defined($initial_txt)||!defined($cumul_nbest_bleu));

my @initial_points;
open(I,"<$initial_txt");
while(defined(my $line=<I>)) {
    push(@initial_points,$line);
}
close(I);

my $batch_size=@initial_points/$no_parallel;

#print STDERR "batch_size=$batch_size\n";

my $batch_id=0;
my @batch;
my @initial_txt_batch_files;
my %final_file_sizes;
my @final_txt_batch_files;

#clean files:
print STDERR "rm -rf $work_dir/err.batch.*\n";
system("rm -rf $work_dir/err.batch.*");
print STDERR "rm -rf $work_dir/initial.txt.batch.*\n";
system("rm -rf $work_dir/initial.txt.batch.*");
print STDERR "rm -rf $work_dir/final.txt.batch.*\n";
system("rm -rf $work_dir/final.txt.batch.*");

for(my $i=0; $i<@initial_points; $i++) {
    if(@batch>=$batch_size || $i==@initial_points-1) {
       	push(@batch,$initial_points[$i]) if($i==@initial_points-1);
	
	open(B,">$initial_txt.batch.$batch_id");
	for(my $j=0; $j<@batch; $j++) {
	    print B $batch[$j];
	}
	close(B);
	$final_file_sizes{"$work_dir/final.txt.batch.$batch_id"}=@batch;
	push(@final_txt_batch_files,"$work_dir/final.txt.batch.$batch_id");
	push(@initial_txt_batch_files,"$initial_txt.batch.$batch_id");
	undef @batch;
	push(@batch,$initial_points[$i]);
    	$batch_id++;
    } else {
	push(@batch,$initial_points[$i]);
    }
}

my %errlog_files;

for(my $i=0; $i<@initial_txt_batch_files; $i++) {
    my $errlog="$work_dir/err.batch.$i";
    $errlog_files{$errlog}=1;
    print STDERR "nohup sh -c \'$mert \"$mert_options\" $initial_txt_batch_files[$i] $cumul_nbest_bleu 1> $work_dir/final.txt.batch.$i 2> $errlog\' \&\n";
    system("nohup sh -c \'$mert \"$mert_options\" $initial_txt_batch_files[$i] $cumul_nbest_bleu 1> $work_dir/final.txt.batch.$i 2> $errlog\' \&");
}

print STDERR "Optimizing ...  ";

my $finished=0;
my $prev_no_finished_points=0;
while(!$finished) {
    sleep(10);
    my $no_finished_points=0;
    foreach my $final_file (keys %final_file_sizes) {
	my $no_lines;
	if(!(-e "$final_file")) {
	    $finished=0;
#	    last;
	    $no_lines=0;
	} else {
	    $no_lines=`wc -l $final_file`;
	    chomp($no_lines);
	    $no_lines=~s/^([0-9]+).*$/$1/;
	}

	$no_finished_points+=$no_lines;
    }

    if($no_finished_points>$prev_no_finished_points) {
	my $no_digits = rindex($prev_no_finished_points,"");
	for(my $j=1; $j<=$no_digits; $j++) {
	    print STDERR "\x08";
	}
	print STDERR $no_finished_points;
    }
    $prev_no_finished_points=$no_finished_points;
    if($no_finished_points==@initial_points) {
	$finished=1;
	print STDERR " done.\n";
    }
}

#	if($no_lines==$final_file_sizes{$final_file}) {
#	    $finished=1;
#	} else {
#	    $finished=0;
#	    last;
#	}
#}


my @final_points;
for(my $i=0; $i<@final_txt_batch_files; $i++) {
    open(F,"<$final_txt_batch_files[$i]");
    while(defined(my $line=<F>)) {
	push(@final_points, $line);
    }
    close(F);
}

for(my $i=0; $i<@initial_txt_batch_files; $i++) {
    print STDERR "rm $initial_txt_batch_files[$i]\n";
    unlink("$initial_txt_batch_files[$i]");
}
for(my $i=0; $i<@final_txt_batch_files; $i++) {
    print STDERR "rm $final_txt_batch_files[$i]\n";
    unlink("$final_txt_batch_files[$i]");
}   

foreach my $errlog (keys %errlog_files) {
    print STDERR "rm $errlog\n";
    unlink("$errlog");
}   

for(my $i=0; $i<@final_points; $i++) {
    print $final_points[$i];
}



