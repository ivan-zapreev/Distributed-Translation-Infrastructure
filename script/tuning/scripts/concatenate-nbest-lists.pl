#!/usr/bin/perl -w

use strict;
use warnings;

my(@nbest_files)=@ARGV;

my $offset=0;
for(my $i=0; $i<@nbest_files; $i++) {
    my $max_sent_id;
    open(F,"<$nbest_files[$i]")||die("can't open $nbest_files[$i]: $!\n");
    while(defined(my $line=<F>)) {
	if($line=~/^<SENT ID=([0-9]+)>\n/) {
	    my $sent_id=$1;
	    $sent_id=$sent_id+$offset;
	    if(!defined($max_sent_id)||$sent_id>$max_sent_id) {
		$max_sent_id=$sent_id;
	    }
	    print "<SENT ID=$sent_id>\n";
	} else {
	    print $line;
	}
    }
    close(F);
    $offset=$max_sent_id+1;
}

