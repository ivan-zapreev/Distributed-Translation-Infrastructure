#!/usr/bin/perl -w

use strict;
use warnings;
use Getopt::Long "GetOptions";

#Hot-fix
#my $HOME=$ENV{'HOME'};
#my $HOME='/home/christof/';


#BEGIN {
#    if(!defined($ENV{'OISTERHOME'})
#       || $ENV{'OISTERHOME'} eq '') {
#        print STDERR "environment variable OISTERHOME must be set:\n";
#        print STDERR "export OISTERHOME=/path/to/oister/distribution\n";
#        exit(-1);
#    }
#}

#BEGIN {
#   my $release_info=`cat /etc/*-release`;
#    $release_info=~s/\n/ /g;
#    my $os_release;
#    if($release_info=~/CentOS release 5\./) {
#        $os_release='CentOS_5';
#    } elsif($release_info=~/CentOS release 6\./) {
#        $os_release='CentOS_6';
#    }
#    if($os_release eq 'CentOS_6') {
#        unshift @INC, $ENV{"OISTERHOME"}."/lib/perl_modules/lib64/perl5"
#    } else {
#        unshift @INC, $ENV{"OISTERHOME"}."/resources/bin/lib64/perl5/site_perl/5.8.8/x86_64-linux-thread-multi"
#    }
#}
use PerlIO::gzip;

#my $OISTERHOME=$ENV{'OISTERHOME'};


my $f_suffix;
my $e_suffix;
my $corpus_stem;
my $sigma;
my $filter='';
my $align_files_string;
my $exclude='';
my $use_moses_orientation=0;
my $use_dlr=0;
my $use_hdm=0;
my $dm_languages='fe';
my $external_bin_dir;
my $exp_dir;
my $pid=$$;

my $_HELP;
$_HELP = 1
    unless &GetOptions(
       "f=s" => \$f_suffix,
       "e=s" => \$e_suffix,
       "corpus=s" => \$corpus_stem,
       "sigma=s" => \$sigma,
       "align-files=s" => \$align_files_string,
       "filter=s" => \$filter,
       "exclude=s" => \$exclude,
       "external-bin-dir=s" => \$external_bin_dir,
       "moses-orientation" => \$use_moses_orientation,
       "use-dlr" => \$use_dlr,
       "use-hdm" => \$use_hdm,
       "dm-lang=s" => \$dm_languages,
       "experiment-dir=s" => \$exp_dir,
		       );

if(!defined($corpus_stem)) {
    $_HELP=1;
}

if($_HELP) {
    print "\nOptions:
  --f=string foreign suffix
  --e=string native suffix
  --corpus=string
  --sigma=value for sigma interpolation
  --align-files=string (alignment files: file1,file2,..)
  --filter=string (sub-corpora to be considered)
  --exclude=string (sub-corpora to be excluded)
  --external-bin-dir=moses-link external binaries path
  --use-dlr (use discontinous left+right)
  --use-hdm : build hierarchical distortion model
  --dm-lang=string ('fe,f,e' or 'fe,e' or 'fe'=default)
  --help : print this message.\n\n";
    exit(-1);
}

$sigma=0.75 if(!defined($sigma));


my $moses_train_script="$external_bin_dir/moses/scripts/training/train-model.perl";
my $moses_train_wrapper="$external_bin_dir/build/bitext_models/moses-train-wrapper.pl";
my $moses_extract="$external_bin_dir/moses/bin/extract";

if(!defined($align_files_string)) {
    die("You must specify at least one alignment_file.\n");
}

die("--f and --e must have different values.\n") if($f_suffix eq $e_suffix);

my $dm_dir="$exp_dir/dm";
#if(-e "$dm_dir") {
#    die("directory $dm_dir already exists.\n");
#}

if(!(-e "$dm_dir")) {
    system("mkdir $dm_dir");
}

my @align_files=split(/\,/,$align_files_string);
my @extract_files;
my @align_files_extract;
my @extract_hdm_files;
my @align_files_hdm_extract;

for(my $i=0; $i<@align_files; $i++) {
    my $align_file=$align_files[$i];

    my $aligned_file_out=$align_file;
    $aligned_file_out=~s/^.*\/([^\/]+)$/$1/;
#    my($align_strategy)=$aligned_file_out=~/^aligned\.(.+)$/;
#    my($aligner)=$align_file=~/\/align\.(berkeley|m1-m5|hmm)\/refined\./;

    my $align_strategy='grow-diag-final';

#    my $extract_dir="$dm_dir/moses-extract.$aligner.$align_strategy";
    my $extract_dir="$dm_dir/moses-extract/";

#    if(-e "$extract_dir") {
#	die("directory $extract_dir already exists.\n");
#    }

    system("mkdir $extract_dir");
    system("mkdir $extract_dir/corpus");
    system("mkdir $extract_dir/model");
    my($corpus_file_stem)=$corpus_stem=~/^.*\/([^\/]+)$/;

    system("cp $corpus_stem.$f_suffix $extract_dir/corpus/$corpus_file_stem.$f_suffix");
    system("cp $corpus_stem.$e_suffix $extract_dir/corpus/$corpus_file_stem.$e_suffix");
    system("cp $align_file $extract_dir/model/aligned.$align_strategy");

#my $moses_call="$moses_train_script --root-dir=moses-extract --f=$f_suffix --e=$e_suffix --corpus=moses-extract/corpus/$corpus_file_stem --alignment=$align_strategy --first-step=5 --last-step=5";
#print STDERR "$moses_call\n";
#system("$moses_call");

    my $phrase_length=7;

 #   my $moses_call="nohup sh -c \'$moses_train_wrapper $moses_train_script $pid \"--root-dir=$extract_dir --f=$f_suffix --e=$e_suffix --corpus=$extract_dir/corpus/$corpus_file_stem --alignment=$align_strategy --external-bin-dir=$external_bin_dir --first-step=5 --last-step=5\" >& $extract_dir/err.log\' \&";
    my $moses_call="nohup sh -c \'$moses_train_wrapper $moses_extract $pid \"$extract_dir/corpus/dm_aligned.$e_suffix $extract_dir/corpus/dm_aligned.$f_suffix $extract_dir/model/aligned.$align_strategy $extract_dir/model/extract $phrase_length orientation --model phrase-msd --GZOutput --SentenceOffset 0\" >& $extract_dir/err.log\' \&";

#--root-dir=$extract_dir --f=$f_suffix --e=$e_suffix --corpus=$extract_dir/corpus/$corpus_file_stem --alignment=$align_strategy --external-bin-dir=$external_bin_dir --first-step=5 --last-step=5\" >& $extract_dir/err.log\' \&";

    print STDERR "$moses_call\n";
    system("$moses_call");

    my $finished=0;
    while(!$finished) {
	if(-e "$pid.finished") {
	    $finished=1;
	} else {
	    for(my $part=0; $part<1000; $part++) {
		my $part_suffix=$part;
		$part_suffix="0$part_suffix" if($part<1000);
		$part_suffix="0$part_suffix" if($part<100);
		$part_suffix="0$part_suffix" if($part<10);

		my $next_part_suffix=$part+1;
		$next_part_suffix="0$next_part_suffix" if($part+1<1000);
		$next_part_suffix="0$next_part_suffix" if($part+1<100);
		$next_part_suffix="0$next_part_suffix" if($part+1<10);

#	    if(-e "$extract_dir/model/extract.0-0.part$part_suffix" && -e "$extract_dir/model/extract.0-0.part$next_part_suffix") {
#		print STDERR "rm -f $extract_dir/model/extract.0-0.part$part_suffix\n";
#		system("rm -f $extract_dir/model/extract.0-0.part$part_suffix");
#	    }

		if(-e "$extract_dir/model/extract.inv.part$part_suffix" && -e "$extract_dir/model/extract.inv.part$next_part_suffix") {
		    print STDERR "rm -f $extract_dir/model/extract.inv.part$part_suffix\n";
		    system("rm -f $extract_dir/model/extract.inv.part$part_suffix");
		}

		if(-e "$extract_dir/model/extract.o.part$part_suffix" && -e "$extract_dir/model/extract.o.part$next_part_suffix") {
		    print STDERR "rm -f $extract_dir/model/extract.o.part$part_suffix\n";
		    system("rm -f $extract_dir/model/extract.o.part$part_suffix");
		}


	    }
	    sleep(10);
	}
    }
    system("rm -f $pid.finished");

    system("mv $extract_dir/model/extract.gz $extract_dir/model/extract.$phrase_length.gz");
    system("rm -f $extract_dir/model/extract.inv.gz");
    system("rm -f $extract_dir/model/extract.o.gz");

    push(@extract_files,"$extract_dir/model/extract.$phrase_length.gz");
    push(@align_files_extract,"$extract_dir/model/aligned.$align_strategy");

    if($use_hdm) {
	my $phrase_length=100;

#	my $moses_call="nohup sh -c \'$moses_train_wrapper $moses_train_script $pid \"--root-dir=$extract_dir --f=$f_suffix --e=$e_suffix --corpus=$extract_dir/corpus/$corpus_file_stem --alignment=$align_strategy --max-phrase-length=$phrase_length --first-step=5 --last-step=5\" >& $extract_dir/err.log\' \&";

	my $moses_call="nohup sh -c \'$moses_train_wrapper $moses_extract $pid \"$extract_dir/corpus/dm_aligned.$e_suffix $extract_dir/corpus/dm_aligned.$f_suffix $extract_dir/model/aligned.$align_strategy $extract_dir/model/extract $phrase_length orientation --model phrase-msd --GZOutput --SentenceOffset 0\" >& $extract_dir/err.log\' \&";


	print STDERR "$moses_call\n";
	system("$moses_call");

	my $finished=0;
	while(!$finished) {
	    if(-e "$pid.finished") {
		$finished=1;
	    } else {
		for(my $part=0; $part<1000; $part++) {
		    my $part_suffix=$part;
		    $part_suffix="0$part_suffix" if($part<1000);
		    $part_suffix="0$part_suffix" if($part<100);
		    $part_suffix="0$part_suffix" if($part<10);

		    my $next_part_suffix=$part+1;
		    $next_part_suffix="0$next_part_suffix" if($part+1<1000);
		    $next_part_suffix="0$next_part_suffix" if($part+1<100);
		    $next_part_suffix="0$next_part_suffix" if($part+1<10);

		    if(-e "$extract_dir/model/extract.inv.part$part_suffix" && -e "$extract_dir/model/extract.inv.part$next_part_suffix") {
			print STDERR "rm -f $extract_dir/model/extract.inv.part$part_suffix\n";
			system("rm -f $extract_dir/model/extract.inv.part$part_suffix");
		    }

		    if(-e "$extract_dir/model/extract.o.part$part_suffix" && -e "$extract_dir/model/extract.o.part$next_part_suffix") {
			print STDERR "rm -f $extract_dir/model/extract.o.part$part_suffix\n";
			system("rm -f $extract_dir/model/extract.o.part$part_suffix");
		    }
		}
		sleep(10);
	    }
	}
	system("rm -f $pid.finished");

	system("mv $extract_dir/model/extract.gz $extract_dir/model/extract.$phrase_length.gz");
	system("rm -f $extract_dir/model/extract.inv.gz");
	system("rm -f $extract_dir/model/extract.o.gz");

	push(@extract_hdm_files,"$extract_dir/model/extract.$phrase_length.gz");
	push(@align_files_hdm_extract,"$extract_dir/model/aligned.$align_strategy");
    }

}

my $extract_files_arg;
my $extract_hdm_files_arg='';

if($use_moses_orientation) {
    my @extract_align_files;
    for(my $i=0; $i<@extract_files; $i++) {
	$extract_align_files[$i]="$extract_files[$i]:$align_files_extract[$i]";
    }
    $extract_files_arg=join(' ',@extract_align_files);

    if($use_hdm) {
	my @extract_hdm_align_files;
	for(my $i=0; $i<@extract_hdm_files; $i++) {
	    $extract_hdm_align_files[$i]="$extract_hdm_files[$i]:$align_files_hdm_extract[$i]";
	}
	$extract_hdm_files_arg=join(' ',@extract_hdm_align_files);
	$extract_hdm_files_arg=' ' . $extract_hdm_files_arg;
    }
} else {
    $extract_files_arg=join(' ',@extract_files);
    if($use_hdm) {
	$extract_hdm_files_arg=join(' ',@extract_hdm_files);
    }
}
my $orientation_file="$dm_dir/orientation.gz";
my $orientation_hdm_file="$dm_dir/orientation_hdm.gz";

my $moses_orientation_parameter='';
my $no_orientations=3;
if($use_moses_orientation) {
    $moses_orientation_parameter='moses-orientation ';
}
if($use_dlr) {
    $moses_orientation_parameter.='use-dlr ';
    $no_orientations=4;
}


if(0) {
    my $tmp_file="$orientation_file.tmp";
    my $orientation_call="$external_bin_dir/build/bitext_models/extract-orientation-from-multiple-extracts.pl $moses_orientation_parameter$extract_files_arg > $tmp_file";
    my $orientation_call2="cat $tmp_file | sort -T $dm_dir | gzip > $orientation_file";
    print STDERR "$orientation_call\n";
    system($orientation_call);
    print STDERR "$orientation_call2\n";
    system($orientation_call2);
}

if(1) {
    my $orientation_call="$external_bin_dir/build/bitext_models/extract-orientation-from-multiple-extracts.pl $moses_orientation_parameter$extract_files_arg | sort -T $dm_dir | gzip > $orientation_file";
    print STDERR "$orientation_call\n";
    system($orientation_call);
}

if($use_hdm) {
    my $orientation_call="$external_bin_dir/build/bitext_models/extract-orientation-from-multiple-extracts.pl use-hdm $moses_orientation_parameter$extract_files_arg$extract_hdm_files_arg | sort -T $dm_dir | gzip > $orientation_hdm_file";
    print STDERR "$orientation_call\n";
    system($orientation_call);
}

my @dm_langs=split(/\,/,$dm_languages);
for(my $l=0; $l<@dm_langs; $l++) {
    my $dm_lang=$dm_langs[$l];
#    my $build_dm_call="$HOME/scripts/moses-build-distortion-model-new.pl $orientation_file $no_orientations $sigma $dm_lang | gzip 1> $dm_dir/dm\_$dm_lang\_$sigma.gz";
    my $build_dm_call="$external_bin_dir/build/bitext_models/moses-build-distortion-model.pl $orientation_file $no_orientations $sigma $dm_lang | gzip 1> $dm_dir/dm\_$dm_lang\_$sigma.gz";
    print STDERR "$build_dm_call\n";
    system($build_dm_call);

    if($use_hdm) {
	my $build_dm_call="$external_bin_dir/build/bitext_models/moses-build-distortion-model.pl $orientation_hdm_file $no_orientations $sigma $dm_lang | gzip 1> $dm_dir/hdm\_$dm_lang\_$sigma.gz";
	print STDERR "$build_dm_call\n";
	system($build_dm_call);
    }
}

unlink("$orientation_file");
unlink("$orientation_hdm_file");
