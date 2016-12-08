#!/usr/bin/perl -w

if(@ARGV<4||@ARGV>5) {
    print STDERR "\nusage: $0 <LATTICE-FILE> <FEATURE-SCORES-FILE> <FEATURE-ID2NAME-FILE> <\#N-BEST> [<external-path>]\n\n";
    exit(-1);
};

print STDERR "get-nbest-carmel.pl pid=$$\n";

#$e=2.718281828459;

($lattice_file,$feature_scores_file,$feature_id2name_file,$nbest,$external_path)=@ARGV;
$tmp_file="/tmp/tmp.$$"; 
$source_id=0;
$min_inf=-10000000;

#$eppstein="/nfshomes/christof/software/EPPSTEIN-1.1-double/EPPSTEIN";
#$carmel="/fs/clip2/Programs/carmel-3.0/carmel/bin/carmel";
#$carmel="/homes/christof/resources/software/carmel/graehl/carmel/bin/carmel";


if(!defined($external_path)
   && exists($ENV{'OISTEREXTPATH'}) && defined($ENV{'OISTEREXTPATH'})) {
    $external_path=$ENV{'OISTEREXTPATH'};
} elsif(!defined($external_path)) {
    print STDERR "external_path must be set\n";
    exit(-1);
}

my $carmel;
#BEGIN { 
#   my $OISTERHOME=$ENV{'OISTERHOME'};
#   $carmel="$OISTERHOME/resources/software/carmel/carmel"; 
#}

$carmel="$external_path/external_binaries/carmel";

$out_file="$lattice_file." . "nbest";
$scored_file="$lattice_file." . "nbest.rescored";

my %feature_id2name;
open(F,"<$feature_id2name_file")||die("can't open file $feature_id2name_file: $!\n");
while(defined(my $line=<F>)) {
    chomp($line);
    my($feature_id,$feature_name)=split(/\t/,$line);
    $feature_id2name{$feature_id}=$feature_name;
}
close(F);

#$sent=0;
$no_lines_processed=0;
$prev_no_lines=0;
my %node_feature_scores;
open(FILE,"<$lattice_file") || die("can't open $lattice_file: $!\n");
open(FEAT,"<$feature_scores_file") || die("can't open $feature_scores_file: $!\n");
open(OUT,">$out_file") || die("can't open $out_file: $!\n");
open(SCORED,">$scored_file") ||die("can't open file $scored_file: $!\n");
while(defined($line=<FILE>)) {
   
     if(0 && $line=~/<SENT ID=([0-9]+)>/) {
	 $sent_id=$1;
	 if($sent_id!=367) {
	     while(defined($line=<FILE>) && $line!~/<\/SENT>/) {
	     };
	     next;
	 };
     }

     if($line=~/<SENT ID=([0-9]+)>/) {
	$sent_id=$1;
	$std_id=1;

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

#	$std2dc{0}=0;
#	$dc2std{0}=0;


	$no_lines_processed++;
	$no_digits = rindex($prev_no_lines,"");
	if($no_lines_processed % 1 == 0) {
	    for($j=1; $j<=$no_digits; $j++) {
		print STDERR "\x08";
	    };
	    print STDERR "$no_lines_processed";
	    $prev_no_lines = $no_lines_processed;
	};

	undef %dc2std;
	undef %std2dc;
	undef @buffer;
	undef %trans;

	$std2dc{0}=0;
	$dc2std{0}=0;



        $line=<FILE>;
        chomp($line);

	if($line=~/<COVERVECS><\/COVERVECS>/) {
	    while(defined($line=<FILE>) && $line!~/<\/SENT>/) {
	    }
	    print OUT "<SENT ID=$sent_id>\n";
	    print OUT "rank=1 cost=0 trans=NIL coversets=0:0 NIL\n";
	    print OUT "</SENT>\n";
	    next;	    
	}

        ($final_node,$list)=split(/\t/,$line);

	$std_id=$final_node;

	$std2dc{$std_id}=$final_node;
	$dc2std{$final_node}=$std_id;

#        $std2dc{$final_node}=$final_node;
#        $dc2std{$final_node}=$final_node;

	$trans[$std_id]="";
	$std_final=$std_id;
	$std_id++;
	undef @pairs;
	while($list=~s/([0-9]+\|\|\|[^\|]*\|\|\|(?:[0-9\.\-e]+|inf))//) {
	    $edge=$1;
	    push(@pairs,$edge);
	};
        for($i=0; $i<@pairs; $i++) {
            ($node,$translation,$cost)=split(/\|\|\|/,$pairs[$i]);
	    if($cost eq 'inf') {
		$cost=1000;
	    }
	    if(!defined($dc2std{$node})) {
		$std_id=$node;
		$dc2std{$node}=$std_id;
		$std2dc{$std_id}=$node;
		$std_id++;
	    };
#	    $cost=-1*$cost;
#	    $cost=~s/(\-[0-9]+\.[0-9][0-9][0-9][0-9]).*$/$1/;
#	    $cost=$e**$cost;
	    $cost=exp($cost);
	    if(!defined($translation)) {
		$translation="";
	    };
	    $trans{$dc2std{$node}}{$dc2std{$final_node}}=$translation;
	    $translation=~s/\"/\$QUOTE\$/g;
	    $translation=~s/\\/\$BACKSLASH\$/g;
	    if($node==0) {
	      unshift(@buffer,"\(0 \($dc2std{$final_node} \"$translation\" $cost\)\)\n");
#	      unshift(@buffer,"\(0 \($dc2std{$final_node} \"$dc2std{$final_node}\_$translation\" $cost\)\)\n");
	    } else {
	      push(@buffer,"\($dc2std{$node} \($dc2std{$final_node} \"$translation\" $cost\)\)\n");
#	      push(@buffer,"\($dc2std{$node} \($dc2std{$final_node} \"$dc2std{$node}\-$dc2std{$final_node}\_$translation\" $cost\)\)\n");
	    };
	};
   } elsif($line=~/<\/SENT>/) {
#	$highest_id=$std_id-1;
#	$no_edges=@buffer;
	$std_source_id=$dc2std{$source_id};
	$trans[$std_source_id]="";
	if(0 && -e $tmp_file) {
#	    unlink($tmp_file);
	};

#	$tmp_file="tmp/tmp.$$.$sent_id"; 

	open(TMP,">$tmp_file") || die("can't open $tmp_file: $!\n");
#	print TMP "n $highest_id\n";
#	print TMP "m $no_edges\n";
#	print TMP "s $std_source_id\n";
	print TMP "$std_final\n";
	for($i=0; $i<@buffer; $i++) {
	    print TMP $buffer[$i];
	};
	close(TMP);
	system("$carmel -Z $tmp_file -k $nbest 1> $tmp_file.carmel 2> /dev/null");

#	if($sent_id==15) {
#	    system("cp $tmp_file $tmp_file.15");
#	};
	
#	$tmp_file="tmp.$$.$sent_id"; 
	open(TMP,"<$tmp_file.carmel") || die("can't open $tmp_file.carmel: $!\n");
	print OUT "<SENT ID=$sent_id>\n";
	$rank=0;
	while(defined($line=<TMP>)) {
	    next if($line=~/^0/);
	    ($total_cost)=$line=~/ ([^ ]+)\n/;
#	    print STDERR "TOTAL: $total_cost\n";
	    if($total_cost=~/ln$/) {
		$total_cost=~s/ln$//;
            } elsif($total_cost=~/e\^/) {
                $total_cost=~s/e\^//;
	    } elsif($total_cost==0) {
		$total_cost=$min_inf;
 	    } else {
		$total_cost=log($total_cost);
	    }
	    $rank++;
	    undef @seq;
	    push(@seq,0);		
	    while($line=~s/\(([0-9]+) -> ([0-9]+) \"(.*?)\" : \".*?\" \/ ([0-9\.\-e\^]+)\)//) {
#		print STDERR "LALAL\n";
		$from=$1;
		$to=$2;		
		$translation=$3;
		$translation=~s/\$QUOTE\$/\"/g;
		$translation=~s/\$BACKSLASH\$/\\/g;

		$trans{$from}{$to}=$translation;
		$cost=$4;
		push(@seq,$to);
#		print STDERR "TO: $to\n";
	    };
	    $trans_string="";
	    for($i=1; $i<@seq; $i++) {
		if(!defined($trans{$seq[$i-1]}{$seq[$i]})) {
		    print STDERR "NOT DEF: $seq[$i-1] $seq[$i]\n";
		};
		$trans_string.="$trans{$seq[$i-1]}{$seq[$i]} ";
	    };
	    $trans_string=~s/\$QUOTE\$/\"/g;
	    $trans_string=~s/\$BACKSLASH\$/\\/g;
	    $trans_string=~s/ +/ /g;
	    $trans_string=~s/ $//;
	    $trans_string=~s/^ //;
	    print OUT "rank=$rank cost=$total_cost trans=$trans_string";
	    $cover_string="";
	    for($i=1; $i<@seq-1; $i++) {
		if(!defined($trans{$seq[$i-1]}{$seq[$i]})) {
		    print STDERR "NOT DEF: $seq[$i-1] $seq[$i]\n";
		};

#		if($seq[$i-1]==91 && $seq[$i]==942) {
#		    print STDERR "covers{$seq[$i-1]}{$seq[$i]}=$covers{$seq[$i-1]}{$seq[$i]} trans{$seq[$i-1]}{$seq[$i]}=$trans{$seq[$i-1]}{$seq[$i]}\n";
#		}

		#changed for coversets:
#		$cover_string.="$covers{$seq[$i]} $trans{$seq[$i-1]}{$seq[$i]}|||";
		if(!defined($covers{$seq[$i-1]}{$seq[$i]})) {
		    print STDERR "covers{$seq[$i-1]}{$seq[$i]}=$covers{$seq[$i-1]}{$seq[$i]}\n";
		    print STDERR "trans{$seq[$i-1]}{$seq[$i]}=$trans{$seq[$i-1]}{$seq[$i]}\n";
		    print STDERR "seq[$i-1]=", $seq[$i-1], "seq[$i]=$seq[$i]\n";
		}
		$cover_string.="$covers{$seq[$i-1]}{$seq[$i]} $trans{$seq[$i-1]}{$seq[$i]}|||";
	    };
	    $cover_string=~s/\|\|\|$//;
	    print OUT " coversets=$cover_string\n";

	    my %feature_scores_total;
	    for($i=1; $i<@seq-1; $i++) {
		my $node_id=$seq[$i];
		if(exists($node_feature_scores{$node_id})) {
		    foreach $feature_id (keys %{ $node_feature_scores{$node_id} }) {
			$feature_scores_total{$feature_id}+=$node_feature_scores{$node_id}{$feature_id};
		    }
		}
	    }
	    my @feature_scores;
	    foreach my $feature_id (sort {$a<=>$b} (keys %feature_id2name)) {
		if(exists($feature_scores_total{$feature_id}) && $feature_scores_total{$feature_id}!=0) {
		    push(@feature_scores,"$feature_id\=$feature_scores_total{$feature_id}");
#		} else {
#		    push(@feature_scores,0);
		}
	    }
	    print SCORED "$sent_id ||| $trans_string ||| ", join(' ',@feature_scores), "\n";	
	};

	close(TMP);

	
	

	unlink($tmp_file);
	unlink("$tmp_file.carmel");

	undef %node_feature_scores;
	print OUT "</SENT>\n";
#	die;

    } elsif($line=~/<COVERVECS>/) {
	($string)=$line=~/<COVERVECS>(.*)<\/COVERVECS>/;
	undef @pairs;
	@pairs=split(/ /,$string);
	undef %covers;
	for($i=0; $i<@pairs; $i++) {
#	    ($node,$left,$right)=split(/:/,$pairs[$i]);
#	    $covers{$dc2std{$node}}="$left:$right";
	    #changed for coversets:
	    ($node_pair,$left,$right)=split(/:/,$pairs[$i]);
#	    if($node_pair eq '942-91') {
#		print STDERR "pairs[$i]=$pairs[$i]\n";
#	    }
	    ($node,$prev_node)=split(/\-/,$node_pair);
	    if(!defined($dc2std{$prev_node})) {
		next;
		$dc2std{$prev_node}=$std_id;
		$std2dc{$std_id}=$prev_node;
		$std_id++;
#		print STDERR "NOT-DEF: sent=$sent_id node_pair=$node_pair prev_node=$prev_node\n";
	    }
	    if(!defined($dc2std{$node})) {
		next;
		$dc2std{$node}=$std_id;
		$std2dc{$std_id}=$node;
		$std_id++;
#		print STDERR "NOT-DEF: sent=$sent_id node_pair=$node_pair prev_node=$prev_node\n";
	    }

	    $covers{$dc2std{$prev_node}}{$dc2std{$node}}="$left:$right";	    
#	    if($prev_node==91 && $node==942) {
#		print STDERR "covers{$prev_node}{$node}=$covers{$dc2std{$prev_node}}{$dc2std{$node}}\n";
#		print STDERR "covers{$dc2std{$prev_node}}{$dc2std{$node}}=$covers{$dc2std{$prev_node}}{$dc2std{$node}}\n";
#	    }
#	    if($dc2std{$prev_node}==91 && $dc2std{$node}==942) {
#		print STDERR "STD:covers{$prev_node}{$node}=$covers{$dc2std{$prev_node}}{$dc2std{$node}}\n";
#		print STDERR "STD:covers{$dc2std{$prev_node}}{$dc2std{$node}}=$covers{$dc2std{$prev_node}}{$dc2std{$node}}\n";
#	    }		
	}
    } else {
	chomp($line);
        ($right_node,$list)=split(/\t/,$line);
	if(!defined($dc2std{$right_node})) {
	    $std_id=$right_node;
	    $dc2std{$right_node}=$std_id;
	    $std2dc{$std_id}=$right_node;
	    $std_id++;
	};
        undef @pairs;
	
	while($list=~s/([0-9]+\|\|\|[^\|]*\|\|\|(?:[0-9\.\-e]+|inf))//) {
	    $edge=$1;
	    push(@pairs,$edge);
	};
        for($i=0; $i<@pairs; $i++) {
            ($node,$translation,$cost)=split(/\|\|\|/,$pairs[$i]);
	    if($cost eq 'inf') {
		$cost=1000;
	    }

	    if(!defined($dc2std{$node})) {
		$std_id=$node;
		$dc2std{$node}=$std_id;
		$std2dc{$std_id}=$node;
		$std_id++;
	    };
#	    $cost=-1*$cost;
#	    $cost=~s/(\-[0-9]+\.[0-9][0-9][0-9][0-9]).*$/$1/;
#	    $cost=$e**$cost;
	    $cost=exp($cost);
	    if(!defined($translation)) {
		$translation="";
	    };

	    $trans{$dc2std{$node}}{$dc2std{$right_node}}=$translation;
	    $translation=~s/\"/\$QUOTE\$/g;
	    $translation=~s/\\/\$BACKSLASH\$/g;
	    if($node==0) {
	      unshift(@buffer,"\(0 \($std2dc{$right_node} \"$translation\" $cost\)\)\n");
#	      unshift(@buffer,"\(0 \($std2dc{$right_node} \"0\-$std2dc{$right_node}\_$translation\" $cost\)\)\n");
	    } else {
	      push(@buffer,"\($dc2std{$node} \($dc2std{$right_node} \"$translation\" $cost\)\)\n");
#	      push(@buffer,"\($dc2std{$node} \($dc2std{$right_node} \"$dc2std{$node}-$dc2std{$right_node}\_$translation\" $cost\)\)\n");
	    };
        };
    };
};
close(FILE);
close(FEAT);

#system("touch \"$lattice_file.done\"");
