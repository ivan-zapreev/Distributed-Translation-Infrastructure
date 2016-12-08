#!/usr/bin/perl -w


use strict;
use warnings;
use Getopt::Long "GetOptions";
use File::Spec::Functions;

my $dir;

my $_HELP;
$_HELP = 1
  unless &GetOptions(
  "dir|d=s" => \$dir,
  "h|help" => \$_HELP
	   );

if(!defined($dir)) {
    print STDERR "  --dir must be specified\n";
    $_HELP=1;
}

if($_HELP) {
    print "\nOptions:
  --dir=str : path to the tuning experiment directory
  --help : print this message.\n\n";
    exit(-1);
}

$dir=File::Spec->rel2abs($dir);

my @rm_calls;
push(@rm_calls,"rm -rf $dir/translations\.[0-9] $dir/translations\.[0-9][0-9]");
push(@rm_calls,"rm -rf $dir/translations\_trace\.[0-9] $dir/translations\_trace\.[0-9][0-9]");
push(@rm_calls,"rm -rf $dir/cpu_times.*");
push(@rm_calls,"rm -rf $dir/rescored");
push(@rm_calls,"rm -rf $dir/nohup.out");
push(@rm_calls,"rm -rf $dir/*.feature_id2name");
push(@rm_calls,"rm -rf $dir/lattices");
push(@rm_calls,"rm -rf $dir/\*\.conf\.0 $dir/\*\.conf\.init");
#push(@rm_call,"rm -rf $dir/mert-work/pool.nbest\*");
#push(@rm_call,"rm -rf $dir/mert-work/pro\_\*");
push(@rm_calls,"rm -rf $dir/mert-work");

opendir(D,$dir);
while(defined(my $file=readdir(D))) {
    if(-z "$file") {
	push(@rm_calls,"rm -rf $dir/$file");
    }
}
closedir(D);


print STDERR "The following remove calls will be executed:\n\n";
print STDERR join("\n",@rm_calls);

print STDERR "\n\nDo you want to execute all of these calls [Y/n]? : ";
my $reply=<STDIN>;
chomp($reply);

print STDERR "\n\n";
if($reply eq 'Y') {
    for(my $i=0; $i<@rm_calls; $i++) {
	print STDERR "$rm_calls[$i]\n";
	system($rm_calls[$i]);
    }
} else {
    print STDERR "Aborted.\n";
}



 
