#!/usr/bin/perl -w


use strict;
use warnings;
use Getopt::Long "GetOptions";

my $err_logs;
my $config_file;
my $old_config_file;
my $new_config_file;
my $normalize_weights=0;
my $print_new=0;
my $select_strategy='last';
my $sparse_feature_files_string='';

GetOptions(
    "conf=s" => \$config_file,
    "err=s" => \$err_logs,
    "in=s"  => \$old_config_file,
    "out=s" => \$new_config_file,
    "print" => \$print_new,
    "select=s" => \$select_strategy,
    "sparse-features=s" => \$sparse_feature_files_string,
    "norm-weights|normalize-weights|norm|normalize" => \$normalize_weights
    );


my @sparse_feature_files=split(/\,/,$sparse_feature_files_string);


my @decoding_times;
my @avg_decoding_times;
my @min_decoding_times;
my @max_decoding_times;

my @rescoring_times;
my @avg_rescoring_times;
my @min_rescoring_times;
my @max_rescoring_times;

my $avg_total_decoding_time=0;
my $avg_total_rescoring_time=0;
  

my $config_file_print="$config_file.$select_strategy";


my @feature_names;
my @config_buffer;
my @feature_lines;

my $conf_template='';
my $feature_counter=0;
open(C,"<$config_file");
while(defined(my $line=<C>)) {
    if($line=~/^feature:([^\s\t]+)\=/) {
	my $feature=$1;
	push(@feature_names,$feature);
	push(@feature_lines,$line);
	push(@config_buffer,$line) if(!defined($old_config_file));

	my $template_line=$line;
	$template_line=~s/([\s\t]opt\s*)\=\s*[^\[]*\[/$1=FEATUREVAL\_$feature_counter\[/;
	$conf_template.=$template_line;	
	$feature_counter++;
    } else {
	push(@config_buffer,$line) if(!defined($old_config_file));
	$conf_template.=$line;	
    }
}
close(C);

if(defined($old_config_file)) {
    my $first_feature_line=1;
    open(C,"<$old_config_file");
    while(defined(my $line=<C>)) {
	if($line=~/^feature:([^ ]+)\=/) {
	    if($first_feature_line) {
		for(my $i=0; $i<@feature_lines; $i++) {
		    push(@config_buffer,$feature_lines[$i]);
		}
		$first_feature_line=0;
	    }	    
	} else {
	    push(@config_buffer,$line);
	}
    }
    close(C);
}

my @val_buffer;
my @full_val_buffer;
my $iteration=0;
my @bleu_scores;
my @cmert_final_lambdas;

my @err_log_files=split(/\,/,$err_logs);
for(my $k=0; $k<@err_log_files; $k++) {
    my $err_log=$err_log_files[$k];
    open(E,"<$err_log");
    while(defined(my $line=<E>)) {
	if($line=~/^Best point: (.*) \|\|\| (.+)\n/) {
	    my $weight_string=$1;
	    my $bleu_score=$2;
	    $iteration++;
	    
	    my @lambdas=split(/ +/,$weight_string);
	    if($normalize_weights) {
		my $max_lambda=0;
		for(my $i=0; $i<@lambdas; $i++) {
		    $max_lambda=abs($lambdas[$i]) if(abs($lambdas[$i])>=$max_lambda);
		}
		for(my $i=0; $i<@lambdas; $i++) {
		    $lambdas[$i]/=$max_lambda;
		}
	    }
	    my @full_lambdas=@lambdas;
	    
	    for(my $i=0; $i<@lambdas; $i++) {
		if($lambdas[$i]=~/e\-/) {
		    $lambdas[$i]=~s/\.([0-9])[0-9]+e\-([0-9]+)/\.$1e\-$2/;
		} else {
		    $lambdas[$i]=~s/\.([0-9]{5}).+$/\.$1/;
		}
		
		my $tmp=$lambdas[$i];
		$tmp=~s/^\-//;
		my $no_char=rindex($tmp,"");
		for(my $k=$no_char; $k<7; $k++) {
		    $lambdas[$i].=' ';
		}
		$lambdas[$i]=" $lambdas[$i]" if($lambdas[$i]>=0);
	    }
	    
	    $val_buffer[$iteration]=join(" ",@lambdas);
	    $full_val_buffer[$iteration]=join(" ",@full_lambdas);
	    $bleu_scores[$iteration]=$bleu_score;
	} elsif($line=~/Training finished:\s*(.*)\n/) {
	    @cmert_final_lambdas=split(/ /,$1);
	    my $max_lambda=0;
	    for(my $i=0; $i<@cmert_final_lambdas; $i++) {
		$max_lambda=abs($cmert_final_lambdas[$i]) if(abs($cmert_final_lambdas[$i])>=$max_lambda);
	    }
	    for(my $i=0; $i<@cmert_final_lambdas; $i++) {
		$cmert_final_lambdas[$i]/=$max_lambda;
	    }
	    for(my $i=0; $i<@cmert_final_lambdas; $i++) {
		if($cmert_final_lambdas[$i]=~/e\-/) {
		    $cmert_final_lambdas[$i]=~s/\.([0-9])[0-9]+e\-([0-9]+)/\.$1e\-$2/;
		} else {
		    $cmert_final_lambdas[$i]=~s/\.([0-9]{5}).+$/\.$1/;
		}
		
		my $tmp=$cmert_final_lambdas[$i];
		$tmp=~s/^\-//;
		my $no_char=rindex($tmp,"");
	    for(my $k=$no_char; $k<7; $k++) {
		$cmert_final_lambdas[$i].=' ';
	    }
		$cmert_final_lambdas[$i]=" $cmert_final_lambdas[$i]" if($cmert_final_lambdas[$i]>=0);
	    }
	    
	}
    

	if($line=~/^total decoding time: CPU sec=([0-9\.\-e]+)\n/) {
	    my $decoding_time=$1;
	    push(@decoding_times,$decoding_time);
	    $avg_total_decoding_time+=$decoding_time;
	} elsif($line=~/^average decoding time: CPU sec=([0-9\.\-e]+)\n/) {
	    my $decoding_time=$1;
	    push(@avg_decoding_times,$decoding_time);
	} elsif($line=~/^min decoding time: CPU sec=([0-9\.\-e]+)\n/) {
	    my $decoding_time=$1;
	    push(@min_decoding_times,$decoding_time);
	} elsif($line=~/^max decoding time: CPU sec=([0-9\.\-e]+)\n/) {
	    my $decoding_time=$1;
	    push(@max_decoding_times,$decoding_time);
	} elsif($line=~/^total rescoring time: CPU sec=([0-9\.\-e]+)\n/) {
	    my $rescoring_time=$1;
	    push(@rescoring_times,$rescoring_time);
	    $avg_total_rescoring_time+=$rescoring_time;
	} elsif($line=~/^average rescoring time: CPU sec=([0-9\.\-e]+)\n/) {
	    my $rescoring_time=$1;
	    push(@avg_rescoring_times,$rescoring_time);
	} elsif($line=~/^min rescoring time: CPU sec=([0-9\.\-e]+)\n/) {
	    my $rescoring_time=$1;
	    push(@min_rescoring_times,$rescoring_time);
	} elsif($line=~/^max rescoring time: CPU sec=([0-9\.\-e]+)\n/) {
	    my $rescoring_time=$1;
	    push(@max_rescoring_times,$rescoring_time);
	}
    }
    close(E);
}

if(@decoding_times>0) {
    $avg_total_decoding_time/=@decoding_times;
}
if(@rescoring_times>0) {
    $avg_total_rescoring_time/=@rescoring_times;
}


my $max_bleu=0;
my $max_iteration;
for(my $i=1; $i<@bleu_scores; $i++) {
    if($bleu_scores[$i]>=$max_bleu) {
	$max_bleu=$bleu_scores[$i];
	$max_iteration=$i;
    }
}

#OVERWRITE (always take the last iterations);
#$max_iteration=-1;

for(my $i=0; $i<@decoding_times; $i++) {
    print STDERR "decoding  ", $i+1, "\t= $decoding_times[$i] sec.";
    if(defined($avg_decoding_times[$i])) {
	print STDERR " (avg=$avg_decoding_times[$i] min=$min_decoding_times[$i] max=$max_decoding_times[$i])";
    }
    print STDERR "\n";

}
printf STDERR "Avg. total = %.2f sec.\n", $avg_total_decoding_time;
print STDERR "\n";

for(my $i=0; $i<@rescoring_times; $i++) {
    if(defined($rescoring_times[$i])) {
	print STDERR "rescoring ", $i+1, "\t= $rescoring_times[$i] sec.";
	if(defined($avg_rescoring_times[$i])) {
	    print STDERR " (avg=$avg_rescoring_times[$i] min=$min_rescoring_times[$i] max=$max_rescoring_times[$i])";
	}
	print STDERR "\n";
    }
}
printf STDERR "Avg. total = %.2f sec.\n", $avg_total_rescoring_time;
print STDERR "\n";

for(my $i=1; $i<@bleu_scores; $i++) {
    print STDERR "iteration=$i\tBLEU=$bleu_scores[$i]";
    if($i==$max_iteration) {
	print STDERR "\t<--- best overall BLEU";
    } elsif($i>1 && $bleu_scores[$i]>$bleu_scores[$i-1]) {
	print STDERR "\t\^";
    } elsif($i>1 && $bleu_scores[$i]<$bleu_scores[$i-1]) {
	print STDERR "\tv";
    } elsif($i>1 && $bleu_scores[$i]==$bleu_scores[$i-1]) {
	print STDERR "\t\-";
    }
    print STDERR "\n";
}
print STDERR "\n";


for(my $i=0; $i<@feature_names; $i++) {
    print STDERR '(', $i+1, ,") $feature_names[$i]\n";
}

print STDERR "\n(1)";
for(my $i=1; $i<@feature_names; $i++) {
    print STDERR "      (", $i+1, ')';
}
print STDERR "\n";

#print STDERR " dist    lm      t(e|f)  l(e|f)  t(f|e)  l(f|e)  p-pen   w-pen\n";
for(my $i=1; $i<@val_buffer; $i++) {
    next if(!defined($val_buffer[$i]));
    if($i==$max_iteration) {
	print STDERR &add_indices($val_buffer[$i],1), "\t<--- best BLEU\n";
    } else {
	print STDERR &add_indices($val_buffer[$i],1), "\n";
    };
};

if(defined($cmert_final_lambdas[0])) {
    print STDERR "\nfinished training values:\n";
    print STDERR join(' ',@cmert_final_lambdas), "\n";    
}

print STDERR "\nBest values:\n";

exit(-1) if(!defined($max_iteration) || !defined($val_buffer[$max_iteration]));
#my $best_val=$full_val_buffer[$max_iteration];
my $best_val=$full_val_buffer[-1];
$best_val=~s/\t/ /g;
$best_val=~s/ +/ /g;
$best_val=~s/^ //;
$best_val=~s/ $//;
print STDERR "$best_val\n\n";
my @best_values=split(/ /,$best_val);

my $last_val=$val_buffer[-1];
$last_val=~s/\t/ /g;
$last_val=~s/ +/ /g;
$last_val=~s/^ //;
$last_val=~s/ $//;
my @last_values=split(/ /,$last_val);

if(defined($new_config_file)) {
    for(my $i=0; $i<@feature_names; $i++) {
	my $escape_feature_name=$feature_names[$i];
	$escape_feature_name=~s/([\[\]\|\(\)])/\\$1/g;
	for(my $j=0; $j<@config_buffer; $j++) {
	    if($config_buffer[$j]=~/^feature:$escape_feature_name\=/) {
		print STDERR "best_values[$i]=$best_values[$i]\n";
		$config_buffer[$j]=~s/^[^ \=]+\=[^\s\t\(]*/feature:$escape_feature_name\=$best_values[$i]/;
		$config_buffer[$j]=~s/\\//g;
	    }
	}
    }

    if(-e "$new_config_file") {
	print STDERR "ouput file $new_config_file already exists.\n";
	exit(-1);
    } else {
	open(F,">$new_config_file");
	for(my $i=0; $i<@config_buffer; $i++) {
	    print F $config_buffer[$i];
	}
	close(F);
    }
}


if($print_new) {
    my $selected_iteration;
    my $selected_iteration_to;

    if($select_strategy eq 'last') {
	$selected_iteration=-1;
    } elsif($select_strategy eq 'best') {
	$selected_iteration=$max_iteration;
    } elsif($select_strategy=~/^([0-9]+)$/) {
	$selected_iteration=$1;
    } elsif($select_strategy=~/^([0-9]+)\-([0-9]+)$/) {
	$selected_iteration=$1;
	$selected_iteration_to=$2;
    }
    

    print STDERR "selected iteration=$selected_iteration\n";
    my @selected_lambdas=split(/ /,$full_val_buffer[$selected_iteration]);
    
    if(defined($selected_iteration_to)) {
	for(my $i=$selected_iteration+1; $i<=$selected_iteration_to; $i++) {
	    my @lambdas=split(/ /,$full_val_buffer[$i]);
	    for(my $j=0; $j<@lambdas; $j++) {
		$selected_lambdas[$j]+=$lambdas[$j];
	    }
	}
	for(my $j=0; $j<@selected_lambdas; $j++) {
	    $selected_lambdas[$j]/=(($selected_iteration_to-$selected_iteration)+1);
	}
    }
	

    for(my $i=0; $i<@selected_lambdas; $i++) {
	$conf_template=~s/FEATUREVAL\_$i\[/$selected_lambdas[$i]\[/;
    }

    if(-e "$config_file_print" && !(-z "$config_file_print")) {
	print STDERR "File $config_file_print already exists.\n";
	exit(-1);
    } else {
	print STDERR "generated: $config_file_print\n";
	open(F,">$config_file_print");
	print F $conf_template;
	close(F);
    }

    for(my $i=0; $i<@sparse_feature_files; $i++) {
	my $sparse_file=$sparse_feature_files[$i];
	my $sparse_file_print="$sparse_file.$select_strategy";

	if(-e "$sparse_file_print" && !(-z "$sparse_file_print")) {
	    print STDERR "File $sparse_file_print already exists.\n";
	    exit(-1);
	} else {
	    print STDERR "generated: $sparse_file_print\n";
	    open(F,"<$sparse_file")||die("can't open file $sparse_file: $!\n");
	    open(P,">$sparse_file_print")||die("can't open file $sparse_file_print: $!\n");
	    while(defined(my $line=<F>)) {
		chomp($line);
		my($feature_id,$feature_name,$feature_weights_string)=split(/ \|\|\| /o,$line);
		my @feature_weights=split(/ /,$feature_weights_string);
		my $selected_iteration_sparse=$selected_iteration-1;
		my $selected_weights="$feature_weights[0] $feature_weights[$selected_iteration_sparse]";
		print P "$feature_id ||| $feature_name ||| $selected_weights\n";
	    }	    
	    close(F);
	    close(P);
	}
    }
}


print STDERR "Next iteration call:\n";
#print STDERR "$best_values[0]\,$ranges[0]";
#for($i=1; $i<@best_values; $i++) {
#    print STDERR "\;$best_values[$i]\,$ranges[$i]";
#};

#print STDERR "$last_values[0]\,$ranges[0]";
#for(my $i=1; $i<@last_values; $i++) {
#    print STDERR "\;$last_values[$i]\,$ranges[$i]";
#};
#print STDERR "\n\n";

sub add_indices {
    my($string,$offset)=@_;
    $offset||=0;
    my $string_copy=$string;
    $string_copy=~s/^[\s\t]+//;
    $string_copy=~s/[\s\t]+$//;

    my @values=split(/[\s\t]+/,$string_copy);
    for(my $i=0; $i<@values; $i++) {
	$values[$i]=($i+$offset).":$values[$i]";
    }

    return join(' ',@values);
#    return $string;
}
    
