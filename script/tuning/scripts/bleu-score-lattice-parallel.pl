#!/usr/bin/perl -w

use strict;
use warnings;

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


my($lattice_file,$O_score_file,$mu,$src_file,@ref_files)=@ARGV;

if(@ARGV<4) {
    print STDERR "\nusage: bleu-lattice-score-parallel.pl <lattice-file> <O-score-file> <src-file> <ref-files> <num-parallel>\n\n";
    exit(-1);
}

my $num_parallel=1;
if($ref_files[-1]=~/^([0-9]+)$/) {
    $num_parallel=$ref_files[-1];
    pop(@ref_files);
}
my $ref_files_string=join(' ',@ref_files);


my $total_size=0;
my %lattice_sizes;
my $num_sentences_remaining=0;

if($lattice_file=~/\.gz/) {
    open F, "<:gzip", $lattice_file, or die("can't open $lattice_file: $!\n");
} else {
    open(F,"<$lattice_file")||die("can't open file $lattice_file: $!\n");
}
while(defined(my $line=<F>)) {
    if($line=~/^<SENT ID=([0-9]+)>/) {
	my $sent_id=$1;
	$num_sentences_remaining++;
	my $lattice_size=0;
	while(defined($line=<F>) && $line!~/^<\/SENT>/) {
	    $_=$line;
	    $lattice_size+=tr/\|//;
	}
	$lattice_sizes{$sent_id}=$lattice_size;
	$total_size+=$lattice_size;
    }
}
close(F);

my $avg_size=$total_size/$num_parallel;
my @batches;
my $current_size=0;
my $sent_from;
my $sent_to;

print STDERR "avg_size=$avg_size\n";
foreach my $sent_id (sort {$a<=>$b} (keys %lattice_sizes)) {
    if(!defined($sent_from)) {
	$sent_from=$sent_id;
    }
    $num_sentences_remaining--;
    $current_size+=$lattice_sizes{$sent_id};
#    print STDERR "lattice_sizes{$sent_id}=$lattice_sizes{$sent_id}\n";
    if($current_size>=$avg_size || $num_sentences_remaining==0) {
	push(@batches,"$sent_from $sent_id");
#	push(@batches,"$sent_from $sent_id $current_size");
	$current_size=0;
	undef $sent_from;
    }
}

my $num_active_jobs=0;
my %finished_jobs;
my %active_jobs;
for(my $i=0; $i<@batches; $i++) {
    print STDERR "batches[$i]=$batches[$i]\n";
    my($from,$to)=split(/ /,$batches[$i]);
    my $finished_file="bleu_lattice.finished.$i";
    my $call="nohup sh -c \'$OISTERHOME/tuning/scripts/bleu-score-lattice.pl $lattice_file $O_score_file $mu $src_file $ref_files_string $from $to 1> $lattice_file.batch.$i 2> $lattice_file.err.$i.log\; touch $finished_file\' \&";
    print STDERR "$call\n";
    system($call);
    $num_active_jobs++;
    $active_jobs{$finished_file}=$i;
}

#exit(-1);

while($num_active_jobs>0) {
    sleep(3);
    foreach my $finished_file (keys %active_jobs) {
	if(-e "$finished_file" && !exists($finished_jobs{$finished_file})) {
	    $num_active_jobs--;
	    $finished_jobs{$finished_file}=$active_jobs{$finished_file};
	}
    }
}

# cleaning up:
foreach my $finished_file (keys %finished_jobs) {
    unlink($finished_file);
    unlink("$lattice_file.err.$finished_jobs{$finished_file}.log");    
}


for(my $i=0; $i<@batches; $i++) {
    my $batch_file="$lattice_file.batch.$i";
    open(F,"<$batch_file") ||die("can't open file $batch_file: $!\n");
    while(<F>) {
	print $_;
    }
    close(F);
    unlink($batch_file);
}



