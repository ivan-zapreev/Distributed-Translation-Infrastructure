#!/usr/bin/perl -w

use strict;
use warnings;
use Getopt::Long "GetOptions";

my $experiment_dir;
my $f_suffix;

my $e_suffix;
my $a_suffix;
my $files_prefix;
my $_HELP;
my $build_distortion_model = 0;
my $use_moses_orientation = 0;
my $use_dlr = 0;
my $use_hdm = 0;
my $dm_languages = 'fe';
my $dm_interpolation=0.75;
my $clean_up = 0;
my $build_phrase_table = 0;
my $phrase_table_smoothing_string;
my $dependencies_path;
#my $_HELP = 0;

$_HELP = 1
unless &GetOptions(
	"f=s" => \$f_suffix,
    "e=s" => \$e_suffix,
    "a=s" => \$a_suffix,
    "experiment-dir=s" => \$experiment_dir,
    "input-files-prefix=s" => \$files_prefix,
    "build-distortion-model" => \$build_distortion_model,
    "moses-orientation" => \$use_moses_orientation,
    "use-dlr" => \$use_dlr,
    "use-hdm|use-hrm" => \$use_hdm,
    "dm-lang=s" => \$dm_languages,
    "distortion-model-interpolation=s" => \$dm_interpolation,
    "build-phrase-table" => \$build_phrase_table,
    "pt-smoothing|phrase-table-smoothing=s" => \$phrase_table_smoothing_string,
    "dependencies=s" => \$dependencies_path,
    "help|h" => \$_HELP,
);

if(!defined($f_suffix)) {
    $_HELP=1;
}

if(!defined($dependencies_path)) {
    print STDERR "  --dependencies=str must be set to the dependencies folder\n";
    $_HELP=1;
}

if($_HELP) {
    print "\nOptions:
  --f : foreign-side suffix (e.g., 'ar' or 'fr' if the source file name is [shared-Prefix-With-Other-Files].ar or [shared-Prefix-With-Other-Files].fr)
  --e : target-side suffix (e.g., 'en' or 'de'  if the target file name is [shared-Prefix-With-Other-Files].en or [shared-Prefix-With-Other-Files].de)
  --a : alignment files suffix (e.g., grow-diag-final if the alignment file name is  [shared-Prefix-With-Other-Files].grow-diag-final )
  --moses-orientation : use moses orientation counts
  --use-dlr (use discontinous left+right)
  --use-hdm (use hierarchical distortion modeling)
  --dm-lang=string ('fe,f,e' or 'fe,e' or 'fe'=default)
  --experiment-dir=string (path to the directory where the source, target and the alignment files are located and the output files are being placed)
  --input-files_prefix=srting (the shared prefix of the input files names)
  --build_distortion_model (flag to create lexicalized reordering model)
  --build_phrase_table (flag to create phrase table (translation model))
  --dependencies=string (path to dependencies folder)
  --help : print this message.\n\n";
    exit(-1);
}


my @config_pt_lines;
$config_pt_lines[0]='feature:phrase_table[0].t(f|e)=0.01(-10)         init=0.1[0.1,0.3]  opt=0.1[0.1,0.3]';
$config_pt_lines[1]='feature:phrase_table[1].l(f|e)=0.01(-10)         init=0.1[0.1,0.3]  opt=0.1[0.1,0.3]';
$config_pt_lines[2]='feature:phrase_table[2].t(e|f)=0.01(-10)         init=0.1[0.1,0.3]  opt=0.1[0.1,0.3]';
$config_pt_lines[3]='feature:phrase_table[3].l(e|f)=0.01(-10)         init=0.1[0.1,0.3]  opt=0.1[0.1,0.3]';
$config_pt_lines[4]='feature:phrase_table[4].phrase_penalty(e|f)=0.11(-10)         init=-0.5[0.1,0.3]  opt=0.1[0.1,0.3]';

print STDERR "Build lexical translations.\n";
system("mkdir $experiment_dir/models");
system("mkdir $experiment_dir/models/model");
system("mkdir $experiment_dir/lex_trans");
system("mkdir $experiment_dir/lex_trans/model");
system("mkdir $experiment_dir/lex_trans/corpus");
my @lines_lex_bitext_f;
my @lines_lex_bitext_e;
my @lines_lex_bitext_a;
my @lines_pos_bitext_f;
my @lines_pos_bitext_e;
my @lines_misc_text;

my $bitext_f = "$experiment_dir/$files_prefix.$f_suffix";
my $bitext_e = "$experiment_dir/$files_prefix.$e_suffix";
my $bitext_a = "$experiment_dir/$files_prefix.$a_suffix";
&buffer_file($bitext_f, \@lines_lex_bitext_f, 0);
&buffer_file($bitext_e, \@lines_lex_bitext_e, 0);
&buffer_file($bitext_a, \@lines_lex_bitext_a, 0);


=begin COMMENTS

for(my $align_run=0; $align_run<@aligner_runs; $align_run++) {
    my $aligner=$aligner_runs[$align_run];
    foreach my $refinement_strategy (keys %{ $refiners{$aligner} }) {
	# f:
	&buffer_file($bitext_f,\@lines_lex_bitext_f,0);
	if(defined($pos_bitext_f)) {
	    &buffer_file($pos_bitext_f,\@lines_pos_bitext_f,0);
	}
	if($background_size>0) {
	    &buffer_file($background_bitext_f,\@lines_lex_bitext_f,0);
	    if(defined($pos_background_bitext_f)) {
		&buffer_file($pos_background_bitext_f,\@lines_pos_bitext_f,0);
	    }
	}
	if($dict_size>0) {
	    &buffer_file($dict_bitext_f,\@lines_lex_bitext_f,0);
	}

	# e:
	&buffer_file($bitext_e,\@lines_lex_bitext_e,0);
	if(defined($pos_bitext_e)) {
	    &buffer_file($pos_bitext_e,\@lines_pos_bitext_e,0);
	}
	if($background_size>0) {
	    &buffer_file($background_bitext_e,\@lines_lex_bitext_e,0);
	    if(defined($pos_background_bitext_e)) {
		&buffer_file($pos_background_bitext_e,\@lines_pos_bitext_e,0);
	    }
	}
	if($dict_size>0) {
	    &buffer_file($dict_bitext_e,\@lines_lex_bitext_e,0);
	}
	# refined alignments:
	&buffer_file("$experiment_dir/align.$aligner/refined.$refinement_strategy/model/aligned.$refinement_strategy",\@lines_lex_bitext_a,0);

	for(my $i=0; $i<@misc_corpora; $i++) {
	    &buffer_file($misc_corpora[$i],\@{ $lines_misc_text[$i] },0);
	    if($background_size>0) {
		&buffer_file($misc_background_corpora[$i],\@lines_misc_text,0);
	    }
	}
    }
}
=end COMMENTS
=cut

&write_buffer_to_file(\@lines_lex_bitext_f,"$experiment_dir/lex_trans/corpus/lex_aligned.$f_suffix");
undef @lines_lex_bitext_f;
&write_buffer_to_file(\@lines_lex_bitext_e,"$experiment_dir/lex_trans/corpus/lex_aligned.$e_suffix");
undef @lines_lex_bitext_e;
&write_buffer_to_file(\@lines_lex_bitext_a,"$experiment_dir/lex_trans/model/aligned.grow-diag-final");
undef @lines_lex_bitext_a;

#These are defined here to skip commenting the codes using them. The codes were not needed at the moment.
#However they may be needed later. HAMID
my $pos_bitext_f;
my $pos_bitext_e;
my @misc_corpora;

if(defined($pos_bitext_f)) {
    &write_buffer_to_file(\@lines_pos_bitext_f,"$experiment_dir/models/model/aligned_pos.$f_suffix");
    undef @lines_pos_bitext_f;
}
if(defined($pos_bitext_e)) {
    &write_buffer_to_file(\@lines_pos_bitext_e,"$experiment_dir/models/model/aligned_pos.$e_suffix");
    undef @lines_pos_bitext_e;
}

for(my $i=0; $i<@misc_corpora; $i++) {
    my($corpus_name)=$misc_corpora[$i]=~/([^\/]+)$/;
    &write_buffer_to_file(\@{ $lines_misc_text[$i] },"$experiment_dir/models/model/aligned_misc.$corpus_name");
    undef @{ $lines_misc_text[$i] };
}


my $external_path = defined($dependencies_path) ? $dependencies_path : "./dependencies";
my $moses_train_script="./dependencies/moses/scripts/training/train-model.perl";

my $call_lex_prob="$moses_train_script --root-dir=$experiment_dir/lex_trans --external-bin-dir=$external_path/external_binaries --corpus=$experiment_dir/lex_trans/corpus/lex_aligned --f=$f_suffix --e=$e_suffix --first-step=4 --last-step=4 --alignment=grow-diag-final >& $experiment_dir/err.lex_prob.log";
print STDERR "$call_lex_prob\n";
system($call_lex_prob);

my $clean_lex_f2e_call="cat $experiment_dir/lex_trans/model/lex.f2e | $external_path/build/bitext_models/remove-moses-zero-lex-prob-entries.pl 1> $experiment_dir/lex_trans/model/lex.f2e.clean";
print STDERR "$clean_lex_f2e_call\n";
system($clean_lex_f2e_call);
system("mv $experiment_dir/lex_trans/model/lex.f2e.clean $experiment_dir/models/model/lex.f2e");

my $clean_lex_e2f_call="cat $experiment_dir/lex_trans/model/lex.e2f | $external_path/build/bitext_models/remove-moses-zero-lex-prob-entries.pl 1> $experiment_dir/lex_trans/model/lex.e2f.clean";
print STDERR "$clean_lex_e2f_call\n";
system($clean_lex_e2f_call);
system("mv $experiment_dir/lex_trans/model/lex.e2f.clean  $experiment_dir/models/model/lex.e2f");


my $corpus_size=&number_lines($bitext_f);

# NEXT:
if($build_distortion_model) {
    my @lines_dm_bitext_f;
    my @lines_dm_bitext_e;
    my @lines_dm_bitext_a;
    my $last_sent_id_f=0;
    my $last_sent_id_e=0;
    system("mkdir $experiment_dir/dm");
    system("mkdir $experiment_dir/dm/corpus");
    system("mkdir $experiment_dir/dm/model");
#    my @aligner_files;
#    for(my $aligner_run=0; $aligner_run<@aligner_runs; $aligner_run++) {
#        my $aligner=$aligner_runs[$aligner_run];
#        foreach my $refinement_strategy (keys %{ $bitext_refiners{$aligner} }) {
	    # f:
	    $last_sent_id_f=&buffer_file_dm_bitext($bitext_f,\@lines_dm_bitext_f,$last_sent_id_f,1);
#	    if($background_size>0) {
#		$last_sent_id_f=&buffer_file_dm_bitext($background_bitext_f,\@lines_dm_bitext_f,$last_sent_id_f,1);
#	    }
#	    if($dict_size>0) {
#		$last_sent_id_f=&buffer_file_dm_bitext($dict_bitext_f,\@lines_dm_bitext_f,$last_sent_id_f,0);
#	    }
	    # e:
	    $last_sent_id_e=&buffer_file_dm_bitext($bitext_e,\@lines_dm_bitext_e,$last_sent_id_e,1);
#	    if($background_size>0) {
#		$last_sent_id_e=&buffer_file_dm_bitext($background_bitext_e,\@lines_dm_bitext_e,$last_sent_id_e,1);
#	    }
#	    if($dict_size>0) {
#		$last_sent_id_e=&buffer_file_dm_bitext($dict_bitext_e,\@lines_dm_bitext_e,$last_sent_id_e,0);
#	    }
	    # refined alignments:
	    #my $start_dict=$corpus_size+$background_size+1;
	    my $start_dict=$corpus_size + 1;
#	    &buffer_file("$experiment_dir/align.$aligner/refined.$refinement_strategy/model/aligned.$refinement_strategy",\@lines_dm_bitext_a,0);
	    &buffer_file_dm_alignment($bitext_f, $bitext_e, $bitext_a,\@lines_dm_bitext_a,$start_dict);

#	}
#    }
    &write_buffer_to_file(\@lines_dm_bitext_f,"$experiment_dir/dm/corpus/dm_aligned.$f_suffix");
    undef @lines_dm_bitext_f;
    &write_buffer_to_file(\@lines_dm_bitext_e,"$experiment_dir/dm/corpus/dm_aligned.$e_suffix");
    undef @lines_dm_bitext_e;
    &write_buffer_to_file(\@lines_dm_bitext_a,"$experiment_dir/dm/model/aligned.grow-diag-final");
    undef @lines_dm_bitext_a;

    my $aligner_string="$experiment_dir/dm/model/aligned.grow-diag-final";
    my $moses_orientation_parameter='';
    if($use_moses_orientation) {
        $moses_orientation_parameter='--moses-orientation ';
    }
    if($use_dlr) {
        $moses_orientation_parameter.='--use-dlr ';
    }
    if($use_hdm) {
        $moses_orientation_parameter.='--use-hdm ';
    }

    my $lex_dm_call="$external_path/build/bitext_models/moses-build-distortion-extract-file-multiple-alignments.pl $moses_orientation_parameter--f=$f_suffix --e=$e_suffix --dm-lang=$dm_languages --sigma=$dm_interpolation --external-bin-dir=$external_path/ --corpus=$experiment_dir/dm/corpus/dm_aligned --align-files=$aligner_string --experiment-dir=$experiment_dir";
    print STDERR "$lex_dm_call\n";
    system($lex_dm_call);

    my @dm_langs=split(/\,/,$dm_languages);
    for(my $l=0; $l<@dm_langs; $l++) {
        my $dm_lang=$dm_langs[$l];
        system("mv $experiment_dir/dm/dm\_$dm_lang\_$dm_interpolation.gz $experiment_dir/models/model/");
        if($use_hdm) {
            system("mv $experiment_dir/dm/hdm\_$dm_lang\_$dm_interpolation.gz $experiment_dir/models/model/");
        }
    }
}

system("mkdir $experiment_dir/models/corpus/");
system("cp $experiment_dir/lex_trans/corpus/lex_aligned.$f_suffix $experiment_dir/models/corpus/bitext.$f_suffix");
system("cp $experiment_dir/lex_trans/corpus/lex_aligned.$e_suffix $experiment_dir/models/corpus/bitext.$e_suffix");
system("cp $experiment_dir/lex_trans/model/aligned.grow-diag-final $experiment_dir/models/model/aligned.grow-diag-final");
system("cp $experiment_dir/lex_trans/corpus/lex_aligned.$f_suffix $experiment_dir/models/model/aligned.$f_suffix");
system("cp $experiment_dir/lex_trans/corpus/lex_aligned.$e_suffix $experiment_dir/models/model/aligned.$e_suffix");

if($clean_up) {
    print STDERR "rm -rf $experiment_dir/lex_trans\n";
    system("rm -rf $experiment_dir/lex_trans");
}

if($build_phrase_table) {
    my $call_phrase_table="$moses_train_script --root-dir=$experiment_dir/models --external-bin-dir=$external_path/external_binaries --corpus=$experiment_dir/models/corpus/bitext --f=$f_suffix --e=$e_suffix --first-step=5 --last-step=6 --alignment=grow-diag-final >& $experiment_dir/err.phrase_table.log";
    print STDERR "$call_phrase_table\n";
    system($call_phrase_table);
} else {
    exit(-1);
}


my $phrasetable_format=&determine_phrasetable_format("$experiment_dir/models/model/phrase-table.gz");

open(F,">$experiment_dir/models/model/phrase-table.config_info");
print F join("\n",@config_pt_lines), "\n";
close(F);


my $new_phrase_table='phrase-table.new.gz';


# FIX

my $requires_pos_tags=1;
my $part_of_speech_included=0;
#if($requires_pos_tags && defined($pos_corpus_stem)) {
#    my $bitext_pos_flags="--bitext-pos-f=$experiment_dir/models/model/aligned_pos.$f_suffix --bitext-pos-e=$experiment_dir/models/model/aligned_pos.$e_suffix";
#
#    my $pos_call="$OISTERHOME/build/bitext_models/scripts/add-phrase-table-part-of-speech.pl --pt=$experiment_dir/models/model/phrase-table.gz --pt-pos=$experiment_dir/models/model/$new_phrase_table --bitext-f=$experiment_dir/models/model/aligned.$f_suffix --bitext-e=$experiment_dir/models/model/aligned.$e_suffix --extract-positions-file=$experiment_dir/dm/moses-extract/model/extract.$max_phrase_length.gz $bitext_pos_flags";
#    print STDERR "$pos_call\n";
#    system($pos_call);
#
#    system("mv $experiment_dir/models/model/$new_phrase_table $experiment_dir/models/model/phrase-table.gz");
#    if(-e "$experiment_dir/models/model/phrase-table.new.config\_info") {
#	system("mv $experiment_dir/models/model/phrase-table.new.config\_info $experiment_dir/models/model/phrase-table.config\_info");
#    }
#    $part_of_speech_included=1;
#}


#if($sparse_feature_options ne '') {
##    $new_phrase_table="phrase-table.sparse-feat.gz";
#
#    my $sparse_call="$OISTERHOME/build/bitext_models/scripts/add-phrase-table-sparse-features.pl --bitext-src=$experiment_dir/models/model/aligned.$f_suffix --bitext-trg=$experiment_dir/models/model/aligned.$e_suffix --phrase-table=$experiment_dir/models/model/phrase-table.gz --phrase-table-sparse=$experiment_dir/models/model/$new_phrase_table --feature-key=$experiment_dir/models/model/phrase-table.features $sparse_feature_options";
#    print STDERR "$sparse_call\n";
#    system($sparse_call);
#    system("mv $experiment_dir/models/model/$new_phrase_table $experiment_dir/models/model/phrase-table.gz");
#    if(-e "$experiment_dir/models/model/phrase-table.new.config\_info") {
#	system("mv $experiment_dir/models/model/phrase-table.new.config\_info $experiment_dir/models/model/phrase-table.config\_info");
#    }
#}


if(defined($phrase_table_smoothing_string) && $phrase_table_smoothing_string ne '') {
    my(@smoothing_flags)=split(/\,/,$phrase_table_smoothing_string);
    for(my $i=0; $i<@smoothing_flags; $i++) {
	$smoothing_flags[$i]="\-\-$smoothing_flags[$i]";
    }
    my $smoothing_args=join(' ',@smoothing_flags);

    my $smoothing_call="$external_path/build/bitext_models/add-phrase-table-smoothing.pl --pt=$experiment_dir/models/model/phrase-table.gz --pt-smooth=$experiment_dir/models/model/$new_phrase_table $smoothing_args --external-path=$external_path";
    print STDERR "$smoothing_call\n";
    system($smoothing_call);
    system("mv $experiment_dir/models/model/$new_phrase_table $experiment_dir/models/model/phrase-table.gz");
    if(-e "$experiment_dir/models/model/phrase-table.new.config\_info") {
	system("mv $experiment_dir/models/model/phrase-table.new.config\_info $experiment_dir/models/model/phrase-table.config\_info");
    }
}

=begin REMOVE
if(defined($lexical_weighting_string) && $lexical_weighting_string ne '') {
    my $requires_ibm1=0;
    my $requires_rf=0;
    my(@lexical_weighting_flags)=split(/\,/,$lexical_weighting_string);
    for(my $i=0; $i<@lexical_weighting_flags; $i++) {
	if($lexical_weighting_flags[$i]=~/\-ibm1/) {
	    $requires_ibm1=1;
	}
	if($lexical_weighting_flags[$i]=~/\-rf/) {
	    $requires_rf=1;
	}
	$lexical_weighting_flags[$i]="\-\-$lexical_weighting_flags[$i]";
    }
    if($requires_ibm1) {
	push(@lexical_weighting_flags,"--ibm1-f2e=$experiment_dir/models/model/ibm1.f2e");
	push(@lexical_weighting_flags,"--ibm1-e2f=$experiment_dir/models/model/ibm1.e2f");
    }
    if($requires_rf) {
	push(@lexical_weighting_flags,"--lex-f2e=$experiment_dir/models/model/lex.f2e");
	push(@lexical_weighting_flags,"--lex-e2f=$experiment_dir/models/model/lex.e2f");
    }

    my $lexical_weighting_args=join(' ',@lexical_weighting_flags);


    if($requires_ibm1 && !$ibm1_exists) {
	my $mgiza_flag='';
	if($use_mgiza) {
	    $mgiza_flag="\-\-mgiza \-\-mgiza-cpus=5 ";
	}
	my $ibm1_call="$OISTERHOME/build/bitext_models/scripts/ibm1-aligner.pl $mgiza_flag --experiment-dir=$experiment_dir/models/model --external-path=$external_path --f=$f_suffix --e=$e_suffix --num-iterations=$num_ibm1_iterations --corpus-stem=aligned";
	print STDERR "$ibm1_call\n";
	system($ibm1_call);
	$ibm1_exists=1;
    }

    my $lexical_weighting_call="$OISTERHOME/build/bitext_models/scripts/add-phrase-table-lexical-weights.pl --pt=$experiment_dir/models/model/phrase-table.gz --pt-lex=$experiment_dir/models/model/$new_phrase_table $lexical_weighting_args";
    print STDERR "$lexical_weighting_call\n";
    system($lexical_weighting_call);

    system("mv $experiment_dir/models/model/$new_phrase_table $experiment_dir/models/model/phrase-table.gz");
    if(-e "$experiment_dir/models/model/phrase-table.new.config\_info") {
	system("mv $experiment_dir/models/model/phrase-table.new.config\_info $experiment_dir/models/model/phrase-table.config\_info");
    }
}


if(defined($pos_trans_string) && $pos_trans_string ne '' && defined($pos_corpus_stem)) {

    my $postrans_call="$OISTERHOME/build/bitext_models/scripts/add-phrase-table-part-of-speech-translations.pl --pt=$experiment_dir/models/model/phrase-table.gz --pt-pos-trans=$experiment_dir/models/model/$new_phrase_table --bitext-f=$experiment_dir/models/model/aligned.$f_suffix --bitext-e=$experiment_dir/models/model/aligned.$e_suffix --bitext-pos-f=$experiment_dir/models/model/aligned_pos.$f_suffix --bitext-pos-e=$experiment_dir/models/model/aligned_pos.$e_suffix --alignment=$experiment_dir/dm/moses-extract/model/aligned.grow-diag-final --extract-positions-file=$experiment_dir/dm/moses-extract/model/extract.$max_phrase_length.gz --probs=$pos_trans_string";
    print STDERR "$postrans_call\n";
    system($postrans_call);

    system("mv $experiment_dir/models/model/$new_phrase_table $experiment_dir/models/model/phrase-table.gz");
    if(-e "$experiment_dir/models/model/phrase-table.new.config\_info") {
	system("mv $experiment_dir/models/model/phrase-table.new.config\_info $experiment_dir/models/model/phrase-table.config\_info");
    }
}

if(defined($bilm_string) && $bilm_string ne '') {
    my @bilm_args=split(/\,/,$bilm_string);
    my %bilm_flags;
    for(my $i=0; $i<@bilm_args; $i++) {
	$bilm_flags{$bilm_args[$i]}=1;
    }
    my @bilms;
    push(@bilms,"--bitext-lm=bilm_w-w.lm") if(exists($bilm_flags{'w-w'}));
    push(@bilms,"--bitext-poslm=bilm_p-p.lm") if(exists($bilm_flags{'p-p'}) && $part_of_speech_included);
    push(@bilms,"--bitext-word2poslm=bilm_w-p.lm") if(exists($bilm_flags{'w-p'}) && $part_of_speech_included);
    push(@bilms,"--bitext-pos2wordlm=bilm_p-w.lm") if(exists($bilm_flags{'p-w'}) && $part_of_speech_included);
    push(@bilms,"--bitext-word2xlm=bilm_w-x.lm") if(exists($bilm_flags{'w-x'}));
    push(@bilms,"--bitext-pos2xlm=bilm_p-x.lm") if(exists($bilm_flags{'p-x'}) && $part_of_speech_included);
    my $bilm_args_string=join(' ',@bilms);

    my $bilm_call="$OISTERHOME/build/bitext_models/scripts/add-phrase-table-bilingual-lm.pl --pt=$experiment_dir/models/model/phrase-table.gz --pt-bilm=$experiment_dir/models/model/$new_phrase_table --model-path=$experiment_dir/models/model/ --bitext-f=$experiment_dir/models/model/aligned.$f_suffix --bitext-e=$experiment_dir/models/model/aligned.$e_suffix --bitext-pos-f=$experiment_dir/models/model/aligned_pos.$f_suffix --bitext-pos-e=$experiment_dir/models/model/aligned_pos.$e_suffix --alignment=$experiment_dir/models/model/aligned.grow-diag-final $bilm_args_string --order=5 --external-path=$external_path";
    print STDERR "$bilm_call\n";
    system($bilm_call);

    system("mv $experiment_dir/models/model/$new_phrase_table $experiment_dir/models/model/phrase-table.gz");
    if(-e "$experiment_dir/models/model/phrase-table.new.config\_info") {
	system("mv $experiment_dir/models/model/phrase-table.new.config\_info $experiment_dir/models/model/phrase-table.config\_info");
    }
}



if($sigtest_filter ne '0') {
    my $sigfilter_phrase_table='phrase-table.sigfilter.gz';

    my $salm="$external_path/external_binaries/IndexSA.O64";
    my $call="cd $experiment_dir/models/model/; $salm aligned.$f_suffix; cd -";
    print STDERR "$call\n";
    system($call);
    $call="cd $experiment_dir/models/model/; $salm aligned.$e_suffix; cd -";
    print STDERR "$call\n";
    system($call);

    $call="cd $experiment_dir/models/model; cat phrase-table.gz | gunzip | $external_path/moses/contrib/sigtest-filter/filter-pt -f aligned.$f_suffix -e aligned.$e_suffix -l $sigtest_filter | gzip > $sigfilter_phrase_table; cd -";
    print STDERR "$call\n";
    system($call);

    if($clean_up) {
	print STDERR "Cleaning up:\n";
	$call="rm -rf $experiment_dir/models/model/*.sa_corpus";
	print STDERR "$call\n";
	system($call);
	$call="rm -rf $experiment_dir/models/model/*.sa_offset";
	print STDERR "$call\n";
	system($call);
	$call="rm -rf $experiment_dir/models/model/*.sa_suffix";
	print STDERR "$call\n";
	system($call);
	$call="rm -rf $experiment_dir/models/model/*.id_voc";
	print STDERR "$call\n";
	system($call);
    }
}
=end REMOVE
=cut



if($clean_up) {
    	my $call="rm -rf $experiment_dir/dm/corpus";
	print STDERR "$call\n";
	system($call);
	$call="rm -rf $experiment_dir/dm/model";
	print STDERR "$call\n";
	system($call);
	$call="rm -rf $experiment_dir/dm/moses-extract/model/extract.100.gz";
	print STDERR "$call\n";
	system($call);
	$call="rm -rf $experiment_dir/models/corpus";
	print STDERR "$call\n";
	system($call);
	$call="rm -rf $experiment_dir/models/model/extract.inv.sorted.gz";
	print STDERR "$call\n";
	system($call);
}


sub determine_phrasetable_format {
    my($phrase_table)=@_;

    if($phrase_table=~/\.gz$/o) {
	open F, "<:gzip", $phrase_table, or die("can't open $phrase_table: $!\n");
    } else {
	open(F,"<$phrase_table") || die("can't open $phrase_table: $!\n");
    }

    my $format='undef';
    my $c=0;
    while(defined(my $line=<F>)) {
	chomp($line);
	$c++;
	last if($c>50);
	my @entries=split(/ \|\|\| /,$line);
	if(@entries==5) {
	    if($entries[2]=~/^[ 0-9\.e\-]+$/
	       && $entries[3]=~/^[0-9]+\-[0-9]+/
	       && $entries[4]=~/[0-9]+ [0-9]+ [0-9]+$/) {
		$format='f_e_p_a_c';
		last;
	    } elsif($entries[2]=~/^[ \(\)0-9\,]+$/
		    && $entries[3]=~/^[ \(\)0-9\,]+$/
		    && $entries[4]=~/^[ 0-9\.e\-]+$/) {
		$format='f_e_af_ae_p';
		last;
	    }
	}
    }
    close(F);
    return $format;
}

sub buffer_file {
    my($file,$buffer,$offset)=@_;
    for(my $i=0; $i<$offset; $i++) {
	push(@$buffer,"");
    }
    open(F,"<$file")||die("can't open file $file: $!\n");
    while(defined(my $line=<F>)) {
	push(@$buffer,$line);
    }
    close(F);
}


sub write_buffer_to_file {
    my($buffer,$file)=@_;

    if($file=~/\.gz$/o) {
        open F, ">:gzip", $file, or die("can't open $file: $!\n");
    } else {
        open(F,">$file") || die("can't open $file: $!\n");
    }

    for(my $i=0; $i<@$buffer; $i++) {
	print F $buffer->[$i];
    }
    close(F);
}

sub number_lines {
    my($text)=@_;

    my $num_lines_text=0;
    open(F,"<$text")||die("can't open file $text: $!\n");
    while(defined(my $line=<F>)) {
	$num_lines_text++;
    }
    close(F);

    return $num_lines_text;
}

sub buffer_file_dm_alignment {
    my($file_f,$file_e,$file_a,$buffer,$start_dict)=@_;
    my $sent_id=0;

    open(F,"<$file_f")||die("can't open file $file_f: $!\n");
    open(E,"<$file_e")||die("can't open file $file_e: $!\n");
    open(A,"<$file_a")||die("can't open file $file_a: $!\n");
    while(defined(my $line_f=<F>) && defined(my $line_e=<E>) && defined(my $line_a=<A>)) {
	chomp($line_f);
	chomp($line_e);
	chomp($line_a);
	$sent_id++;
	if($sent_id<$start_dict) {
	    my @tokens_f=split(/ /,$line_f);
	    my $f_end=@tokens_f+1;
	    my @tokens_e=split(/ /,$line_e);
	    my $e_end=@tokens_e+1;
	    my @tokens_a=split(/ /,$line_a);
	    for(my $i=0; $i<@tokens_a; $i++) {
		my($f_index,$e_index)=split(/\-/,$tokens_a[$i]);
		$f_index++;
		$e_index++;
		$tokens_a[$i]="$f_index\-$e_index";
	    }
	    unshift(@tokens_a,'0-0');
	    push(@tokens_a,"$f_end\-$e_end");
	    $line_a=join(' ',@tokens_a);
	}

	push(@$buffer,"0-0\n");
	push(@$buffer,"$line_a\n");
	push(@$buffer,"0-0\n");
    }
    close(F);
}

sub buffer_file_dm_bitext {
    my($file,$buffer,$last_sent_id,$add_sent_tags)=@_;
    $add_sent_tags||=0;

    open(F,"<$file")||die("can't open file $file: $!\n");
    while(defined(my $line=<F>)) {
	chomp($line);
	$last_sent_id++;
	my @tokens=split(/ /,$line);
	if($add_sent_tags) {
	    unshift(@tokens,'<s>');
	    push(@tokens,'</s>');
	}
	for(my $i=0; $i<@tokens; $i++) {
	    $tokens[$i]="$i:$tokens[$i]";
	}
	my $position_line=join(' ',@tokens) . "\n";;
	push(@$buffer,"<sent_id=$last_sent_id>\n");
	push(@$buffer,$position_line);
	push(@$buffer,"<\/sent_id>\n");
    }
    close(F);
    return $last_sent_id;
}



