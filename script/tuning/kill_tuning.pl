#!/usr/bin/perl -w

use strict;
use warnings;
use File::Basename;

my $script_name=basename($0);

#Print the legend if the number of arguments is zero
if ( @ARGV == 0 ) {
    print "-------\n";
    print "INFO:\n";
    print "    This script allows to kill all the tuning processes to stop tuning.\n";
    print "-------\n";
    print "USAGE:\n";
    print "    $script_name <tuning_error_log>\n";
    print "        <tuning_error_log> - the tuning.log file produced by the tuning script\n";
    print "-------\n";
    print "ERROR: Provide required script arguments!\n";
    exit(-1);
}

my($err_log)=@ARGV;

#First check if the person does want to do that
print "\nAre you sure you want to stop the tuning process?\n";

print "\n";
print "Answer (yes/no)? ";
my $answer=<STDIN>;
if($answer!~/^yes/) {
    print STDERR "Processes spared.\n";
    exit(-1);
}

print STDERR "Starting killing the active tuning processes ...\n";

#Declare the variable for the number of killed processes
my $num_killed;

#Iterate until there is no active process ids left
do {
    print "----------------------------------------------------------------------\n";
    print "Starting a killing iteration, analysing the error log for process ids.\n";
    
    #Just read all the process ids logged in the error log
    my @all_pids;
    open(E,"<$err_log")||die("ERROR: Can't open file $err_log: $!\n");
    while(defined(my $line=<E>)) {
        if($line=~/.* pid=([0-9]+)\n/) {
            push @all_pids, $1;
        }
    }
    close(E);

    print "The error log analysis is done, starting the killing.\n";

    #Kill and report on active processes
    $num_killed = 0;
    foreach my $pid (@all_pids) {
        #Check if the process is running
        system("ps -p $pid >/dev/null 2>&1");
        my $exit_value = $? >> 8;
        if($exit_value == 0) {
            system("ps -p $pid | grep -v '  PID TTY          TIME CMD'");
            system("kill -9 ${pid}");
            $num_killed += 1;
        }
    }
    print "The number of (newly) killed processes is: $num_killed\n";
} until($num_killed==0);

print "----------------------------------------------------------------------\n";
print STDERR "The killing is over, we are finished!\n";

