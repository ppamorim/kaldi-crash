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

#include "online2-pcm-nnet3-decode-faster.h"

using namespace std;

namespace kaldi {

std::string LatticeToString(const Lattice &lat, const fst::SymbolTable &word_syms) {
  LatticeWeight weight;
  std::vector<int32> alignment;
  std::vector<int32> words;
  GetLinearSymbolSequence(lat, &alignment, &words, &weight);

  std::ostringstream msg;
  for (size_t i = 0; i < words.size(); i++) {
    std::string s = word_syms.Find(words[i]);
    if (s.empty()) {
      KALDI_WARN << "Word-id " << words[i] << " not in symbol table.";
      msg << "<#" << std::to_string(i) << "> ";
    } else
      msg << s << " ";
  }
  return msg.str();
}

std::string GetTimeString(int32 t_beg, int32 t_end, BaseFloat time_unit) {
  char buffer[100];
  double t_beg2 = t_beg * time_unit;
  double t_end2 = t_end * time_unit;
  snprintf(buffer, 100, "%.2f %.2f", t_beg2, t_end2);
  return std::string(buffer);
}

int32 GetLatticeTimeSpan(const Lattice& lat) {
  std::vector<int32> times;
  LatticeStateTimes(lat, &times);
  return times.back();
}

std::string LatticeToString(const CompactLattice &clat, const fst::SymbolTable &word_syms) {
  if (clat.NumStates() == 0) {
    KALDI_WARN << "Empty lattice.";
    return "";
  }
  CompactLattice best_path_clat;
  CompactLatticeShortestPath(clat, &best_path_clat);

  Lattice best_path_lat;
  ConvertLattice(best_path_clat, &best_path_lat);
  return LatticeToString(best_path_lat, word_syms);
}

PcmDecoder::PcmDecoder(int argc, char *argv[]) {

  try {

    using namespace kaldi;
    using namespace fst;

    typedef kaldi::int32 int32;
    typedef kaldi::int64 int64;

    this->frame_offset = 0;
    this->samp_count = 0;

    const char *usage =
        "Reads in audio from a buffer and performs online\n"
        "decoding with neural nets (nnet3 setup), with iVector-based\n"
        "speaker adaptation and endpointing.\n"
        "Note: some configuration values and inputs are set via config\n"
        "files whose filenames are passed as options\n"
        "\n"
        "Usage: online2-pcm-nnet3-decode-faster [options] <nnet3-in> "
        "<fst-in> <word-symbol-table>\n";

    ParseOptions po(usage);

    // feature_opts includes configuration for the iVector adaptation,
    // as well as the basic features.
    this->feature_opts_ = new OnlineNnet2FeaturePipelineConfig();
    this->decodable_opts_ = new nnet3::NnetSimpleLoopedComputationOptions();
    this->decoder_opts_ = new LatticeFasterDecoderConfig();
    this->endpoint_opts_ = new OnlineEndpointConfig();

    BaseFloat chunk_length_secs = 0.18;
    BaseFloat output_period = 1;
    this->samp_freq = 16000.0;
    int read_timeout = 3;
    bool produce_time = false;

    po.Register("samp-freq", &samp_freq,
                "Sampling frequency of the input signal (coded as 16-bit slinear).");
    po.Register("chunk-length", &chunk_length_secs,
                "Length of chunk size in seconds, that we process.");
    po.Register("output-period", &output_period,
                "How often in seconds, do we check for changes in output.");
    po.Register("num-threads-startup", &g_num_threads,
                "Number of threads used when initializing iVector extractor.");
    po.Register("read-timeout", &read_timeout,
                "Number of seconds of timout for TCP audio data to appear on the stream. Use -1 for blocking.");
    po.Register("produce-time", &produce_time,
                "Prepend begin/end times between endpoints (e.g. '5.46 6.81 <text_output>', in seconds)");
    
    feature_opts_->Register(&po);
    decodable_opts_->Register(&po);
    decoder_opts_->Register(&po);
    endpoint_opts_->Register(&po);

    po.Read(argc, argv);

    if (po.NumArgs() != 3) {
      po.PrintUsage();
      return;
    }

    this->samp_count = 0;// this is used for output refresh rate
    this->chunk_len = static_cast<size_t>(chunk_length_secs * samp_freq);
    this->check_period = static_cast<int32>(samp_freq * output_period);
    this->check_count = this->check_period;

    std::string nnet3_rxfilename = po.GetArg(1),
        fst_rxfilename = po.GetArg(2),
        word_syms_filename = po.GetArg(3);

    this->feature_info_ = new OnlineNnet2FeaturePipelineInfo(*feature_opts_);

    this->frame_shift = feature_info_->FrameShiftInSeconds();
    this->frame_subsampling = decodable_opts_->frame_subsampling_factor;

    KALDI_VLOG(1) << "Loading AM...";

    this->trans_model_ = new TransitionModel();
    this->am_nnet_ = new nnet3::AmNnetSimple();

    {
      bool binary;
      this->ki = new Input(nnet3_rxfilename, &binary);
      trans_model_->Read(ki->Stream(), binary);
      am_nnet_->Read(ki->Stream(), binary);
      SetBatchnormTestMode(true, &(am_nnet_->GetNnet()));
      SetDropoutTestMode(true, &(am_nnet_->GetNnet()));
      nnet3::CollapseModel(nnet3::CollapseModelConfig(), &(am_nnet_->GetNnet()));
    }

    // this object contains precomputed stuff that is used by all decodable
    // objects.  It takes a pointer to am_nnet because if it has iVectors it has
    // to modify the nnet to accept iVectors at intervals.
    this->decodable_info_ = new nnet3::DecodableNnetSimpleLoopedInfo(
      *decodable_opts_,
      am_nnet_);

    KALDI_VLOG(1) << "Loading FST...";

    fst::Fst<fst::StdArc> *decode_fst = ReadFstKaldiGeneric(fst_rxfilename);

    if (!word_syms_filename.empty())
      if (!(this->word_syms_ = fst::SymbolTable::ReadText(word_syms_filename)))
        KALDI_ERR << "Could not read symbol table from file "
                  << word_syms_filename;

    this->feature_pipeline_ = new OnlineNnet2FeaturePipeline(*feature_info_);

    SingleUtteranceNnet3Decoder *decoder = new SingleUtteranceNnet3Decoder(
        *decoder_opts_,
        *trans_model_,
        *decodable_info_,
        *decode_fst, 
        feature_pipeline_);
    this->decoder_ = decoder;

  } catch (const std::exception &e) {
    std::cerr << e.what();
  }

}

} // namespace kaldi