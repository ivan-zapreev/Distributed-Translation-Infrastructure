#!/usr/bin/perl -w

use strict;
use warnings;

my($moses_train,$id,$parameters)=@ARGV;

my $no_args=@ARGV;
print STDERR "no of arguments=$no_args\n";

if(@ARGV!=3) {
    print STDERR "usage: moses-train-wrapper.pl <moses-script> <id> <\"parameters\">\n";
    exit(-1);
}

my $root_dir='.';
if($parameters=~/\-\-root-dir=([^ ]+)[ \n]/) {
    $root_dir=$1;
}

#$parameters=~s/\-\-root-dir=([^ ]+)//;

print STDERR "$moses_train $parameters\n";
system("$moses_train $parameters");
#system("touch $root_dir/$id.finished");
system("touch $id.finished");

