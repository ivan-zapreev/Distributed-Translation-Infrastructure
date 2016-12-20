#!/usr/bin/perl -w

#
# Owner: Dr. Ivan S Zapreev
#
# Visit my Linked-in profile:
#      <https://nl.linkedin.com/in/zapreevis>
# Visit my GitHub:
#      <https://github.com/ivan-zapreev>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Created on November 14, 2016, 11:07 AM
#

use strict;
use warnings;
use Getopt::Long "GetOptions";
use File::Basename;

my $script_name=basename($0);

#Print the legend if the number of arguments is zero
if ( @ARGV == 0 ) {
    print "-------\n";
    print "INFO:\n";
    print "    This script allows to monitor running/finished tuning process and\n";
    print "    to extract the configuration files for different tuning iterations.\n";
    print "    This script must be run from the same folder where tuning was/is run.\n";
    print "-------\n";
    print "USAGE:\n";
    print "    $script_name --conf=<file_name> --err=<file_name> --select=<string>\n";
    print "        --conf=<file_name> - the configuration file used in tuning process\n";
    print "        --err=<file_name>  - the tuning.log file produced by the tuning script\n";
    print "        --select=<string>  - the iteration index for which the config file is to\n";
    print "                             be generated or 'best'/'last' values to get the config\n";
    print "                             files for the best-scoring or last iteration respectively.\n";
    print "                             This parameter is optional, if specified - the script\n";
    print "                             generates a corresponding iteration's config file\n";
    print "-------\n";
    print "ERROR: Provide required script arguments!\n";
    exit(-1);
}

my $err_logs;
my $config_file;
my $select_strategy='undef';

GetOptions(
    "conf=s" => \$config_file,
    "err=s" => \$err_logs,
    "select=s" => \$select_strategy,
    );

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

my @feature_names;
my @config_buffer;
my @feature_lines;

#Read the feature mapping file first
my $features_mapping_file = ".$config_file.work.feature_id2name";
open(FEATURES_MAPPING_FILE, "<$features_mapping_file")||die("ERROR: Can't open file $features_mapping_file: $!\n");
my %features2id = ();
my %features_spec = ();
while(my $line = <FEATURES_MAPPING_FILE>)
{
    #Extract the feature and id values
    chomp($line);
    my($index, $feature_name) = split(/\t/, $line);
    
    #Extract the property name from the feature name
    my $prap_name = $feature_name;
    $prap_name =~ s/\[\d+\]//;
    
    #Remember if this feature is a single or multiple
    if($feature_name =~ /^[^\[]+\[\d+\]$/) {
        $features_spec{$prap_name} = 1;
    } else {
        $features_spec{$prap_name} = 0;
    }
    
    #Store the feature base name, if it is a new one
    my $num_dist_features = scalar @feature_names;
    if(($num_dist_features == 0 ) || ($feature_names[-1] ne $prap_name)) {
        push(@feature_names,$prap_name);
    }
    
    #Store the fearute name to id mapping
    $features2id{$feature_name} = $index;
}
close(FEATURES_MAPPING_FILE);

#Parse the configuration file and prepare its template
my $conf_template='';
open(C,"<$config_file")||die("ERROR: Can't open file $config_file: $!\n");
while(defined(my $line=<C>)) {
    #Check that the line is a property and is also a valid feature
    if(($line=~/^[\s\t]*([^\s\t]+)\=/)) {
        #The line contains a property
        my $prap_name = $1;
        if($prap_name ~~ @feature_names) {
            #The property is a feature
            push(@feature_lines,$line);
            push(@config_buffer,$line);

            my $template_line=$line;

            #Substitute features with their place holders
            sub get_feature_id {
                my ($feature_name, $elem_idx) = @_;
                if($features_spec{$feature_name} == 1) {
                    #This feature has multiple values
                    $feature_name .= "[".$elem_idx."]";
                }
                return $features2id{$feature_name};
            }
            
            #Iterate through features and replace them with template element ids
            my $elem_idx = 0;
            while($line =~ m/([=|])[\s\t]*([-+]?\d*\.?\d*)/g) {
                my $feature_id = get_feature_id($prap_name, $elem_idx);
                $template_line=~s/($1)[\s\t]*$2/$1<$feature_id>/;
                $elem_idx++;
            }

            $conf_template.=$template_line;	
        } else {
            #The line is a non-feature property
            push(@config_buffer,$line);
            $conf_template.=$line;	
        }
    } else {
        #The line is a comment or empty
        push(@config_buffer,$line);
        $conf_template.=$line;	
    }
}
close(C);

my @val_buffer;
my @full_val_buffer;
my $iteration=0;
my @bleu_scores;
my @cmert_final_lambdas;

my @err_log_files=split(/\,/,$err_logs);
for(my $k=0; $k<@err_log_files; $k++) {
    my $err_log=$err_log_files[$k];
    open(E,"<$err_log")||die("ERROR: Can't open file $err_log: $!\n");
    while(defined(my $line=<E>)) {
        if($line=~/^Best point: (.*) \|\|\| (.+)\n/) {
            my $weight_string=$1;
            my $bleu_score=$2;
            $iteration++;

            my @lambdas=split(/ +/,$weight_string);
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

#In case the selected strategy is chosen then we shall generate a config script
if($select_strategy ne 'undef') {
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
    } else {
        print STDERR "ERROR: Unexpected '--select=' option value: $select_strategy.\n";
        exit(1);
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
        $conf_template=~s/<$i>/$selected_lambdas[$i]/;
    }

    my $config_file_print="$config_file.$select_strategy";

    if(-e "$config_file_print" && !(-z "$config_file_print")) {
        print STDERR "ERROR: File $config_file_print already exists.\n";
        exit(-1);
    } else {
        print STDERR "generated: $config_file_print\n";
        open(F,">$config_file_print");
        print F $conf_template;
        close(F);
    }
}

print STDERR "Next iteration call:\n";

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
}
    
