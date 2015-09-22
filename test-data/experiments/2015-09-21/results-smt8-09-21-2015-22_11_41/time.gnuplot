set term pdf
set output 'time.09-21-2015-22_11_41.pdf'
set xlabel "Model size (Gb)"
set ylabel "Time (CPU Minutes)"
set grid
set auto fix
set offsets graph 0.05, graph 0.15, graph 0.1, graph 0.1
set key right top box
set key samplen 2 spacing 2 font ",8"
plot\
"<paste lm.sizes.info time.g2dm.data" using 4:5 pi -1 lc rgb "blue" title "g2dm" with linespoints,\
"<paste lm.sizes.info time.c2dm.data" using 4:5 pi -1 lc rgb "blue" title "c2dm" with linespoints,\
"<paste lm.sizes.info time.w2ch.data" using 4:5 pi -1 lc rgb "blue" title "w2ch" with linespoints,\
"<paste lm.sizes.info time.w2ca.data" using 4:5 pi -1 lc rgb "blue" title "w2ca" with linespoints,\
"<paste lm.sizes.info time.c2wa.data" using 4:5 pi -1 lc rgb "blue" title "c2wa" with linespoints,\
"<paste lm.sizes.info time.c2dh.data" using 4:5 pi -1 lc rgb "blue" title "c2dh" with linespoints,\
"<paste lm.sizes.info time.kenlm.data" using 4:5 pi -1 lc rgb "green" title "kenlm" with linespoints,\
"<paste lm.sizes.info time.srilm.data" using 4:5 pi -1 lc rgb "red" title "srilm" with linespoints
set output
