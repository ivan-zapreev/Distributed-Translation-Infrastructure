reset 
set term pngcairo
set output 'memory.png'
set xlabel "Model size (Gb)"
set ylabel "MRSS (Gb)"

set for [i=1:6] linetype i dt i
set style line 1 lt 1 linecolor rgb "green" lw 1 pt 1
set style line 2 lt 1 linecolor rgb "red" lw 1 pt 1
set style line 3 lt 2 linecolor rgb "blue" lw 1 pt 2
set style line 4 lt 3 linecolor rgb "blue" lw 1 pt 3
set style line 5 lt 4 linecolor rgb "blue" lw 1 pt 4
set style line 6 lt 5 linecolor rgb "blue" lw 1 pt 5

#set datafile separator " "
set key
set auto x
set xtics 1, 2, 9
set yrange [0:12]
set grid

set key left top Left

#set label "(Disabled)" at -.8, 1.8

plot "<paste  model_size.dat kenlm_mem.dat" using 4:6 ls 1 title "KenLM" with lines,\
  "<paste model_size.dat srilm_mem.dat" using 4:6 ls 2 title "SRILM" with lines ,\
  "<paste model_size.dat owl_w2ca_mem.dat" using 4:6 ls 3 title "w2ca" with lines,\
  "<paste model_size.dat owl_c2wa_mem.dat" using 4:6 ls 4 title "c2wa" with lines,\
  "<paste model_size.dat owl_cmhm_mem.dat" using 4:6 ls 5 title "cmhm" with lines,\
  "<paste model_size.dat owl_w2ch_mem.dat" using 4:6 ls 6 title "w2ch" with lines,
set output

#kenlm_mem.dat		owl_c2wa_time.dat	owl_w2ca_time.dat	srilm_time.dat
#kenlm_time.dat		owl_cmhm_mem.dat	owl_w2ch_mem.dat
#model_size.dat		owl_cmhm_time.dat	owl_w2ch_time.dat
#owl_c2wa_mem.dat	owl_w2ca_mem.dat	srilm_mem.dat
