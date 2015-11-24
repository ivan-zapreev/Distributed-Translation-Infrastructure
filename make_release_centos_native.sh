#!/bin/sh
g++ -I ./inc -I./ext -std=c++0x -lrt -m64 -flto \
    -march=native -mtune=native \
    -O3 \
    -msse2 \
    -funroll-loops \
    -ftree-loop-linear \
    -ftree-loop-im \
    -funswitch-loops \
    -funroll-loops \
    -ffast-math \
    -o dist/Release__Centos_/back-off-language-model-smt \
     src/ARPAGramBuilder.cpp src/ByteMGramId.cpp src/C2WArrayTrie.cpp src/Logger.cpp src/W2CArrayTrie.cpp \
     src/ARPATrieBuilder.cpp src/C2DHybridTrie.cpp src/G2DMapTrie.cpp src/main.cpp src/W2CHybridTrie.cpp \
     src/AWordIndex.cpp src/C2DMapTrie.cpp src/H2DMapTrie.cpp src/StatisticsMonitor.cpp src/xxhash.c
