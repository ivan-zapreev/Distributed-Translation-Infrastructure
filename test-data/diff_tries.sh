#!/bin/sh
../dist/Release__Linux_/back-off-language-model-smt e_00_1000.lm test.txt c2wa info3 | grep -v "CPU" | grep -v "vmsize" | grep -v "Cultivating" > release.c2wa.out
../dist/Release__Linux_/back-off-language-model-smt e_00_1000.lm test.txt cmhm info3  | grep -v "CPU" | grep -v "vmsize" | grep -v "Cultivating" > release.cmhm.out
../dist/Release__Linux_/back-off-language-model-smt e_00_1000.lm test.txt w2ca info3 | grep -v "CPU" | grep -v "vmsize" | grep -v "Cultivating" | grep -v "' memory allocation strategy." > release.w2ca.out
../dist/Release__Linux_/back-off-language-model-smt e_00_1000.lm test.txt w2ch info3 | grep -v "CPU" | grep -v "vmsize" | grep -v "Cultivating" > release.w2ch.out

echo "\n----> c2wa vs. cmhm"
diff release.c2wa.out release.cmhm.out > diff.c2wa.cmhm.out
cat diff.c2wa.cmhm.out
echo "\n----> c2wa vs. w2ca"
diff release.c2wa.out release.w2ca.out > diff.c2wa.w2ca.out
cat diff.c2wa.w2ca.out
echo "\n----> c2wa vs. w2ch"
diff release.c2wa.out release.w2ch.out > diff.c2wa.w2ch.out
cat diff.c2wa.w2ch.out
