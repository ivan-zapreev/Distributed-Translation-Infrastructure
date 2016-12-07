#!/usr/bin/perl -w

if(@ARGV<5||@ARGV>7) {
    print STDERR "\nusage: $0 <LATTICE-FILE-SCORE> <LATTICE-FILE-COMPARISON> <FEATURE-SCORES-FILE> <FEATURE-ID2NAME-FILE> <\#N-BEST> <num-parallel> [<external-path>]\n\n";
    exit(-1);
};

print STDERR "get-nbest-carmel.pl pid=$$\n";

($lattice_file_score,$lattice_file_comparison,$feature_scores_file,$feature_id2name_file,$nbest,$num_parallel,$external_path)=@ARGV;

BEGIN {
    if(!defined($ENV{'OISTERHOME'})
       || $ENV{'OISTERHOME'} eq '') {
        print STDERR "environment variable OISTERHOME must be set:\n";
        print STDERR "export OISTERHOME=/path/to/oister/distribution\n";
        exit(-1);
    }
}

my $OISTERHOME=$ENV{'OISTERHOME'};
use PerlIO::gzip;


if(!defined($external_path)
   && exists($ENV{'OISTEREXTPATH'}) && defined($ENV{'OISTEREXTPATH'})) {
    $external_path=$ENV{'OISTEREXTPATH'};
} elsif(!defined($external_path)) {
    print STDERR "external_path must be set\n";
    exit(-1);
}

my $total_size=0;
my %lattice_sizes;
my $num_sentences_remaining=0;


if($lattice_file_score=~/\.gz/) {
    open F, "<:gzip", $lattice_file_score, or die("can't open $lattice_file_score: $!\n");
} else {
    open(F,"<$lattice_file_score")||die("can't open file $lattice_file_score: $!\n");
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

my $out_file="$lattice_file_score." . "nbest";
my $scored_file="$lattice_file_score." . "nbest.rescored";

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
#       push(@batches,"$sent_from $sent_id $current_size");
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
    my $call="nohup sh -c \'$OISTERHOME/tuning/scripts/get-nbest-carmel-feature-scores-bleu-and-model.pl $lattice_file_score:$from\-$to $lattice_file_comparison $feature_scores_file $feature_id2name_file $nbest $external_path >& $lattice_file_score.err.$i.log\; touch $finished_file\' \&";
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
    unlink("$lattice_file_score.err.$finished_jobs{$finished_file}.log");
}

open(O,">$out_file");
for(my $i=0; $i<@batches; $i++) {
    my($from,$to)=split(/ /,$batches[$i]);

    my $batch_file="$lattice_file_score.nbest.$from\-$to";

    open(F,"<$batch_file") ||die("can't open file $batch_file: $!\n");
    while(<F>) {
        print O $_;
    }
    close(F);
    unlink($batch_file);
}
close(O);

if(0) {
    open(O,">$scored_file");
    for(my $i=0; $i<@batches; $i++) {
        my($from,$to)=split(/ /,$batches[$i]);
        my $batch_file="$lattice_file_score.nbest.$from\-$to.rescored";
        open(F,"<$batch_file") ||die("can't open file $batch_file: $!\n");
        while(<F>) {
            print O $_;
        }
        close(F);
        unlink($batch_file);
    }
    close(O);
}

