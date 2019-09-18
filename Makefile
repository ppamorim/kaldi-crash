
all:

include ../kaldi.mk

TESTFILES =

OBJFILES = online2-pcm-nnet3-decode-faster.o

LIBNAME = kaldi-pcm-decoder

ADDLIBS = ../ivector/kaldi-ivector.a ../nnet3/kaldi-nnet3.a \
          ../chain/kaldi-chain.a ../nnet2/kaldi-nnet2.a \
          ../cudamatrix/kaldi-cudamatrix.a ../decoder/kaldi-decoder.a \
          ../lat/kaldi-lat.a ../hmm/kaldi-hmm.a ../feat/kaldi-feat.a \
          ../transform/kaldi-transform.a ../gmm/kaldi-gmm.a \
          ../tree/kaldi-tree.a ../util/kaldi-util.a ../matrix/kaldi-matrix.a \
          ../base/kaldi-base.a 




include ../makefiles/default_rules.mk

