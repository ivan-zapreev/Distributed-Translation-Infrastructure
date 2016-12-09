#!/usr/bin/perl -w

use File::Basename;
use PerlIO::gzip;
use Cwd 'abs_path';

if(@ARGV<4||@ARGV>6) {
    print STDERR "\nusage: $0 <LATTICE-FILE> <FEATURE-SCORES-FILE> <FEATURE-ID2NAME-FILE> <\#N-BEST> <num-parallel> [<external-path>]\n\n";
    exit(-1);
};

my $scripts_location=abs_path($0);
my $script_name = basename($0);
print STDERR "$script_name pid=$$\n";

($lattice_file,$feature_scores_file,$feature_id2name_file,$nbest,$num_parallel,$external_path)=@ARGV;

if(!defined($external_path)) {
    print STDERR "external_path must be set\n";
    exit(-1);
}

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

my $out_file="$lattice_file." . "nbest";
my $scored_file="$lattice_file." . "nbest.rescored";

print STDERR "avg_size=$avg_size\n";
foreach my $sent_id (sort {$a<=>$b} (keys %lattice_sizes)) {
    if(!defined($sent_from)) {
        $sent_from=$sent_id;
    }
    $num_sentences_remaining--;
    $current_size+=$lattice_sizes{$sent_id};
    if($current_size>=$avg_size || $num_sentences_remaining==0) {
        push(@batches,"$sent_from $sent_id");
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
    my $finished_file="carmel_batch.finished.$i";
    my $call="nohup sh -c \'$scripts_location/get-k-best.pl $lattice_file:$from\-$to $feature_scores_file $feature_id2name_file $nbest $external_path >& $lattice_file.err.$i.log\; touch $finished_file\' \&";
    print STDERR "$call\n";
    system($call);
    $num_active_jobs++;
    $active_jobs{$finished_file}=$i;
}

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

open(O,">$out_file");
for(my $i=0; $i<@batches; $i++) {
    my($from,$to)=split(/ /,$batches[$i]);

    my $batch_file="$lattice_file.nbest.$from\-$to";

    open(F,"<$batch_file") ||die("can't open file $batch_file: $!\n");
    while(<F>) {
        print O $_;
    }
    close(F);
    unlink($batch_file);
}
close(O);

open(O,">$scored_file");
for(my $i=0; $i<@batches; $i++) {
    my($from,$to)=split(/ /,$batches[$i]);
    my $batch_file="$lattice_file.nbest.$from\-$to.rescored";
    open(F,"<$batch_file") ||die("can't open file $batch_file: $!\n");
    while(<F>) {
        print O $_;
    }
    close(F);
    unlink($batch_file);
}
close(O);


