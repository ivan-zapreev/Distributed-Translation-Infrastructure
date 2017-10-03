#!/usr/bin/perl -w

use strict;
use warnings;
use IO::File;

#BEGIN {
#    if(!defined($ENV{'OISTERHOME'})
#       || $ENV{'OISTERHOME'} eq '') {
#        print STDERR "environment variable OISTERHOME must be set:\n";
#        print STDERR "export OISTERHOME=/path/to/oister/distribution\n";
#        exit(-1);
#    }
#}
#BEGIN {
#    my $release_info=`cat /etc/*-release`;
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

if(@ARGV==0) {
    print STDERR "No arguments provided.\n";
    exit(-1);
}

my @extract_files=@ARGV;
my $moses_orientation=0;
my $no_orientations=3;

my @extract_files_tmp;
my $use_dlr=0;
my $use_hdm=0;
for(my $i=0; $i<@extract_files; $i++) {
    if($extract_files[$i]=~/^moses\-orient/) {
	$moses_orientation=1;
    } elsif($extract_files[$i]=~/use\-hdm/) {
	    $use_hdm=1;
    } elsif($extract_files[$i]=~/use\-dlr/) {
	    $use_dlr=1;
	    $no_orientations=4;
    } else {
	push(@extract_files_tmp,$extract_files[$i]);
    }
}
undef @extract_files;
my @align_files;
my @extract_hdm_files;
for(my $i=0; $i<@extract_files_tmp; $i++) {
    if($extract_files_tmp[$i]=~/\:/) {
	my($extract_file,$align_file)=$extract_files_tmp[$i]=~/^(.+)\:(.+)$/;
	if($use_hdm && $extract_file=~/\.100\.gz$/) {
	    push(@extract_hdm_files,$extract_file);
	} else {
	    push(@extract_files,$extract_file);
	    push(@align_files,$align_file);
	}
    } else {
	if($use_hdm && $extract_files_tmp[$i]=~/\.100\.gz$/) {
	    push(@extract_hdm_files,$extract_files_tmp[$i]);
	} else {
	    push(@extract_files,$extract_files_tmp[$i]);
	}
    }
}
undef @extract_files_tmp;

my $use_alignments=0;
if(@align_files>0) {
    $use_alignments=1;
}

my @filehandles;
for(my $i=0; $i<@extract_files; $i++) {
    if($extract_files[$i]=~/^moses\-orient/) {
	$moses_orientation=1;
    } else {
	$filehandles[$i]=IO::File->new();
    }
}

my @filehandles_hdm;
for(my $i=0; $i<@extract_hdm_files; $i++) {
    if($extract_hdm_files[$i]=~/^moses\-orient/) {
	$moses_orientation=1;
    } else {
	$filehandles[$i]=IO::File->new();
    }
}


my @filehandles_align;
for(my $i=0; $i<@align_files; $i++) {
    $filehandles_align[$i]=IO::File->new();
}

for(my $i=0; $i<@filehandles; $i++) {
    if($extract_files[$i]=~/\.gz$/) {
	open $filehandles[$i], "<:gzip", $extract_files[$i], or die("can't open file $extract_files[$i]: $!\n");
    } else {
	open($filehandles[$i],"<$extract_files[$i]")||die("can't open file $extract_files[$i]: $!\n");
    }

    if(defined($align_files[$i])) {
	if($align_files[$i]=~/\.gz$/) {
	    open $filehandles_align[$i], "<:gzip", $align_files[$i], or die("can't open file $align_files[$i]: $!\n");
	} else {
	    open($filehandles_align[$i],"<$align_files[$i]")||die("can't open file $align_files[$i]: $!\n");
	}
    }

    if($use_hdm && defined($extract_hdm_files[$i])) {
	if($extract_hdm_files[$i]=~/\.gz$/) {
	    open $filehandles_hdm[$i], "<:gzip", $extract_hdm_files[$i], or die("can't open file $extract_hdm_files[$i]: $!\n");
	} else {
	    open($filehandles_hdm[$i],"<$extract_hdm_files[$i]")||die("can't open file $extract_hdm_files[$i]: $!\n");
	}
    }

}

my $finished=0;
my @lines;
my @lines_hdm;

#my $debug_id=0;

while(!$finished) {
    my %extracted;
    my %counts;
    my %counts_inv;
    my %counts_hdm;
    my %counts_inv_hdm;
    my %f_phrase;
    my %e_phrase;
    my %f2e_align;
    my %e2f_align;


    for(my $i=0; $i<@filehandles; $i++) {
	$lines[$i]=$filehandles[$i]->getline;

	next if(!defined($lines[$i]));
	my $line=$lines[$i];
	if($line=~/^<sent/) {

#            $debug_id++;
#            if($debug_id>10) {
#                exit(-1);
#            }

	    my $align_line;
	    if(defined($align_files[$i])) {
		$_=$filehandles_align[$i]->getline;
		$align_line=$filehandles_align[$i]->getline;
		$_=$filehandles_align[$i]->getline;
		chomp($align_line);
		my @align_pairs=split(/ +/,$align_line);
		for(my $i=0; $i<@align_pairs; $i++) {
		    my($f_i,$e_j)=split(/\-/,$align_pairs[$i]);
		    $f2e_align{$f_i}{$e_j}=1;
		    $e2f_align{$e_j}{$f_i}=1;
		}
	    }

#	    print STDERR "$extract_files[$i]:$line";
	    while(defined($line=$filehandles[$i]->getline) && $line!~/^<\/sent\_id/) {
		my($f,$e)=$line=~/^(.+?) \|\|\| (.+?) \|\|\| /o;
		my @tokens_f=split(/ /,$f);
		my @tokens_e=split(/ /,$e);

		if(($tokens_f[0]=~/^0:<s>/ && @tokens_f>1)
		   || ($tokens_e[0]=~/^0:<s>/ && @tokens_e>1)
		   || ($tokens_f[-1]=~/<\/s>$/ && @tokens_f>1)
		   || ($tokens_e[-1]=~/<\/s>$/ && @tokens_e>1)) {
		    next;
		}

		my($from_f)=$tokens_f[0]=~/^([0-9]+)\:/;
		my($to_f)=$tokens_f[-1]=~/^([0-9]+)\:/;
		my($from_e)=$tokens_e[0]=~/^([0-9]+)\:/;
		my($to_e)=$tokens_e[-1]=~/^([0-9]+)\:/;

		$counts{$from_e}{$to_e}{$from_f}{$to_f}++;
		$counts_inv{$to_e}{$from_e}{$from_f}{$to_f}++;
		my @words;
		for(my $j=0; $j<@tokens_f; $j++) {
		    my($word)=$tokens_f[$j]=~/^[0-9]+\:(.+)$/;
		    push(@words,$word);
		}
		$f_phrase{$from_f}{$to_f}=join(' ',@words);
		undef @words;
		for(my $j=0; $j<@tokens_e; $j++) {
		    my($word)=$tokens_e[$j]=~/^[0-9]+\:(.+)$/;
		    push(@words,$word);
		}
		$e_phrase{$from_e}{$to_e}=join(' ',@words);
	    }
	}
    }

    if($use_hdm) {
	for(my $i=0; $i<@filehandles_hdm; $i++) {
	    $lines_hdm[$i]=$filehandles_hdm[$i]->getline;

	    next if(!defined($lines_hdm[$i]));
	    my $line_hdm=$lines_hdm[$i];
	    if($line_hdm=~/^<sent/) {

		while(defined($line_hdm=$filehandles_hdm[$i]->getline) && $line_hdm!~/^<\/sent\_id/) {
		    my($f,$e)=$line_hdm=~/^(.+?) \|\|\| (.+?) \|\|\| /o;
		    my @tokens_f=split(/ /,$f);
		    my @tokens_e=split(/ /,$e);

		    if(($tokens_f[0]=~/^0:<s>/ && @tokens_f>1)
		       || ($tokens_e[0]=~/^0:<s>/ && @tokens_e>1)
		       || ($tokens_f[-1]=~/<\/s>$/ && @tokens_f>1)
		       || ($tokens_e[-1]=~/<\/s>$/ && @tokens_e>1)) {
			next;
		    }
		    my($from_f)=$tokens_f[0]=~/^([0-9]+)\:/;
		    my($to_f)=$tokens_f[-1]=~/^([0-9]+)\:/;
		    my($from_e)=$tokens_e[0]=~/^([0-9]+)\:/;
		    my($to_e)=$tokens_e[-1]=~/^([0-9]+)\:/;

		    $counts_hdm{$from_e}{$to_e}{$from_f}{$to_f}++;
		    $counts_inv_hdm{$to_e}{$from_e}{$from_f}{$to_f}++;

#		    my @words;
#		    for(my $j=0; $j<@tokens_f; $j++) {
#			my($word)=$tokens_f[$j]=~/^[0-9]+\:(.+)$/;
#			push(@words,$word);
#		    }
#		    $f_phrase{$from_f}{$to_f}=join(' ',@words);
#		    undef @words;
#		    for(my $j=0; $j<@tokens_e; $j++) {
#			my($word)=$tokens_e[$j]=~/^[0-9]+\:(.+)$/;
#			push(@words,$word);
#		    }
#		    $e_phrase{$from_e}{$to_e}=join(' ',@words);

		}
	    }
	}
    }

    my %orientation_counts;
    if($use_hdm) {
	&collect_orientation_counts(\%counts,\%counts_inv,\%counts_hdm,\%counts_inv_hdm,\%f_phrase,\%e_phrase,\%f2e_align,0,\%orientation_counts);
    } else {
	&collect_orientation_counts(\%counts,\%counts_inv,\%counts,\%counts_inv,\%f_phrase,\%e_phrase,\%f2e_align,$use_alignments,\%orientation_counts);
    }


    foreach my $fe_pair (sort (keys %orientation_counts)) {
	my($fe_pair_print)=$fe_pair=~/^\[[^\]]+\] (.+)$/;
#        my $fe_pair_print=$fe_pair;

	print "$fe_pair_print";

	if($moses_orientation) {
#	    my($fe_pair_print)=$fe_pair=~/^\[[^\]]+\] (.+)$/;
#	    print "$fe_pair_print";

	    for(my $i=0; $i<($no_orientations*2); $i++) {
		$orientation_counts{$fe_pair}[$i]=0 if(!defined($orientation_counts{$fe_pair}[$i]));
	    }


	    if($use_dlr) {
		# wrt previous phrase
		if($orientation_counts{$fe_pair}[0]>0
		   && $orientation_counts{$fe_pair}[1]==0
		   && $orientation_counts{$fe_pair}[2]==0
		   && $orientation_counts{$fe_pair}[3]==0) {
		    $orientation_counts{$fe_pair}[0]=1;
		} elsif($orientation_counts{$fe_pair}[1]>0
			&& $orientation_counts{$fe_pair}[0]==0
			&& $orientation_counts{$fe_pair}[2]==0
			&& $orientation_counts{$fe_pair}[3]==0) {
		    $orientation_counts{$fe_pair}[1]=1;
		} elsif($orientation_counts{$fe_pair}[0]>0
			|| $orientation_counts{$fe_pair}[1]>0
			|| $orientation_counts{$fe_pair}[2]>0
			|| $orientation_counts{$fe_pair}[3]>0) {
		    $orientation_counts{$fe_pair}[0]=0;
		    $orientation_counts{$fe_pair}[1]=0;
		    my $dl_count=sprintf("%.2f",$orientation_counts{$fe_pair}[2]/($orientation_counts{$fe_pair}[2]+$orientation_counts{$fe_pair}[3]));
		    $dl_count=~s/\.0+$//;
		    my $dr_count=sprintf("%.2f",$orientation_counts{$fe_pair}[3]/($orientation_counts{$fe_pair}[2]+$orientation_counts{$fe_pair}[3]));
		    $dr_count=~s/\.0+$//;
		    $orientation_counts{$fe_pair}[2]=$dl_count;
		    $orientation_counts{$fe_pair}[3]=$dr_count;
		}

		# wrt next phrase
		if($orientation_counts{$fe_pair}[0+$no_orientations]>0
		   && $orientation_counts{$fe_pair}[1+$no_orientations]==0
		   && $orientation_counts{$fe_pair}[2+$no_orientations]==0
		   && $orientation_counts{$fe_pair}[3+$no_orientations]==0) {
		    $orientation_counts{$fe_pair}[0+$no_orientations]=1;
		} elsif($orientation_counts{$fe_pair}[1+$no_orientations]>0
			&& $orientation_counts{$fe_pair}[0+$no_orientations]==0
			&& $orientation_counts{$fe_pair}[2+$no_orientations]==0
			&& $orientation_counts{$fe_pair}[3+$no_orientations]==0) {
		    $orientation_counts{$fe_pair}[1+$no_orientations]=1;
		} elsif($orientation_counts{$fe_pair}[0+$no_orientations]>0
			|| $orientation_counts{$fe_pair}[1+$no_orientations]>0
			|| $orientation_counts{$fe_pair}[2+$no_orientations]>0
			|| $orientation_counts{$fe_pair}[3+$no_orientations]>0) {
		    $orientation_counts{$fe_pair}[0+$no_orientations]=0;
		    $orientation_counts{$fe_pair}[1+$no_orientations]=0;

		    my $dl_count=sprintf("%.2f",$orientation_counts{$fe_pair}[2+$no_orientations]/($orientation_counts{$fe_pair}[2+$no_orientations]+$orientation_counts{$fe_pair}[3+$no_orientations]));
		    $dl_count=~s/\.0+$//;
		    my $dr_count=sprintf("%.2f",$orientation_counts{$fe_pair}[3+$no_orientations]/($orientation_counts{$fe_pair}[2+$no_orientations]+$orientation_counts{$fe_pair}[3+$no_orientations]));
		    $dr_count=~s/\.0+$//;
		    $orientation_counts{$fe_pair}[2+$no_orientations]=$dl_count;
		    $orientation_counts{$fe_pair}[3+$no_orientations]=$dr_count;
		}


	    } else {
		# wrt previous phrase
		if($orientation_counts{$fe_pair}[0]>0
		   && $orientation_counts{$fe_pair}[1]==0
		   && $orientation_counts{$fe_pair}[2]==0) {
		    $orientation_counts{$fe_pair}[0]=1;
		} elsif($orientation_counts{$fe_pair}[1]>0
			&& $orientation_counts{$fe_pair}[0]==0
			&& $orientation_counts{$fe_pair}[2]==0) {
		    $orientation_counts{$fe_pair}[1]=1;
		} elsif($orientation_counts{$fe_pair}[0]>0
			|| $orientation_counts{$fe_pair}[1]>0
			|| $orientation_counts{$fe_pair}[2]>0) {
		    $orientation_counts{$fe_pair}[0]=0;
		    $orientation_counts{$fe_pair}[1]=0;
		    $orientation_counts{$fe_pair}[2]=1;
		}

		# wrt next phrase
		if($orientation_counts{$fe_pair}[0+$no_orientations]>0
		   && $orientation_counts{$fe_pair}[1+$no_orientations]==0
		   && $orientation_counts{$fe_pair}[2+$no_orientations]==0) {
		    $orientation_counts{$fe_pair}[0+$no_orientations]=1;
		} elsif($orientation_counts{$fe_pair}[1+$no_orientations]>0
			&& $orientation_counts{$fe_pair}[0+$no_orientations]==0
			&& $orientation_counts{$fe_pair}[2+$no_orientations]==0) {
		    $orientation_counts{$fe_pair}[1+$no_orientations]=1;
		} elsif($orientation_counts{$fe_pair}[0+$no_orientations]>0
			|| $orientation_counts{$fe_pair}[1+$no_orientations]>0
			|| $orientation_counts{$fe_pair}[2+$no_orientations]>0) {
		    $orientation_counts{$fe_pair}[0+$no_orientations]=0;
		    $orientation_counts{$fe_pair}[1+$no_orientations]=0;
		    $orientation_counts{$fe_pair}[2+$no_orientations]=1;
		}
	    }

	    for(my $i=0; $i<($no_orientations*2); $i++) {
		print ' ', $orientation_counts{$fe_pair}[$i];
	    }
	    print "\n";
	} else {
#	    print "$fe_pair";
	    for(my $i=0; $i<($no_orientations*2); $i++) {
		$orientation_counts{$fe_pair}[$i]=0 if(!defined($orientation_counts{$fe_pair}[$i]));
		print ' ', $orientation_counts{$fe_pair}[$i];
	    }
	    print "\n";
	}
    }

    $finished=1;
    for(my $i=0; $i<@lines; $i++) {
	if(defined($lines[$i])) {
	    $finished=0;
	    last;
	}
    }
}



sub max {
    return $_[0] if($_[0]>$_[1]);
    return $_[1];
}

sub collect_orientation_counts {
    my($counts,$counts_inv,$counts_neighbor,$counts_inv_neighbor,$f_phrase,$e_phrase,$f2e_align,$use_alignments,$orientation_counts)=@_;

    foreach my $from_e (keys %$counts) {
	foreach my $to_e (keys %{ $$counts{$from_e} }) {
	    my $next_from_e=$to_e+1;

	    if(exists($$counts_neighbor{$next_from_e})) {
		foreach my $next_to_e (keys %{ $$counts_neighbor{$next_from_e} }) {
		    foreach my $from_f (keys %{ $$counts{$from_e}{$to_e} }) {
			foreach my $to_f (keys %{ $$counts{$from_e}{$to_e}{$from_f} }) {
			    foreach my $next_from_f (keys %{ $$counts_neighbor{$next_from_e}{$next_to_e} }) {
				foreach my $next_to_f (keys %{ $$counts_neighbor{$next_from_e}{$next_to_e}{$next_from_f} }) {

				    if(($next_from_f<=$from_f && $from_f<=$next_to_f)
				       || ($next_from_f<=$to_f && $to_f<=$next_to_f)
				       || ($from_f<=$next_from_f && $next_from_f<=$to_f)
				       || ($from_f<=$next_to_f && $next_to_f<=$to_f)) {
					next;
				    }

				    my $freq=&max($$counts{$from_e}{$to_e}{$from_f}{$to_f},
						  $$counts_neighbor{$next_from_e}{$next_to_e}{$next_from_f}{$next_to_f});

				    my $fe_pair;
				    my $fe_pair_next;
				    $fe_pair="[$from_f,$to_f,$from_e,$to_e] $$f_phrase{$from_f}{$to_f} ||| $$e_phrase{$from_e}{$to_e} |||";


				    if($use_alignments) {
					if($next_from_f==$to_f+1
					   && exists($$f2e_align{$next_from_f})
					   && exists($$f2e_align{$next_from_f}{$next_from_e})) {
					    $$orientation_counts{$fe_pair}[0+$no_orientations]+=$freq;
					} elsif($next_to_f==$from_f-1
						&& exists($$f2e_align{$next_to_f})
						&& exists($$f2e_align{$next_to_f}{$next_from_e})) {
					    $$orientation_counts{$fe_pair}[1+$no_orientations]+=$freq;
					} else {
					    if($use_dlr) {
						if($next_from_f>$to_f) {
						    $$orientation_counts{$fe_pair}[3+$no_orientations]+=$freq;
						} else {
						    $$orientation_counts{$fe_pair}[2+$no_orientations]+=$freq;
						}
					    } else {
						$$orientation_counts{$fe_pair}[2+$no_orientations]+=$freq;
					    }
					}
				    } else {
					if($next_from_f==$to_f+1) {
					    $$orientation_counts{$fe_pair}[0+$no_orientations]+=$freq;
					} elsif($next_to_f==$from_f-1) {
					    $$orientation_counts{$fe_pair}[1+$no_orientations]+=$freq;
					} else {
					    if($use_dlr) {
						if($next_from_f>$to_f) {
						    $$orientation_counts{$fe_pair}[3+$no_orientations]+=$freq;
						} else {
						    $$orientation_counts{$fe_pair}[2+$no_orientations]+=$freq;
						}
					    } else {
						$$orientation_counts{$fe_pair}[2+$no_orientations]+=$freq;
					    }
					}
				    }
				}
			    }
			}
		    }
		}
	    }

	    my $prev_to_e=$from_e-1;
	    next if(!exists($$counts_inv_neighbor{$prev_to_e}));
	    foreach my $prev_from_e (keys %{ $$counts_inv_neighbor{$prev_to_e} }) {
		foreach my $from_f (keys %{ $$counts{$from_e}{$to_e} }) {
		    foreach my $to_f (keys %{ $$counts{$from_e}{$to_e}{$from_f} }) {
			foreach my $prev_from_f (keys %{ $$counts_inv_neighbor{$prev_to_e}{$prev_from_e} }) {
			    foreach my $prev_to_f (keys %{ $$counts_inv_neighbor{$prev_to_e}{$prev_from_e}{$prev_from_f} }) {

				if(($prev_from_f<=$from_f && $from_f<=$prev_to_f)
				   || ($prev_from_f<=$to_f && $to_f<=$prev_to_f)
				   || ($from_f<=$prev_from_f && $prev_from_f<=$to_f)
				   || ($from_f<=$prev_to_f && $prev_to_f<=$to_f)) {
				    next;
				}

				my $freq=&max($$counts{$from_e}{$to_e}{$from_f}{$to_f},
					      $$counts_inv_neighbor{$prev_to_e}{$prev_from_e}{$prev_from_f}{$prev_to_f});

				my $fe_pair;
				$fe_pair="[$from_f,$to_f,$from_e,$to_e] $$f_phrase{$from_f}{$to_f} ||| $$e_phrase{$from_e}{$to_e} |||";

				if($use_alignments) {
				    if($prev_to_f==$from_f-1
				       && exists($$f2e_align{$prev_to_f})
				       && exists($$f2e_align{$prev_to_f}{$prev_to_e})) {
					$$orientation_counts{$fe_pair}[0]+=$freq;
				    } elsif($prev_from_f==$to_f+1
					    && exists($$f2e_align{$prev_from_f})
					    && exists($$f2e_align{$prev_from_f}{$prev_to_e})) {
					$$orientation_counts{$fe_pair}[1]+=$freq;
				    } else {
					if($use_dlr) {
					    if($prev_from_f>$to_f) {
						$$orientation_counts{$fe_pair}[2]+=$freq;
					    } else {
						$$orientation_counts{$fe_pair}[3]+=$freq;
					    }
					} else {
					    $$orientation_counts{$fe_pair}[2]+=$freq;
					}
				    }
				} else {
				    if($prev_to_f==$from_f-1) {
					$$orientation_counts{$fe_pair}[0]+=$freq;
				    } elsif($prev_from_f==$to_f+1) {
					$$orientation_counts{$fe_pair}[1]+=$freq;
				    } else {
					if($use_dlr) {
					    if($prev_from_f>$to_f) {
						$$orientation_counts{$fe_pair}[2]+=$freq;
					    } else {
						$$orientation_counts{$fe_pair}[3]+=$freq;
					    }
					} else {
					    $$orientation_counts{$fe_pair}[2]+=$freq;
					}
				    }
				}
			    }
			}
		    }
		}
	    }
	}
    }
}
