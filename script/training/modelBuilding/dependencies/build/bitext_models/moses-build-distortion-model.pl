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

my($orientation_file,$no_orientations,$smoothing_factor,$language_parameter)=@ARGV;

$smoothing_factor||=0.75;
$language_parameter||='fe';

$language_parameter='fe' if($language_parameter eq 'ef');

if($orientation_file=~/\.gz$/o) {
    open F, "<:gzip", $orientation_file, or die("can't open $orientation_file: $!\n");
} else {
    open(F,"<$orientation_file") || die("can't open $orientation_file: $!\n");
}

my @global_orientations;
while(defined(my $line=<F>)) {
    my($fe_pair,$freq_string)=$line=~/^(.+? \|\|\| .+? \|\|\| )(.+?)\n/o;

    if($language_parameter eq 'f') {
	my($f,$e)=split(/ \|\|\| /o,$fe_pair);
	$e='_NIL_';
	$fe_pair="$f ||| $e ||| ";
    } elsif($language_parameter eq 'e') {
	my($f,$e)=split(/ \|\|\| /o,$fe_pair);
	$f='_NIL_';
	$fe_pair="$f ||| $e ||| ";
    }

    my @pair_orientations=split(/ /,$freq_string);
    for(my $i=0; $i<@pair_orientations; $i++) {
	$global_orientations[$i]+=$pair_orientations[$i];
    }
}
close(F);
my $prev_global=0;
my $next_global=0;

for(my $i=0; $i<$no_orientations; $i++) {
    $prev_global+=$global_orientations[$i];
}
for(my $i=$no_orientations; $i<($no_orientations*2); $i++) {
    $next_global+=$global_orientations[$i];
}


for(my $i=0; $i<$no_orientations; $i++) {
    if($prev_global>0) {
	$global_orientations[$i]/=$prev_global;
    } else {
	$global_orientations[$i]=0;
    }
}
for(my $i=$no_orientations; $i<($no_orientations*2); $i++) {
    if($next_global>0) {
	$global_orientations[$i]/=$next_global;
    } else {
	$global_orientations[$i]=0;
    }
}

if($language_parameter eq 'fe') {
    print "UNK ||| UNK ||| ";
} elsif($language_parameter eq 'f') {
    print "UNK ||| \_NIL\_ ||| ";
} elsif($language_parameter eq 'e') {
    print "\_NIL\_ ||| UNK ||| ";
}

my @print_global=@global_orientations;
for(my $i=0; $i<@global_orientations; $i++) {
    $print_global[$i]=sprintf("%.5f",$global_orientations[$i]);
}
print join(' ',@print_global), "\n";


if($orientation_file=~/\.gz$/o) {
    open F, "<:gzip", $orientation_file, or die("can't open $orientation_file: $!\n");
} else {
    open(F,"<$orientation_file") || die("can't open $orientation_file: $!\n");
}
my $prev_fe_pair;
my @orientations;
while(defined(my $line=<F>)) {
    my($fe_pair,$freq_string)=$line=~/^(.+? \|\|\| .+? \|\|\| )(.+?)\n/o;

    if($language_parameter eq 'f') {
	my($f,$e)=split(/ \|\|\| /o,$fe_pair);
	$e='_NIL_';
	$fe_pair="$f ||| $e ||| ";
    } elsif($language_parameter eq 'e') {
	my($f,$e)=split(/ \|\|\| /o,$fe_pair);
	$f='_NIL_';
	$fe_pair="$f ||| $e ||| ";
    }

    my @pair_orientations=split(/ /,$freq_string);
    if(defined($prev_fe_pair) && $fe_pair ne $prev_fe_pair) {

	my $prev_total=0;
	my $next_total=0;
	for(my $i=0; $i<$no_orientations; $i++) {
	    $prev_total+=$orientations[$i];
	}
	for(my $i=$no_orientations; $i<($no_orientations*2); $i++) {
	    $next_total+=$orientations[$i];
	}

#	print "PAIR=$prev_fe_pair", join(' ',@orientations), " prev_total=$prev_total next_total=$next_total\n";

	for(my $i=0; $i<$no_orientations; $i++) {
	    $orientations[$i]=($smoothing_factor*$global_orientations[$i]+$orientations[$i])
		/($smoothing_factor+$prev_total);
	    $orientations[$i]=sprintf("%.5f",$orientations[$i]);
	}
	for(my $i=$no_orientations; $i<($no_orientations*2); $i++) {
	    $orientations[$i]=($smoothing_factor*$global_orientations[$i]+$orientations[$i])
		/($smoothing_factor+$next_total);
	    $orientations[$i]=sprintf("%.5f",$orientations[$i]);
	}

	print "$prev_fe_pair", join(' ',@orientations), "\n";

	for(my $i=0; $i<@orientations; $i++) {
	    $orientations[$i]=$pair_orientations[$i];
	}

    } else {
	for(my $i=0; $i<@pair_orientations; $i++) {
	    $orientations[$i]+=$pair_orientations[$i];
	}
    }
    $prev_fe_pair=$fe_pair;

}
close(F);

my $prev_total=0;
my $next_total=0;
for(my $i=0; $i<$no_orientations; $i++) {
    $prev_total+=$orientations[$i];
}
for(my $i=$no_orientations; $i<($no_orientations*2); $i++) {
    $next_total+=$orientations[$i];
}

for(my $i=0; $i<$no_orientations; $i++) {
    $orientations[$i]=($smoothing_factor*$global_orientations[$i]+$orientations[$i])
	/($smoothing_factor+$prev_total);
    $orientations[$i]=sprintf("%.5f",$orientations[$i]);
}
for(my $i=$no_orientations; $i<($no_orientations*2); $i++) {
    $orientations[$i]=($smoothing_factor*$global_orientations[$i]+$orientations[$i])
	/($smoothing_factor+$next_total);
    $orientations[$i]=sprintf("%.5f",$orientations[$i]);
}

print "$prev_fe_pair", join(' ',@orientations), "\n";

for(my $i=0; $i<@orientations; $i++) {
    $orientations[$i]=0;
}

