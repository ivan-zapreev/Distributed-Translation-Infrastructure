set term pdf
set output 'mem.09-19-2015-19_07_47.pdf'
set xlabel "Model size (Gb)"
set ylabel "Max RSS (Gb)"
set grid
set auto fix
set offsets graph 0.05, graph 0.05, graph 0.1, graph 0.1
set key left top box
set key samplen 2 spacing 2 font ",8"
plot\
"<paste lm.sizes.info mem.g2dm.data" using 4:5 pi -1 lc rgb "blue" title "g2dm" with linespoints,\
"<paste lm.sizes.info mem.c2dm.data" using 4:5 pi -1 lc rgb "blue" title "c2dm" with linespoints,\
"<paste lm.sizes.info mem.w2ch.data" using 4:5 pi -1 lc rgb "blue" title "w2ch" with linespoints,\
"<paste lm.sizes.info mem.w2ca.data" using 4:5 pi -1 lc rgb "blue" title "w2ca" with linespoints,\
"<paste lm.sizes.info mem.c2wa.data" using 4:5 pi -1 lc rgb "blue" title "c2wa" with linespoints,\
"<paste lm.sizes.info mem.c2dh.data" using 4:5 pi -1 lc rgb "blue" title "c2dh" with linespoints,\
"<paste lm.sizes.info mem.kenlm.data" using 4:5 pi -1 lc rgb "green" title "kenlm" with linespoints,\
"<paste lm.sizes.info mem.srilm.data" using 4:5 pi -1 lc rgb "red" title "srilm" with linespoints
set output
