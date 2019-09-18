#!/bin/bash

rm -r build/run
mkdir -p build

# -O1 \
# -Wall -Wno-sign-compare -Wno-unused-local-typedefs -Wno-deprecated-declarations \
# -Winit-self -DKALDI_DOUBLEPRECISION=0 -DHAVE_EXECINFO_H=1 -DHAVE_CXXABI_H \
# -DHAVE_CLAPACK -msse -msse2 -pthread -g  -Wno-mismatched-tags \

g++ -g -std=c++11 \
 main.cpp \
-L./lib/ \
-lkaldi-pcm-decoder \
-lkaldi-online2 \
-lkaldi-ivector \
-lkaldi-nnet3 \
-lkaldi-chain \
-lkaldi-nnet2 \
-lkaldi-cudamatrix \
-lkaldi-decoder \
-lkaldi-lat \
-lkaldi-fstext \
-lkaldi-hmm \
-lkaldi-feat \
-lkaldi-transform \
-lkaldi-gmm \
-lkaldi-tree \
-lkaldi-util \
-lkaldi-matrix \
-lkaldi-base \
-lfst \
-framework Accelerate \
-lm -lpthread -ldl \
-DKALDI_DOUBLEPRECISION=0 -DHAVE_EXECINFO_H=1 -DHAVE_CXXABI_H -DHAVE_CLAPACK -msse -msse2 -pthread \
-o build/run