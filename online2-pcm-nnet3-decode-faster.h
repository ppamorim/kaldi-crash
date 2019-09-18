// online2bin/online2-pcm-nnet3-decode-faster.cc

// Copyright 2014  Johns Hopkins University (author: Daniel Povey)
//           2016  Api.ai (Author: Ilya Platonov)
//           2018  Polish-Japanese Academy of Information Technology (Author: Danijel Korzinek)

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#include "feat/wave-reader.h"
#include "online2/online-nnet3-decoding.h"
#include "online2/online-nnet2-feature-pipeline.h"
#include "online2/onlinebin-util.h"
#include "online2/online-timing.h"
#include "online2/online-endpoint.h"
#include "fstext/fstext-lib.h"
#include "lat/lattice-functions.h"
#include "util/kaldi-thread.h"
#include "nnet3/nnet-utils.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <poll.h>
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <iostream>

#include <execinfo.h>

#include <stdio.h>
#include <stdlib.h>

using namespace std;

namespace kaldi {


class PcmDecoder {

  public:
    PcmDecoder(int argc, char *argv[]);
    size_t chunk_len;

  private:

    kaldi::OnlineNnet2FeaturePipeline *feature_pipeline_;
    kaldi::SingleUtteranceNnet3Decoder *decoder_;

    OnlineNnet2FeaturePipelineInfo *feature_info_;

    int32 check_period;
    int32 check_count;

    int32 frame_offset;
    BaseFloat frame_shift;
    int32 frame_subsampling;

    int32 samp_count;// this is used for output refresh rate

    BaseFloat samp_freq;

    bool produce_time = false;
    bool input_finalized_ = false;

    fst::SymbolTable *word_syms_;

    OnlineNnet2FeaturePipelineConfig* feature_opts_;
    nnet3::NnetSimpleLoopedComputationOptions* decodable_opts_;
    LatticeFasterDecoderConfig* decoder_opts_;
    OnlineEndpointConfig* endpoint_opts_;

    TransitionModel* trans_model_;
    nnet3::AmNnetSimple* am_nnet_;
    Input* ki;
    nnet3::DecodableNnetSimpleLoopedInfo* decodable_info_;

    OnlineSilenceWeighting* silence_weighting_;
    std::vector<std::pair<int32, BaseFloat>>* delta_weights_;

};


}