#!/bin/sh
g++ -I ./inc -I./ext -mtune=native -std=c++0x -lrt -m64 -flto src/ARPAGramBuilder.cpp src/ByteMGramId.cpp src/C2WArrayTrie.cpp src/Logger.cpp src/W2CArrayTrie.cpp \
              src/ARPATrieBuilder.cpp src/C2DHybridTrie.cpp src/G2DMapTrie.cpp src/main.cpp src/W2CHybridTrie.cpp \
              src/AWordIndex.cpp src/C2DMapTrie.cpp src/H2DMapTrie.cpp src/StatisticsMonitor.cpp src/xxhash.c \
    -o dist/Release__Centos_/back-off-language-model-smt 2>&1 1>compile.log
