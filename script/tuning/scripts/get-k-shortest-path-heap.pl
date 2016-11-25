#!/usr/bin/perl -w

use strict;
use warnings;

use PerlIO::gzip;
use Time::HiRes;

use POSIX;

my($lattice_file,$from_sent,$to_sent,$k)=@ARGV;

BEGIN {
    if ( !defined( $ENV{'OOISTERHOME'} )
        || $ENV{'OOISTERHOME'} eq '' )
    {
        print STDERR "environment variable OOISTERHOME must be set:\n";
        print STDERR "export OOISTERHOME=/path/to/ooister/\n";
        exit(-1);
    }
    my $OOISTERHOME = $ENV{'OOISTERHOME'};

    unshift( @INC, "$OOISTERHOME/src" );
}
use Heap;


our $bool_debugParameters=0;



my $neg_inf=-1000;
my $pos_inf=1000;

my $heap_min_inf=-1*10**10;
my $heap_inf=10**10;

$k||=100;

if($lattice_file=~/\.gz/) {
    open FILE, "<:gzip", $lattice_file, or die("can't open $lattice_file: $!\n");
} else {
    open(FILE,"<$lattice_file")||die("can't open file $lattice_file: $!\n");
}

my $real_startTime = [ Time::HiRes::gettimeofday() ];

my $no_lines_processed=0;
my $prev_no_lines=0;
while(defined(my $line=<FILE>)) {

     if($line=~/<SENT ID=([0-9]+)>/) {
	my $sent_id=$1;

        my %graph_edge;
        my %graph_label;
        my %vertices_from;
        my %vertices_to;

        if(defined($from_sent) && defined($to_sent)) {
            if($sent_id<$from_sent) {
                while(defined($line=<FILE>) && $line!~/^<\/SENT>/) {
		}
		next;
            } elsif($sent_id>$to_sent) {
                last;
            }
        }

	$no_lines_processed++;
	my $no_digits = rindex($prev_no_lines,"");
	if($no_lines_processed % 1 == 0) {
	    for(my $j=1; $j<=$no_digits; $j++) {
		print STDERR "\x08";
	    };
	    print STDERR "$no_lines_processed";
	    $prev_no_lines = $no_lines_processed;
	};


        while(defined($line=<FILE>) && $line!~/^<\/SENT>/) {
            chomp($line);

            if($line=~/^<COVERVECS>/) {
                my($string)=$line=~/<COVERVECS>(.*)<\/COVERVECS>/;
                my @pairs=split(/ /,$string);
                for(my $i=0; $i<@pairs; $i++) {
                    my($node_pair,$left,$right)=split(/:/,$pairs[$i]);
            	    my($node_to,$node_from)=split(/\-/,$node_pair);
                    $graph_label{$node_from}{$node_to}[0]="$left\-$right";

                }
            } else {
                my($node_to,$list)=split(/\t/,$line);
                my @pairs;
                while($list=~s/([0-9]+\|\|\|[^\|]*\|\|\|(?:[0-9\.\-e]+|inf))//) {
                    my $edge=$1;
                    push(@pairs,$edge);
                }
                for(my $i=0; $i<@pairs; $i++) {
                    my($node_from,$translation,$cost)=split(/\|\|\|/,$pairs[$i]);
                    if($cost eq 'inf') {
                        $cost=$pos_inf;
                    } elsif($cost eq '-inf') {
                        $cost=$neg_inf;
                    }

                    $cost=-1*$cost;
                    $graph_edge{$node_from}{$node_to}=$cost;
                    $translation||='';
                    $graph_label{$node_from}{$node_to}[1]=$translation;
                    $vertices_from{$node_from}=1;
                    $vertices_to{$node_to}=1;
                }
            }
        }

        my($start_node,$end_node)=&find_start_and_end_nodes(\%vertices_from,\%vertices_to);

        my @k_paths;
        &yen_ksp(\%graph_edge,$start_node,$end_node,$k,\@k_paths);

        for(my $i=0; $i<@k_paths; $i++) {
            my @path=split(/ /,$k_paths[$i]);
            my $derivation=&get_derivation(\%graph_edge,\%graph_label,\@path);
            my($cost)=$derivation=~/^([^\t]+)\t/;
#            print "k=", $i+1, " cost=$cost\n";
        }

     }
}

my $real_timeDiff = Time::HiRes::tv_interval($real_startTime);
print STDERR "\n\nProcessing took ", sprintf( "%.4f", $real_timeDiff ), " seconds\n\n";



sub yen_ksp {
    my($graph_edge,$start_node,$end_node,$k_max,$k_paths)=@_;

    my @A;

    my @A_rootPaths;

    my @A_cost;
    my @A_spurIndex;

    my %B_rootPath;
    my %B_completionNode;
    my %B_cost;
    my %B_spurIndex;

    my $heap_resize=$k_max*4;

    my $path_id=0;
#    my @heap_keys;
#    my @heap_values;
#    my $heap_size=0;
#    my $heap_max_value=$heap_min_inf;

    my $obj_heap=Heap->new( type=>'min', sizeLimit=>$k_max );


    my $inf=1000000000;


    my %graph_edge_inv;
    foreach my $node_from (keys %$graph_edge) {
        foreach my $node_to (keys %{ $$graph_edge{$node_from} }) {
            $graph_edge_inv{$node_to}{$node_from}=$$graph_edge{$node_from}{$node_to};
        }
    }

    my %shortest_path_tree_inv_weight;
    my %shortest_path_tree_successor;
    my $shortest_path_string=&invert_path_string(&bellman_ford(\%graph_edge_inv,$end_node,$start_node,\%shortest_path_tree_inv_weight,\%shortest_path_tree_successor));
    undef %graph_edge_inv;

#    print STDERR "shortest_path_string=$shortest_path_string\n";

    if($shortest_path_string eq '') {
    } else {
        my @path=split(/ /,$shortest_path_string);
        push(@A,[ @path ]);
        my $cost=&get_path_cost($graph_edge,\@path);
        push(@A_cost,$cost);
        push(@A_spurIndex,-1);

        for(my $i=0; $i<@path-1; $i++) {
            my $string=join(' ',@path[0..$i]);
            $A_rootPaths[$i]{$string}{$path[$i+1]}=1;
        }
    }

    my $exhausted=0;
    for(my $k=1; $k<$k_max && !$exhausted; $k++) {

        my $k_left=$k_max-$k;

        if($obj_heap->size()>0 && $obj_heap->top_value()==$A_cost[$k-1]) {
#        if(@heap_keys>0 && &heap_top_value(\@heap_keys,\@heap_values)==$A_cost[$k-1]) {
            my($min_key,$min_value)=$obj_heap->withdraw();
#            my($min_key,$min_value)=&heap_extract_min(\@heap_keys,\@heap_values);
#            $heap_size--;
            my $b_cost=$B_cost{$min_key};
            my $b_completionPathString=&get_completion_path(\%shortest_path_tree_successor,$B_completionNode{$min_key});
            my $b_pathString="$B_rootPath{$min_key} $b_completionPathString";
            my $b_spurIndex=$B_spurIndex{$min_key};

            my @path=split(/ /,$b_pathString);

            push(@A,[ @path ]);
            push(@A_cost,$b_cost);
            push(@A_spurIndex,$b_spurIndex);

            for(my $i=0; $i<@path-1; $i++) {
                my $string=join(' ',@path[0..$i]);
                $A_rootPaths[$i]{$string}{$path[$i+1]}=1;
            }
            next;
        }

        $obj_heap->adjust_limit( sizeLimit=>$k_left );
 
        my $rootPathEndIndex=$A_spurIndex[$k-1];
        my @rootPath=();
        my $rootPathCost=0;
        if($rootPathEndIndex>=0) {
            @rootPath=@{ $A[$k-1] }[0..$rootPathEndIndex];
            $rootPathCost=&get_path_cost($graph_edge,\@rootPath);
        }

        for(my $i=$A_spurIndex[$k-1]+1; $i<@{ $A[$k-1] }-1; $i++) {
            my %graph_edge_removed;
            my %vertices_removed;

            my $spurNode=$A[$k-1][$i];
#            my @rootPath=@{ $A[$k-1] }[0..$i];
            push(@rootPath,$spurNode);
            if($i>0) {
                $rootPathCost+=$$graph_edge{$A[$k-1][$i-1]}{$spurNode};
            }

#            my $rootPathCost=&get_path_cost($graph_edge,\@rootPath);

            my $rootPathString=join(' ',@rootPath);
            for(my $j=0; $j<@rootPath-1; $j++) {
                $vertices_removed{$rootPath[$j]}=1;
            }

            if(exists($A_rootPaths[$i]{$rootPathString})) {
                foreach my $node (keys %{ $A_rootPaths[$i]{$rootPathString} }) {
                    $graph_edge_removed{$spurNode}{$node}=1;
                }
            }


            my $min_completion_cost=$inf;
            my $min_completion_cost_arg;

            my %completion_nodes;
            foreach my $node_to (keys %{ $$graph_edge{$spurNode} }) {
                if(!exists($graph_edge_removed{$spurNode}) || !exists($graph_edge_removed{$spurNode}{$node_to})) {                    
                    $completion_nodes{$node_to}=$shortest_path_tree_inv_weight{$node_to}+$$graph_edge{$spurNode}{$node_to};
                }
            }

            foreach my $node (keys %completion_nodes) {

#                my $completion_path=&get_completion_path(\%shortest_path_tree_successor,$node);
#                my $totalPathString="$rootPathString $completion_path";
#                my @totalPath=split(/ /,$totalPathString);

                my $cost=$rootPathCost+$completion_nodes{$node};
#                if($heap_size<$k_left || $cost<$heap_max_value) {
#                if($obj_heap->size()<$k_left || $cost<$heap_max_value) {
                if(1) {

                    $path_id++;
#                    $B_rootPath{$path_id}=$rootPathString;
#                    $B_completionNode{$path_id}=$node;
#                    $B_cost{$path_id}=$cost;
#                    $B_spurIndex{$path_id}=$i;

#                    &min_heap_insert(\@heap_keys,\@heap_values,$path_id,$B_cost{$path_id});
                    my $bool_added=$obj_heap->add($path_id,$cost);
                    if($bool_added) {
                        $B_rootPath{$path_id}=$rootPathString;
                        $B_completionNode{$path_id}=$node;
                        $B_cost{$path_id}=$cost;
                        $B_spurIndex{$path_id}=$i;
                    } else {
                        $path_id--;
                    }          


#                    $heap_size++;
#                    if($cost>$heap_max_value) {
#                        $heap_max_value=$cost;
#                    }
#                    print STDERR "heap_size=$heap_size\n";
                }
                
            }
        }


#        if(@heap_keys>0) {
        if($obj_heap->size()>0) {
#            my($min_key,$min_value)=&heap_extract_min(\@heap_keys,\@heap_values);
            my($min_key,$min_value)=$obj_heap->withdraw();

#            $heap_size--;

            my $b_cost=$B_cost{$min_key};
            my $b_completionPathString=&get_completion_path(\%shortest_path_tree_successor,$B_completionNode{$min_key});
            my $b_pathString="$B_rootPath{$min_key} $b_completionPathString";
#            my $b_pathString=$B_path{$min_key};

            my $b_spurIndex=$B_spurIndex{$min_key};

            my @path=split(/ /,$b_pathString);

            push(@A,[ @path ]);
            push(@A_cost,$b_cost);
            push(@A_spurIndex,$b_spurIndex);
            for(my $i=0; $i<@path-1; $i++) {
                my $string=join(' ',@path[0..$i]);
                $A_rootPaths[$i]{$string}{$path[$i+1]}=1;
            }
        } else {
            $exhausted=1;
        }
    }
    
    for(my $i=0; $i<@A; $i++) {
        push(@$k_paths,join(' ',@{ $A[$i] }));
    }

}


        
sub find_start_and_end_nodes {
    my($vertices_from,$vertices_to)=@_;

    my @start_nodes;
    my @end_nodes;
    foreach my $node (keys %$vertices_from) {
        if(!exists($$vertices_to{$node})) {
            push(@start_nodes,$node);
        }
    }
    foreach my $node (keys %$vertices_to) {
        if(!exists($$vertices_from{$node})) {
            push(@end_nodes,$node);
        }
    }

    my $fail=0;
    if(@start_nodes>1) {
        print STDERR "Error: There is more than one start-node: ", join(', ',@start_nodes), "\n";
        $fail=1;
    }
    if(@end_nodes>1) {
        print STDERR "Error: There is more than one end-node: ", join(', ',@end_nodes), "\n";
        $fail=1;
    }

    if($fail) {
        exit(-1);
    } else {
        return ($start_nodes[0],$end_nodes[0]);
    }
}


sub bellman_ford {
    my($graph_edge,$start_node,$end_node,$shortest_path_tree_weight,$shortest_path_tree_predecessor)=@_;

    my $inf=1000000000;

    # initialize graph:

    my %weight;
    my %weight_change_previous;

    $$shortest_path_tree_weight{$start_node}=0;
    $weight_change_previous{$start_node}=1;
    my %predecessor;
    $$shortest_path_tree_predecessor{$start_node}=-1;


    my @queue=( $start_node );
    my %visited;
    $visited{$start_node}=1;
    $visited{$end_node}=1;


    my %edge;
    my %vertices_num_incoming_edges;

    my $num_iterations=0;
    while(@queue>0) {
        my $node_from=shift(@queue);

        foreach my $node_to (keys %{ $$graph_edge{$node_from} }) {
            $$shortest_path_tree_weight{$node_to}=$inf;
            $vertices_num_incoming_edges{$node_to}++;
            if(!exists($visited{$node_to})) {
                push(@queue,$node_to);
                $visited{$node_to}=1;
                $num_iterations++;                
            }
        }
    }
    undef %visited;

    my @vertices_topo_sorted;
    &topological_sort($graph_edge,\%vertices_num_incoming_edges,$start_node,\@vertices_topo_sorted);

    # relax edges:
    for(my $i=0; $i<@vertices_topo_sorted-1; $i++) {
        my $node_from=$vertices_topo_sorted[$i];

        foreach my $node_to (keys %{ $$graph_edge{$node_from} }) {
            my $sum=$$shortest_path_tree_weight{$node_from} + $$graph_edge{$node_from}{$node_to};
            if($sum < $$shortest_path_tree_weight{$node_to}) {
                $$shortest_path_tree_weight{$node_to}=$sum;
                $$shortest_path_tree_predecessor{$node_to}=$node_from;
            }
        }
    }

    # read-out path

    if(!exists($$shortest_path_tree_predecessor{$end_node})) {
        return '';
    } else {
        my @path=( $end_node );
        my $predecessor_node=$$shortest_path_tree_predecessor{$end_node};
        while($predecessor_node>=0) {
            unshift(@path,$predecessor_node);
            $predecessor_node=$$shortest_path_tree_predecessor{$predecessor_node};
        }
        return join(' ',@path);
    }
}

sub get_path_cost {
    my($graph_edge,$path)=@_;

    my $sum=0;
    for(my $i=0; $i<@$path-1; $i++) {
        $sum+=$$graph_edge{$path->[$i]}{$path->[$i+1]};
    }
    return $sum;
}

sub get_derivation {
    my($graph_edge,$graph_label,$path)=@_;

    my @derivation;
    my $sum=0;
    for(my $i=0; $i<@$path-2; $i++) {
        my $node_from=$path->[$i];
        my $node_to=$path->[$i+1];
        my $label=join(' ',@{ $$graph_label{$node_from}{$node_to} });
        my $cost=-1*$$graph_edge{$node_from}{$node_to};
        push(@derivation,"$label:$cost");
        $sum+=$cost;
    }
    return "$sum\t" . join(' ',@derivation);
}


sub combine_paths {
    my($path_prefix,$path_suffix)=@_;

    if($path_prefix->[-1] != $path_suffix->[0]) {
        print STDERR "Error\n";
        exit(-1);
    }

    my @path=@$path_prefix;
    for(my $i=1; $i<@$path_suffix; $i++) {
        push(@path,$path_suffix->[$i]);
    }

    return join(' ',@path);
}


sub topological_sort {
    my($edge,$vertices_num_incoming_edges,$start_node,$sorted_list)=@_;

    my @S=( $start_node );
    
    while(@S>0) {
        my $node_n=pop(@S);
        push(@$sorted_list,$node_n);

        foreach my $node_m (keys %{ $$edge{$node_n} }) {
            $$vertices_num_incoming_edges{$node_m}--;
            if($$vertices_num_incoming_edges{$node_m}==0) {
                push(@S,$node_m);
            }
        }
    }        
}


sub invert_path_string {
    my($string)=@_;

    return '' if(!defined($string) || $string eq '');

    my(@tokens)=split(/ /,$string);
    my @tokens_inv;
    for(my $i=@tokens-1; $i>=0; $i--) {
        push(@tokens_inv,$tokens[$i]);
    }
    return join(' ',@tokens_inv);
}


sub get_completion_path {
    my($shortest_path_tree_successor,$start_node)=@_;
    
    my @path=( $start_node );
    my $successor_node=$$shortest_path_tree_successor{$start_node};
    while($successor_node>=0) {
        push(@path,$successor_node);
        $successor_node=$$shortest_path_tree_successor{$successor_node};
    }
    return join(' ',@path);
}





