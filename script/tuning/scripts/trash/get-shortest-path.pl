#!/usr/bin/perl -w

use strict;
use warnings;

use PerlIO::gzip;
use Time::HiRes;

my($lattice_file,$from_sent,$to_sent,$k)=@ARGV;


my $neg_inf=-20;
my $pos_inf=20;

$k||=100;

if($lattice_file=~/\.gz/) {
    open FILE, "<:gzip", $lattice_file, or die("can't open $lattice_file: $!\n");
} else {
    open(FILE,"<$lattice_file")||die("can't open file $lattice_file: $!\n");
}

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

                    $graph_edge{$node_from}{$node_to}=-1*$cost;
                    $translation||='';
                    $graph_label{$node_from}{$node_to}[1]=$translation;
                    $vertices_from{$node_from}=1;
                    $vertices_to{$node_to}=1;
                }
            }
        }

        my($start_node,$end_node)=&find_start_and_end_nodes(\%vertices_from,\%vertices_to);
#        print STDERR "start=$start_node end=$end_node\n";


        my $real_startTime = [ Time::HiRes::gettimeofday() ];

        my @k_paths;
        &yen_ksp(\%graph_edge,$start_node,$end_node,$k,\@k_paths);
        my $real_timeDiff = Time::HiRes::tv_interval($real_startTime);

        for(my $i=0; $i<@k_paths; $i++) {
            my @path=split(/ /,$k_paths[$i]);
            my $derivation=&get_derivation(\%graph_edge,\%graph_label,\@path);
            my($cost)=$derivation=~/^([^\t]+)\t/;
            print "k=", $i+1, " cost=$cost\n";
#            print "derivation=$derivation\n";
        }
        print STDERR "\n\nProcessing took ", sprintf( "%.4f", $real_timeDiff ), " seconds\n\n";

#        my $shortest_path_string=&bellman_ford(\%graph_edge,$start_node,$end_node);
#        print "shortest_path=$shortest_path_string\n";
#        my @path=split(/ /,$shortest_path_string);
#        my $derivation=&get_derivation(\%graph_edge,\%graph_label,\@path);
#        print "derivation=$derivation\n";
     }
}


sub yen_ksp {
    my($graph_edge,$start_node,$end_node,$k_max,$k_paths)=@_;

    my @A;
    my %B;
    my %B_spurIndex;

    my @A_rootPaths;

    my @A_cost;
    my @A_spurIndex;

    my @B_sorted_path;
    my @B_sorted_cost;
    my @B_sorted_spurIndex;

    my $shortest_path_string=&bellman_ford($graph_edge,$start_node,$end_node);
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

        if(@B_sorted_cost>0 && $B_sorted_cost[0]==$A_cost[$k-1]) {
            my $b_cost=shift(@B_sorted_cost);
            my $b_pathString=shift(@B_sorted_path);
            my $b_spurIndex=shift(@B_sorted_spurIndex);

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
            

        for(my $i=$A_spurIndex[$k-1]+1; $i<@{ $A[$k-1] }-1; $i++) {
            my %graph_edge_removed;
            my %vertices_removed;

            my $spurNode=$A[$k-1][$i];
            my @rootPath=@{ $A[$k-1] }[0..$i];
            my $rootPathString=join(' ',@rootPath);
            for(my $j=0; $j<@rootPath-1; $j++) {
                $vertices_removed{$rootPath[$j]}=1;
            }

            if(exists($A_rootPaths[$i]{$rootPathString})) {
                foreach my $node (keys %{ $A_rootPaths[$i]{$rootPathString} }) {
                    $graph_edge_removed{$spurNode}{$node}=1;
                }
            }

            my $spurPathString=&bellman_ford($graph_edge,$spurNode,$end_node,\%vertices_removed,\%graph_edge_removed);
            if($spurPathString ne '') {
                my @spurPath=split(/ /,$spurPathString);
                my $totalPathString=&combine_paths(\@rootPath,\@spurPath);            
                my @totalPath=split(/ /,$totalPathString);
                if(!exists($B{$totalPathString})) {
                    $B{$totalPathString}=&get_path_cost($graph_edge,\@totalPath);
                    $B_spurIndex{$totalPathString}=$i;
                }
            }
        }

        undef @B_sorted_path;
        undef @B_sorted_cost;
        undef @B_sorted_spurIndex;

        foreach my $path (sort { $B{$a}<=>$B{$b} } (keys %B)) {
            push(@B_sorted_path,$path);
            push(@B_sorted_cost,$B{$path});
            push(@B_sorted_spurIndex,$B_spurIndex{$path});            
        }

        if(@B_sorted_cost>0) {
            my $b_cost=shift(@B_sorted_cost);
            my $b_pathString=shift(@B_sorted_path);
            my $b_spurIndex=shift(@B_sorted_spurIndex);

            my @path=split(/ /,$b_pathString);

            push(@A,[ @path ]);
            push(@A_cost,$b_cost);
            push(@A_spurIndex,$b_spurIndex);
            for(my $i=0; $i<@path-1; $i++) {
                my $string=join(' ',@path[0..$i]);
                $A_rootPaths[$i]{$string}{$path[$i+1]}=1;
            }

            delete($B{$b_pathString});
            delete($B_spurIndex{$b_pathString});
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
    my($graph_edge,$start_node,$end_node,$vertices_removed,$graph_edge_removed)=@_;

    my $inf=1000000000;

    # initialize graph:

    my %weight;
    my %weight_change_previous;

    $weight{$start_node}=0;
    $weight_change_previous{$start_node}=1;
    my %predecessor;
    $predecessor{$start_node}=-1;


    my @queue=( $start_node );
    my %visited;
    $visited{$start_node}=1;
    $visited{$end_node}=1;

#    foreach my $node (keys %$vertices_removed) {
#        $visited{$node}=1;
#    }

    my %edge;
    my %vertices_num_incoming_edges;

    my $num_iterations=0;
    while(@queue>0) {
        my $node_from=shift(@queue);

        my $remove_possible=0;
        if(exists($$graph_edge_removed{$node_from})) {
            $remove_possible=1;
        }

        foreach my $node_to (keys %{ $$graph_edge{$node_from} }) {
            if($remove_possible &&  exists($$graph_edge_removed{$node_from}{$node_to})) {
            } else {
                $weight{$node_to}=$inf;
#                $edge{$node_from}{$node_to}=$$graph_edge{$node_from}{$node_to};
                $vertices_num_incoming_edges{$node_to}++;
                if(!exists($visited{$node_to})) {
                    push(@queue,$node_to);
                    $visited{$node_to}=1;
                    $num_iterations++;                
                }
            }
        }
    }
    undef %visited;

    my @vertices_topo_sorted;
    &topological_sort($graph_edge,\%vertices_num_incoming_edges,$graph_edge_removed,$start_node,\@vertices_topo_sorted);

    # relax edges:
    for(my $i=0; $i<@vertices_topo_sorted-1; $i++) {
        my $node_from=$vertices_topo_sorted[$i];

        my $remove_possible=0;
        if(exists($$graph_edge_removed{$node_from})) {
            $remove_possible=1;
        }
        foreach my $node_to (keys %{ $$graph_edge{$node_from} }) {
            if($remove_possible &&  exists($$graph_edge_removed{$node_from}{$node_to})) {
            } else {
                my $sum=$weight{$node_from} + $$graph_edge{$node_from}{$node_to};
                if($sum < $weight{$node_to}) {
                    $weight{$node_to}=$sum;
                    $predecessor{$node_to}=$node_from;
                }
            }
        }
    }


    if(0) {
    # relax edges:
    my $relaxation=1;
    for(my $i=0; $i<$num_iterations && $relaxation; $i++) {
        $relaxation=0;
        my %weight_change_next;

        foreach my $node_from (keys %edge) {
            if(exists($weight_change_previous{$node_from})) {
                foreach my $node_to (keys %{ $edge{$node_from} }) {
                    if($weight{$node_from} + $edge{$node_from}{$node_to} < $weight{$node_to}) {
                        $weight{$node_to}=$weight{$node_from} + $edge{$node_from}{$node_to};
                        $predecessor{$node_to}=$node_from;
                        $relaxation=1;
                        $weight_change_next{$node_to}=1;
                    }
                }
            }
        }
        undef %weight_change_previous;
        if($relaxation) {
            foreach my $node (keys %weight_change_next) {
                $weight_change_previous{$node}=1;
            }
            $weight_change_previous{$start_node}=1;
        } 

        if($i==$num_iterations-1 || $relaxation) {
            print STDERR "num iterations=$i\n";
        }
    }
    }

    # read-out path

    if(!exists($predecessor{$end_node})) {
        return '';
    } else {
        my @path=( $end_node );
        my $predecessor_node=$predecessor{$end_node};
        while($predecessor_node>=0) {
            unshift(@path,$predecessor_node);
            $predecessor_node=$predecessor{$predecessor_node};
        }
        return join(' ',@path);
    }
}

sub get_path_cost {
    my($graph_edge,$path)=@_;

    my $sum=0;
    for(my $i=0; $i<@$path-2; $i++) {
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
    my($edge,$vertices_num_incoming_edges,$graph_edge_removed,$start_node,$sorted_list)=@_;

    my @S=( $start_node );
    
    while(@S>0) {
        my $node_n=pop(@S);
        push(@$sorted_list,$node_n);

        my $remove_possible=0;
        if(exists($$graph_edge_removed{$node_n})) {
            $remove_possible=1;
        }
        foreach my $node_m (keys %{ $$edge{$node_n} }) {
            if($remove_possible &&  exists($$graph_edge_removed{$node_n}{$node_m})) {
            } else {
                $$vertices_num_incoming_edges{$node_m}--;
                if($$vertices_num_incoming_edges{$node_m}==0) {
                    push(@S,$node_m);
                }
            }
        }
    }        
}
