#!/usr/bin/perl -w

use strict;
use warnings;

my($lattice_file)=@ARGV;


open(F,"<$lattice_file")||die("can't open file $lattice_file: $!\n");
open(C,">$lattice_file.clean")||die("can't open file $lattice_file.clean: $!\n");
    
while(defined(my $line=<F>)) {
    if($line=~/^<SENT ID=([0-9]+)>/) {
	my $sent_id=$1;
	my $first_line=1;
	my $start_node=0;
	my $end_node;
	
	my $covervecs_line;
	
	my %edge_trans;
	my %edge_cost;
	my %edge_inv;
	my %edge_span;
	my %nodes;
	
	print C $line;
	
	while(defined($line=<F>) && $line!~/^<\/SENT>/) {
	    if($line=~/^<COVERVECS>/) {
		$covervecs_line=$line;
		$line=~s/^<COVERVECS>//;
		$line=~s/<\/COVERVECS>\n//;
		my @spans=split(/ /,$line);
		for(my $i=0; $i<@spans; $i++) {
		    my($node_to,$node_from,$src_from,$src_to)=$spans[$i]=~/^([0-9]+)\-([0-9]+)\:([0-9]+)\:([0-9]+)$/;
		    $nodes{$node_from}=1;
		    $nodes{$node_to}=1;
		    $edge_span{$node_to}{$node_from}="$src_from:$src_to";
		}
	    } else {
		if($first_line) {
		    $first_line=0;
		    my($node_to,$from_string)=$line=~/^([0-9]+)\t(.+)\n/;
		    $end_node=$node_to;
		    my @from_tuples=split(/ /,$from_string);
		    for(my $i=0; $i<@from_tuples; $i++) {
			my($node_from,$cost)=split(/\|\|\|\|\|\|/o,$from_tuples[$i]);
			$nodes{$node_from}=1;
			$nodes{$node_to}=1;
			$edge_trans{$node_from}{$node_to}='';
			$edge_cost{$node_from}{$node_to}=$cost;
			$edge_inv{$node_to}{$node_from}=1;
		    }
		} else {
		    my($node_to,$from_string)=$line=~/^([0-9]+)\t(.+)\n/;
		    $from_string.=' ';
		    while($from_string=~s/([0-9]+)\|\|\|([^\|]+)\|\|\|([0-9\-\.e]+) / /) {
			my $node_from=$1;
			my $trans=$2;
			my $cost=$3;
			
			$nodes{$node_from}=1;
			$nodes{$node_to}=1;
			$edge_trans{$node_from}{$node_to}=$trans;
			$edge_cost{$node_from}{$node_to}=$cost;
			$edge_inv{$node_to}{$node_from}=1;
		    }
		}
	    }
	}
	
	my %visited;
	my @queue=( $start_node );
	while(@queue>0) {
	    my %queue_next;
	    for(my $i=0; $i<@queue; $i++) {
		my $node_from=$queue[$i];
		foreach my $node_to (keys %{ $edge_trans{$node_from} }) {
		    $queue_next{$node_to}=1;
		}
		$visited{$node_from}=1;
	    }
	    undef @queue;
	    foreach my $node_to (keys %queue_next) {
		if(!exists($visited{$node_to})) {
		    push(@queue,$node_to);
		}
	    }
	}
	
	
	my %visited_back;
	undef @queue;
	if(exists($visited{$end_node})) {
	    @queue=( $end_node );
	}
	while(@queue>0) {
	    my %queue_next;
	    for(my $i=0; $i<@queue; $i++) {
		my $node_to=$queue[$i];
		my @print_parts;
		foreach my $node_from (keys %{ $edge_inv{$node_to} }) {
		    if(exists($visited{$node_from})) {
			push(@print_parts,"$node_from\|\|\|$edge_trans{$node_from}{$node_to}\|\|\|$edge_cost{$node_from}{$node_to}");
			$queue_next{$node_from}=1;
		    }
		}
		if(@print_parts>0) {
		    print C "$node_to\t", join(' ',@print_parts), "\n";
		} 
		$visited_back{$node_to}=1;
	    }
	    undef @queue;
	    foreach my $node_from (keys %queue_next) {
		if(!exists($visited_back{$node_from})) {
		    push(@queue,$node_from);
		}
	    }
	}
	
	my @cover_parts;
	foreach my $node_to (keys %edge_span) {
	    if(!exists($visited{$node_to}) || !exists($visited_back{$node_to})) {
#               print STDERR "removed node=$node_to\n";
		next;
	    }
	    foreach my $node_from (keys %{ $edge_span{$node_to} }) {
		if(!exists($visited{$node_from}) || !exists($visited_back{$node_from})) {
#                   print STDERR "removed node=$node_from\n";
		    next;
		}
		push(@cover_parts,"$node_to\-$node_from:$edge_span{$node_to}{$node_from}");
	    }
	}
	print C "<COVERVECS>", join(' ',@cover_parts), "<\/COVERVECS>\n";
	print C "<\/SENT>\n";
    }
}
close(F);
close(C);

#print STDERR "mv $lattice_file.clean $lattice_file\n";
#system("mv $lattice_file.clean $lattice_file");

