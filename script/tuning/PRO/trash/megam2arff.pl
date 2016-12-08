#!/usr/bin/perl -w

use strict;
use warnings;

my $use_sparse_format=1;

my %feature_names;
my @arff_instances;
my $instance_id=0;
my @feature_indices;

my $smallest_feature_index;
my $largest_feature_index;

my @classes;


# /opt/arch/java/java-1.6.0/jdk1.6.0-x86_64/bin/java -cp /home/ilps/smt/software/weka/weka-3-6-9/weka.jar weka.classifiers.functions.LinearRegression -t foo.arff

while(defined(my $line=<STDIN>)) {
    chomp($line);
    my($class,@feature_values)=split(/ /,$line);
    $class=-1 if($class==0);
    $classes[$instance_id]=$class;
    for(my $i=0; $i<@feature_values-1; $i+=2) {
	my($feature_index)=$feature_values[$i]=~/^F([0-9]+)$/;
	my $value=$feature_values[$i+1];
	$arff_instances[$instance_id][$feature_index]=$value;
	$feature_indices[$feature_index]=1;
    }
    $instance_id++;
}


my $class_arff_index=@feature_indices-1;

print "\@RELATION BLEUREG\n\n";

for(my $i=1; $i<@feature_indices; $i++) {
    print "\@ATTRIBUTE F$i NUMERIC\n";
}
print "\@ATTRIBUTE bleu NUMERIC\n";

print "\n\@DATA\n";
for(my $i=0; $i<@arff_instances; $i++) {
    my @sparse_entries;
    if($use_sparse_format) {
	for(my $j=1; $j<@feature_indices; $j++) {
	    if(defined($arff_instances[$i][$j])) {
		my $sparse_index=$j-1;
		push(@sparse_entries,"$sparse_index $arff_instances[$i][$j]");
	    }
	}
    } else {
	for(my $j=1; $j<@feature_indices; $j++) {
	    if(!defined($arff_instances[$i][$j])) {
		$arff_instances[$i][$j]='?';
	    }
	}
    }

    if($use_sparse_format) {
	print "\{", join(',',@sparse_entries), "\,$class_arff_index $classes[$i]\}\n"; 
    } else {
	shift(@{ $arff_instances[$i] });
	print join(',',@{ $arff_instances[$i] }), "\,$classes[$i]\n";
    }
}
