#!/usr/bin/perl -w

use strict;
use warnings;
use File::Basename;

my($src_file,@ref_files)=@ARGV;

my $script_name = basename($0);
print STDERR "$script_name pid=$$\n";

my $max_n=4;
my $bleu_strategy='plain';
my $bp_strategy='closest';

my @O;
my @O_f;

my @ref_ngrams;
my @ref_lengths;
&get_ref_ngrams(\@ref_ngrams,\@ref_files,\@ref_lengths);

my @src_lengths;
open(F,"<$src_file");
my $sent_id=-1;
while(defined(my $line=<F>)) {
    chomp($line);
    $sent_id++;
    my @tokens=split(/ +/,$line);
    $src_lengths[$sent_id]=@tokens;
}
close(F);


# initialize O and O_f
for(my $i=0; $i<($max_n*2)+1; $i++) {
    $O[0][$i]=0.1;
}
$O_f[0]=0.1;


my $prev_sent_id;
my $running_sent_id=0;
while(defined(my $line=<STDIN>)) {
    chomp($line);
    if($line=~/^([0-9]+ \|\|\|) (\|\|\| .*)$/) {
        $line="$1 NIL $2";
    }

    if($line=~/^([0-9]+) \|\|\| (.+) \|\|\| (.+)$/) {
        my $sent_id=$1;
        my $translation=$2;
        my $scores_string=$3;

        if(!defined($prev_sent_id) || $sent_id ne $prev_sent_id) {
            my @correct;
            my @total;
            my $ref_length;
            my $bleu=&compute_bleu($sent_id,$translation,\@ref_ngrams,\@ref_lengths,$bleu_strategy,$bp_strategy,\@correct,\@total,\$ref_length);

            for(my $i=0; $i<$max_n; $i++) {
                $O[($sent_id+1)][($i*2)]=0.9*($O[$sent_id][($i*2)]+$correct[$i+1]);
                $O[($sent_id+1)][($i*2)+1]=0.9*($O[$sent_id][($i*2)+1]+$total[$i+1]);
            }
            $O[($sent_id+1)][($max_n*2)]=0.9*($O[$sent_id][($max_n*2)]+$ref_length);

            $O_f[($sent_id+1)]=0.9*($O_f[$sent_id]+$src_lengths[$sent_id]);

        }

        $prev_sent_id=$sent_id;
    } else {
        my $translation=$line;
        $translation='NIL' if($translation=~/^[\s\t]*$/);

        my $sent_id=$running_sent_id;
        my @correct;
        my @total;
        my $ref_length;
        my $bleu=&compute_bleu($sent_id,$translation,\@ref_ngrams,\@ref_lengths,$bleu_strategy,$bp_strategy,\@correct,\@total,\$ref_length);

        for(my $i=0; $i<$max_n; $i++) {
            if(!defined($O[$sent_id][($i*2)])) {
                print STDERR "UNDEF: O[$sent_id][($i*2)]\n";
                die;
            }

            if(!defined($correct[$i+1])) {
                print STDERR "UNDEF: O[$sent_id][($i*2)]\n correct[$i+1]\n";
                die;
            }

            $O[($sent_id+1)][($i*2)]=0.9*($O[$sent_id][($i*2)]+$correct[$i+1]);
            $O[($sent_id+1)][($i*2)+1]=0.9*($O[$sent_id][($i*2)+1]+$total[$i+1]);
        }
        $O[($sent_id+1)][($max_n*2)]=0.9*($O[$sent_id][($max_n*2)]+$ref_length);

        $O_f[($sent_id+1)]=0.9*($O_f[$sent_id]+$src_lengths[$sent_id]);
        $running_sent_id++;
    }
}

for(my $i=0; $i<@O; $i++) {
    print "$i ||| ", join(' ',@{ $O[$i] }), ' ', $O_f[$i], "\n";
}


sub get_ref_ngrams {
    my($ref_ngrams,$ref_files,$ref_lengths)=@_;

    my @ref_ngrams_raw;
    my $ref=-1;
    for(my $i=0; $i<@$ref_files; $i++) {
        $ref++;
        open(F,"<$ref_files->[$i]");
        my $sent_id=-1;
        while(defined(my $line=<F>)) {
            chomp($line);
            $sent_id++;
            my @tokens=split(/ +/,$line);
            $ref_lengths->[$sent_id]{$ref}=@tokens;
            for(my $i=0; $i<@tokens; $i++) {
                for(my $j=$i; $j<@tokens && $j<$i+$max_n; $j++) {
                    my $ngram=join(' ',@tokens[$i..$j]);
                    $ref_ngrams_raw[$sent_id]{$ngram}{$ref}++;
                }
            }
        }
        close(F);
    }

    for(my $sent_id=0; $sent_id<@ref_ngrams_raw; $sent_id++) {
        foreach my $ngram (keys %{ $ref_ngrams_raw[$sent_id] }) {
            my $clipped_count;
            foreach my $ref (keys %{ $ref_ngrams_raw[$sent_id]{$ngram} }) {
                if(!defined($clipped_count) || $ref_ngrams_raw[$sent_id]{$ngram}{$ref}>$clipped_count) {
                    $clipped_count=$ref_ngrams_raw[$sent_id]{$ngram}{$ref};
                }
            }
            $$ref_ngrams[$sent_id]{$ngram}=$clipped_count;
        }
    }
}

sub min {
    return $_[0] if($_[0]<$_[1]);
    return $_[1];
}

sub max {
    return $_[0] if($_[0]>$_[1]);
    return $_[1];
}

sub compute_brevaty_penalty {
    my($c,$r)=@_;
    if($c>$r) {
        return 1;
    } else {
           return exp(1-($r/$c));
    }
}


sub compute_bleu {
    my($sent_id,$translation,$ref_ngrams,$ref_lengths,$bleu_strategy,$bp_strategy,$correct,$total,$ref_length)=@_;

    my $bleu;
    if($bleu_strategy eq 'liang') {
        my @bleu_i;
        for(my $i=1; $i<=$max_n; $i++) {
            my @dummy_correct;
            my @dummy_total;
            my $dummy_ref_length;
            if($i==$max_n) {
                $bleu_i[$i]=&compute_sent_bleu($sent_id,$translation,$i,$ref_ngrams,$ref_lengths,$bp_strategy,$correct,$total,$ref_length);
            } else {
                $bleu_i[$i]=&compute_sent_bleu($sent_id,$translation,$i,$ref_ngrams,$ref_lengths,$bp_strategy,\@dummy_correct,\@dummy_total,\$dummy_ref_length);
            }
            $bleu_i[$i]/=(2**($max_n-$i+1));
            $bleu+=$bleu_i[$i];
        }

    } elsif($bleu_strategy eq 'plain') {
       $bleu=&compute_sent_bleu($sent_id,$translation,$max_n,$ref_ngrams,$ref_lengths,$bp_strategy,$correct,$total,$ref_length);
    }

    return $bleu;
}

sub compute_sent_bleu {
    my($sent_id,$translation,$max_n,$ref_ngrams,$ref_lengths,$bp_strategy,$correct_ngrams,$total_ngrams,$ref_length)=@_;

    my @tokens=split(/ +/,$translation);
    my $tst_length=@tokens;

    if($tst_length==0 || $translation eq 'NIL') {
        for(my $n=1; $n<=$max_n; $n++) {
            $correct_ngrams->[$n]=0;
            $total_ngrams->[$n]=0;
        }
        $$ref_length=&compute_ref_seg_length($bp_strategy,0,\%{ $ref_lengths->[$sent_id] });
        return 0;
    }

    my @tst_ngrams;
    for(my $i=0; $i<@tokens; $i++) {
        for(my $j=$i; $j<@tokens && $j<$i+$max_n; $j++) {
            my $ngram=join(' ',@tokens[$i..$j]);
            my $n=$j-$i+1;
            $tst_ngrams[$n]{$ngram}++;
        }
    }

    my @prec_n;
    my $max_n_effective=&min($max_n,&max($tst_length,&compute_ref_seg_length('longest',$tst_length,\%{ $ref_lengths->[$sent_id] })));

    for(my $n=1; $n<=$max_n; $n++) {
        $correct_ngrams->[$n]=0;
        $total_ngrams->[$n]=0;
        $prec_n[$n]=0;
    }

    for(my $n=1; $n<=$max_n_effective; $n++) {
        my $total=0;
        $prec_n[$n]=0;
        foreach my $ngram (keys %{ $tst_ngrams[$n] }) {
            if(exists($$ref_ngrams[$sent_id]{$ngram})) {
              $prec_n[$n]+=&min($tst_ngrams[$n]{$ngram},$$ref_ngrams[$sent_id]{$ngram});
            }
            $total+=$tst_ngrams[$n]{$ngram};
        }
        $correct_ngrams->[$n]=$prec_n[$n];
        if($total==0) {
            $prec_n[$n]=1;
        } else {
            $prec_n[$n]/=$total;
        }
        $total_ngrams->[$n]=$total;
    }
    my $prec=0;
    for(my $n=1; $n<=$max_n_effective; $n++) {
        if($prec_n[$n]==0) {
            $prec=0;
            last;
        } else {
            $prec+=(1/$max_n_effective)*log($prec_n[$n]);
        }
    }
    
    $$ref_length=&compute_ref_seg_length($bp_strategy,$tst_length,\%{ $ref_lengths->[$sent_id] });
    my $bp=&compute_brevaty_penalty($tst_length,$$ref_length);

    my $bleu=$bp*exp($prec);
    return $bleu;

}
    


sub compute_ref_seg_length {
    my($brev_penality_strategy,$tst_length,$ref_lengths)=@_;

    if($brev_penality_strategy eq 'closest') {
        my $distance;
        my $min_dist_ref_length;
        foreach my $ref_id (keys %$ref_lengths) {
            my $ref_id_length=$$ref_lengths{$ref_id};

            if(!defined($distance)) {
                $distance=abs($tst_length-$ref_id_length);
                $min_dist_ref_length=$ref_id_length;
            } elsif(abs($tst_length-$ref_id_length)<$distance) {
                $distance=abs($tst_length-$ref_id_length);
                $min_dist_ref_length=$ref_id_length;
            } elsif(abs($tst_length-$ref_id_length)==$distance && $ref_id_length<$min_dist_ref_length) {
                $distance=abs($tst_length-$ref_id_length);
                $min_dist_ref_length=$ref_id_length;
            }
        }
        return $min_dist_ref_length;
    } elsif($brev_penality_strategy eq 'shortest') {
        my $min_length;
        foreach my $ref_id (keys %$ref_lengths) {
            my $ref_id_length=$$ref_lengths{$ref_id};
            if(!defined($min_length) || $ref_id_length<$min_length) {
                $min_length=$ref_id_length;
            }
        }
        return $min_length;
    } elsif($brev_penality_strategy eq 'longest') {
        my $max_length;
        foreach my $ref_id (keys %$ref_lengths) {
            my $ref_id_length=$$ref_lengths{$ref_id};
            if(!defined($max_length) || $ref_id_length>$max_length) {
            $max_length=$ref_id_length;
            }
        }
        return $max_length;
    }
}

