
sub load_config_file {
    my($config_file,$line);

    ($config_file,$mert_mode,$data,$parameters,$feature_weights,$function_templates,$feature_name2id,$feature_id2name,$default_values,$rescore_lattice)=@_;

    my $feature_counter=0;

    open(F,"<$config_file")||die("can't open config_file $config_file: $!\n");
    while(defined($line=<F>)) {
	$line=~s/[\s\t]*(.*?)[\s\t]*\n/$1/;

	if($line=~/^\#/) {
	    #skip
	} elsif($line=~/^([^\:]+)\:([^\=]+)\=([^\s\t]+)(.*)$/) {
	    my $type=$1;
	    my $name=$2;
	    my $value=$3;
	    my $mert_value=$4;
	    my $default_value;
	    $name=~s/\..+$//;

	    if($value=~/^([0-9\.\-e]+)\(([0-9\.\-e]+)\)/) {
		$value=$1;
		$default_value=$2;
	    }
	    if($type eq 'feature' && $mert_mode && defined($mert_value) && $mert_value=~/opt=([^\[]+)\[/) {
		$mert_value=~s/^.*[\s\t]opt=([^\[]+)\[.*$/$1/;
		$value=$mert_value;
	    } elsif($type eq 'feature' && $mert_mode) {
		print STDERR "mert opt value missing in config-file=$config_file\n";
		print STDERR "line=$line\n";
		print STDERR "name=$name\n";
		print STDERR "mert_value=$mert_value\n";
		exit(-1);
	    }

	    if($type eq 'parameter') {
		if($name=~/^([^\[\]]+)\[([0-9]+)\]$/) {
		    my $name_label=$1;
		    my $name_index=$2;
		    $$parameters{$name_label}[$name_index]=$value;
		} else {
		    $$parameters{$name}=$value;
            #print STDERR "HAMID: paremeter $name with value $value got in \n";
		}
	    } elsif($type eq 'feature') {
		if($name=~/phrase_table\[([0-9]+)\]/) {
		    my $phrase_table_index=$1;
#		    $$feature_weights{'phrase_table'}[$phrase_table_index]=$value;
		    $$default_values{'phrase_table'}[$phrase_table_index]=$default_value if(defined($default_value));
		    print STDERR "default_values{'phrase_table'}[$phrase_table_index]=$$default_values{'phrase_table'}[$phrase_table_index]\n";
		    $$feature_name2id{$name}=$feature_counter;
		    $$feature_weights{$feature_counter}=$value;
		    print STDERR "feature_name2id{$name}=$$feature_name2id{$name}\n";
		    $feature_counter++;
		} elsif($name=~/^(lex\_(?:dm|hrm)[^\[]*)\[([0-9]+)\]/) {
		    my $dm_feature_name=$1;
		    my $dm_index=$2;

		    if($dm_feature_name=~/^lex\_(dm|hrm)$/) {
			$dm_feature_name="$dm_feature_name\_fe";
		    }

		    my $dm_feature_name_rescoring="$dm_feature_name\[$dm_index\]";

#		    $$features{'lex_dm'}[$dm_index]=$value;
#		    $$default_values{'lex_dm'}[$dm_index]=$default_value if(defined($default_value));



# orig:
#		    $$features{$dm_feature_name}[$dm_index]=$value;
# new:
#		    $$feature_weights{$dm_feature_name_rescoring}=$value;

		    $$default_values{$dm_feature_name}[$dm_index]=$default_value if(defined($default_value));
		    print STDERR "default_values{'$dm_feature_name'}[$dm_index]=$$default_values{'$dm_feature_name'}[$dm_index]\n" if(defined($default_value));
		    $$feature_name2id{$dm_feature_name_rescoring}=$feature_counter;
		    print STDERR "feature_name2id{$dm_feature_name_rescoring}=$$feature_name2id{$dm_feature_name_rescoring}\n";
		    $$feature_weights{$feature_counter}=$value;
		    $feature_counter++;
		} else {
#		    $$feature_weights{$name}=$value;
		    $$feature_name2id{$name}=$feature_counter;
		    print STDERR "feature_name2id{$name}=$$feature_name2id{$name}\n";
		    print STDERR "feature_weights{$name}=$value\n";
		    $$feature_weights{$feature_counter}=$value;
		    $feature_counter++;
		}
	    } elsif($type eq 'data') {
		if($name=~/^(bilm_model)\[([0-9]+)\]/) {
		    my $bilm_name=$1;
		    my $bilm_index=$2;
		    $$data{$bilm_name}[$bilm_index]=$value;
		} elsif($name=~/^(depbilm_model)\[([0-9]+)\]/) {
		    my $depbilm_name=$1;
		    my $depbilm_index=$2;
		    $$data{$depbilm_name}[$depbilm_index]=$value;
		} elsif($name=~/^(classlm_model)\[([0-9]+)\]/) {
		    my $classlm_name=$1;
		    my $classlm_index=$2;
		    $$data{$classlm_name}[$classlm_index]=$value;
		} elsif($name=~/^(classlm_map)\[([0-9]+)\]/) {
		    my $classlm_name=$1;
		    my $classlm_index=$2;
		    $$data{$classlm_name}[$classlm_index]=$value;
                } elsif($name=~/^(classlm_fullibm)\[([0-9]+)\]/) {
                    my $classlm_name=$1;
                    my $classlm_index=$2;
                    $$data{$classlm_name}[$classlm_index]=$value;
		} elsif($name=~/^(lex\_(?:dm|hrm)\_model)$/) {
		    $name="$name\_fe";
		    $$data{$name}=$value;
		} else {
		    $$data{$name}=$value;
		}
	    } elsif($type eq 'function') {
		$$function_templates{$name}=$value;
	    } else {
		print STDERR "Incorrect configuration: $line\n";
	    }

	}

    }

    #default config parameter values:
    $$parameters{'poslm_order'}=0 if(!defined($$parameters{'poslm_order'}));
    $$parameters{'min_prune_stack_size'}=0 if(!defined($$parameters{'min_prune_stack_size'}));
    $$parameters{'max_phrase_length'}=7 if(!defined($$parameters{'max_phrase_length'}));
    $$parameters{'phrase_table_threshold'}=0 if(!defined($$parameters{'phrase_table_threshold'}));
#    $$parameters{'precompute-lm'}=0 if($mert_mode);


    foreach my $name (keys %$parameters) {
	if(@{ $$parameters{$name} }>0) {
	    for(my $i=0; $i<@{ $$parameters{$name} }; $i++) {
		print STDERR "parameters{$name}[$i]=$$parameters{$name}[$i]\n";
	    }
	} else {
	    print STDERR "parameters{$name}=$$parameters{$name}\n";
	}
    }

   foreach my $name (keys %$feature_name2id) {
       $$feature_id2name{$$feature_name2id{$name}}=$name;
   }


    # if in rescore_lattice mode, switch off all pruning
    if($rescore_lattice) {
	print STDERR "In lattice rescoring mode:\n";
	print STDERR "parameters{'lm_order'}=parameters{'rescore_lm_order'}=$$parameters{'rescore_lm_order'}\n";
	$$parameters{'lm_order'}=$$parameters{'rescore_lm_order'};
	print STDERR "parameters{'beam_width'}=0.0000000001\n";
	$$parameters{'beam_width'}=0.0000000001;
        print STDERR "parameters{'children_beam_width'}=0.0000000001\n";	$$parameters{'children_beam_width'}=0.0000000001;
	print STDERR "parameters{'stack_size'}=1000\n";
	$$parameters{'stack_size'}=1000;
    }

    close(F);
}


sub buffer_config_file {
    my($config_file)=@_;

    my @buffer;
    open(F,"<$config_file")||die("can't open config_file $config_file: $!\n");
    while(defined($line=<F>)) {
	push(@buffer,$line);
    }
    close(F);

    return @buffer;
}


sub return_initial_lambda_string {
    my($config_file)=@_;

    my @lambdas;
    open(F,"<$config_file")||die("can't open config_file $config_file: $!\n");
    while(defined($line=<F>)) {
	if($line=~/^feature:.+[\s\t]init=([^\[]+)\[([0-9\.e\-]+)\,([0-9\.e\-]+)\]/) {
	    my $value=$1;
	    my $lower=$2;
	    my $higher=$3;
	    push(@lambdas,"$value\,$lower\-$higher");
	}
    }
    close(F);

    return join(";",@lambdas);
}

sub return_last_lambda_string {
    my($config_file)=@_;

    my @lambdas;
    open(F,"<$config_file")||die("can't open config_file $config_file: $!\n");
    while(defined($line=<F>)) {
	if($line=~/^feature:.+[\s\t]opt=([^\[]+)\[([0-9\.e\-]+)\,([0-9\.e\-]+)\]/) {
	    my $value=$1;
	    my $lower=$2;
	    my $higher=$3;
	    push(@lambdas,"$value\,$lower\-$higher");
	}
    }
    close(F);

    return join(";",@lambdas);
}



return 1;
