#!/usr/bin/perl


use strict;
use warnings;
use PerlIO::gzip;
use POSIX;
use File::Basename;

if(@ARGV<4||@ARGV>5) {
    print STDERR "\nusage: $0 <LATTICE-FILE> <FEATURE-SCORES-FILE> <FEATURE-ID2NAME-FILE> <\#N-BEST> [<external-path>]\n\n";
    exit(-1);
};

my $script_name = basename($0);
print STDERR "$script_name pid=$$\n";

my($lattice_file,$feature_scores_file,$feature_id2name_file,$nbest)=@ARGV;

my $neg_inf=-1000;
my $pos_inf=1000;

my $heap_min_inf=-1*10**10;
my $heap_inf=10**10;



my $from_sent;
my $to_sent;

my $out_file;
my $scored_file;

if($lattice_file=~/\:([0-9]+)\-([0-9]+)$/) {
    $from_sent=$1;
    $to_sent=$2;
    $lattice_file=~s/\:([0-9]+)\-([0-9]+)$//;
}

if(defined($from_sent) && defined($to_sent)) {
    $out_file="$lattice_file." . "nbest.$from_sent\-$to_sent";
    $scored_file="$lattice_file." . "nbest.$from_sent\-$to_sent.rescored";
} else {
    $out_file="$lattice_file." . "nbest";
    $scored_file="$lattice_file." . "nbest.rescored";
}

my %feature_id2name;
open(F,"<$feature_id2name_file")||die("can't open file $feature_id2name_file: $!\n");
while(defined(my $line=<F>)) {
    chomp($line);
    my($feature_id,$feature_name)=split(/\t/,$line);
    $feature_id2name{$feature_id}=$feature_name;
}
close(F);

if($feature_scores_file=~/\.gz/) {
    open FEAT, "<:gzip", $feature_scores_file, or die("can't open $feature_scores_file: $!\n");
} else {
    open(FEAT,"<$feature_scores_file")||die("can't open file $feature_scores_file: $!\n");
}

if($lattice_file=~/\.gz/) {
    open FILE, "<:gzip", $lattice_file, or die("can't open $lattice_file: $!\n");
} else {
    open(FILE,"<$lattice_file")||die("can't open file $lattice_file: $!\n");
}

open(OUT,">$out_file") || die("can't open $out_file: $!\n");
open(SCORED,">$scored_file") ||die("can't open file $scored_file: $!\n");

my $no_lines_processed=0;
my $prev_no_lines=0;
while(defined(my $line=<FILE>)) {

     if($line=~/<SENT ID=([0-9]+)>/) {
        my $sent_id=$1;

        my %node_feature_scores;

        if(defined($from_sent) && defined($to_sent)) {
            if($sent_id<$from_sent) {
                while(defined($line=<FILE>) && $line!~/^<\/SENT>/) {
                }
                next;
            } elsif($sent_id>$to_sent) {
                last;
            }
        }

        my $found_feat=0;
        while(!$found_feat && defined(my $line_feat=<FEAT>)) {
            if($line_feat=~/^<SENT ID=$sent_id>/) {
            $found_feat=1;
            while(defined($line_feat=<FEAT>) && $line_feat!~/^<\/SENT>/) {
                chomp($line_feat);
                my($node_id,@feature_values)=split(/ /,$line_feat);
                for(my $i=0; $i<@feature_values; $i++) {
                    my($feature_id,$feature_value)=split(/\=/,$feature_values[$i]);
                    $node_feature_scores{$node_id}{$feature_id}=$feature_value;
                }
            }
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

        my %graph_edge;
        my %graph_label;
        my %vertices_from;
        my %vertices_to;

        while(defined($line=<FILE>) && $line!~/^<\/SENT>/) {
            chomp($line);

            if($line=~/^<COVERVECS>/) {
                my($string)=$line=~/<COVERVECS>(.*)<\/COVERVECS>/;
                my @pairs=split(/ /,$string);
                for(my $i=0; $i<@pairs; $i++) {
                    my($node_pair,$left,$right)=split(/:/,$pairs[$i]);
                        my($node_to,$node_from)=split(/\-/,$node_pair);
                    $graph_label{$node_from}{$node_to}[0]="$left:$right";

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
        &yen_ksp(\%graph_edge,$start_node,$end_node,$nbest,\@k_paths);

        print OUT "<SENT ID=$sent_id>\n";

        for(my $i=0; $i<@k_paths; $i++) {
            my $rank=$i+1;

            my @path=split(/ /,$k_paths[$i]);
            my $cost_derivation=&get_derivation(\%graph_edge,\%graph_label,\@path);
            my($cost,$derivation_string)=split(/\t/,$cost_derivation);
            my $trans_string=&get_translation(\%graph_edge,\%graph_label,\@path);
            print OUT "rank=$rank cost=$cost trans=$trans_string coversets=$derivation_string\n";

            my %feature_scores_total;
            for(my $i=1; $i<@path-1; $i++) {
                my $node_id=$path[$i];
                if(exists($node_feature_scores{$node_id})) {
                    foreach my $feature_id (keys %{ $node_feature_scores{$node_id} }) {
                        $feature_scores_total{$feature_id}+=$node_feature_scores{$node_id}{$feature_id};
                    }
                }
            }
            my @feature_scores;
            foreach my $feature_id (sort {$a<=>$b} (keys %feature_id2name)) {
                if(exists($feature_scores_total{$feature_id}) && $feature_scores_total{$feature_id}!=0) {
                    push(@feature_scores,"$feature_id\=$feature_scores_total{$feature_id}");
                }
            }
            print SCORED "$sent_id ||| $trans_string ||| ", join(' ',@feature_scores), "\n";

        }
        print OUT "<\/SENT>\n";

     }

}

close(F);
close(FEAT);
close(SCORED);


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
    my @heap_keys;
    my @heap_values;
    my $heap_size=0;
    my $heap_max_value=$heap_min_inf;

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

        if(@heap_keys>0 && &heap_top_value(\@heap_keys,\@heap_values)==$A_cost[$k-1]) {
            my($min_key,$min_value)=&heap_extract_min(\@heap_keys,\@heap_values);
            $heap_size--;
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
            push(@rootPath,$spurNode);
            if($i>0) {
                $rootPathCost+=$$graph_edge{$A[$k-1][$i-1]}{$spurNode};
            }

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

                my $cost=$rootPathCost+$completion_nodes{$node};
                if($heap_size<$k_left || $cost<$heap_max_value) {

                    $path_id++;
                    $B_rootPath{$path_id}=$rootPathString;
                    $B_completionNode{$path_id}=$node;
                    $B_cost{$path_id}=$cost;
                    $B_spurIndex{$path_id}=$i;

                    &min_heap_insert(\@heap_keys,\@heap_values,$path_id,$B_cost{$path_id});
                    $heap_size++;
                    if($cost>$heap_max_value) {
                        $heap_max_value=$cost;
                    }
                }

            }
        }


        if(@heap_keys>0) {
            my($min_key,$min_value)=&heap_extract_min(\@heap_keys,\@heap_values);
            $heap_size--;
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
        } else {
            $exhausted=1;
        }

        if($heap_size>$heap_resize) {
            my @heap_keys_new;
            my @heap_values_new;
            my $heap_size_new;
            my $heap_max_value_new;

            for(my $i=0; $i<$k_left; $i++) {

                my($min_key,$min_value)=&heap_extract_min(\@heap_keys,\@heap_values);
                push(@heap_keys_new,$min_key);
                push(@heap_values_new,$min_value);
                $heap_size_new++;
                $heap_max_value_new=$min_value;
            }

            for(my $i=0; $i<@heap_keys; $i++) {
                delete($B_rootPath{$heap_keys[$i]});
                delete($B_completionNode{$heap_keys[$i]});
                delete($B_cost{$heap_keys[$i]});
                delete($B_spurIndex{$heap_keys[$i]});
            }

            &build_min_heap(\@heap_keys_new,\@heap_values_new,0);


            undef @heap_keys;
            undef @heap_values;

            @heap_keys=@heap_keys_new;
            @heap_values=@heap_values_new;
            $heap_size=$heap_size_new;
            $heap_max_value=$heap_max_value_new;
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
        push(@derivation,$label);
        $sum+=$cost;
    }
    return "$sum\t" . join('|||',@derivation);
}

sub get_translation {
    my($graph_edge,$graph_label,$path)=@_;

    my @derivation;
    for(my $i=0; $i<@$path-2; $i++) {
        my $node_from=$path->[$i];
        my $node_to=$path->[$i+1];
        my $label=$$graph_label{$node_from}{$node_to}[1];
        push(@derivation,$label);
    }
    return join(' ',@derivation);
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



################################## HEAP FUNCTIONS ##################################



sub min_heap_insert {
    my($heap_keys,$heap_values,$key,$value)=@_;
    my $heap_values_size=@$heap_values-1;
    $heap_keys->[$heap_values_size+1]=$key;
    $heap_values->[$heap_values_size+1]=$heap_inf;
    &heap_decrease_key($heap_keys,$heap_values,$heap_values_size+1,$value);
}


sub heap_decrease_key {
    my($heap_keys,$heap_values,$i,$value)=@_;

    $heap_values->[$i]=$value;
    while($i>0 && $heap_values->[&heap_parent($i)]>$heap_values->[$i]) {
        &swap($heap_keys,$heap_values,$i,&heap_parent($i));
        $i=&heap_parent($i);
    }
}


sub heap_extract_min {
    my($heap_keys,$heap_values)=@_;

    if(@$heap_values==0) {
        return 'error: heap underflow';
    }

    my $min_key=$heap_keys->[0];
    my $min_value=$heap_values->[0];
    my $heap_values_size=@$heap_values-1;

    $heap_keys->[0]=$heap_keys->[$heap_values_size];
    $heap_values->[0]=$heap_values->[$heap_values_size];
    pop(@$heap_keys);
    pop(@$heap_values);
    &min_heapify($heap_keys,$heap_values,0);
    return ($min_key,$min_value);
}



sub heap_top_value {
    my($heap_keys,$heap_values)=@_;
    return $heap_values->[0];
}

sub heap_top_key {
    my($heap_keys,$heap_values)=@_;
    return $heap_keys->[0];
}


sub build_min_heap {
    my($heap_keys,$heap_values)=@_;
    my $heap_values_size=@$heap_values-1;

    for(my $i=floor($heap_values_size/2); $i>=0; $i--) {
        &min_heapify($heap_keys,$heap_values,$i);
    }
}


sub min_heapify {
    my($heap_keys,$heap_values,$i)=@_;
    my $l=&heap_left($i);
    my $r=&heap_right($i);
    my $heap_values_size=@$heap_values-1;
    my $smallest;

    if($l<=$heap_values_size && $heap_values->[$l]<$heap_values->[$i]) {
        $smallest=$l;
    } else {
        $smallest=$i;
    }

    if($r<=$heap_values_size && $heap_values->[$r]<$heap_values->[$smallest]) {
        $smallest=$r;
    }

    if($smallest!=$i) {
        &swap($heap_keys,$heap_values,$i,$smallest);
        &min_heapify($heap_keys,$heap_values,$smallest);
    }
}


sub swap {
    my($heap_keys,$heap_values,$i,$j)=@_;
    my $tmp=$heap_keys->[$i];
    $heap_keys->[$i]=$heap_keys->[$j];
    $heap_keys->[$j]=$tmp;
    $tmp=$heap_values->[$i];
    $heap_values->[$i]=$heap_values->[$j];
    $heap_values->[$j]=$tmp;
}

sub swap_anom {
   my $tmp=$_[0]->[$_[2]];
   $_[0]->[$_[2]]=$_[0]->[$_[3]];
   $_[0]->[$_[3]]=$tmp;
   $tmp=$_[1]->[$_[2]];
   $_[1]->[$_[2]]=$_[1]->[$_[3]];
   $_[1]->[$_[3]]=$tmp;
}


sub heap_parent {
    return floor(($_[0]-1)/2);
}

sub heap_left {
    return 2*$_[0]+1;
}

sub heap_right {
    return 2*$_[0]+2;
}
