#ifndef UEP_UEP_FAST_RUN_HPP
#define UEP_UEP_FAST_RUN_HPP

#include <cstddef>
#include <vector>

struct simulation_params {
  std::vector<std::size_t> Ks;
  std::vector<std::size_t> RFs;
  std::size_t EF;
  double c;
  double delta;
  std::size_t L;
  std::size_t nblocks = 0; // Override the error limit
  std::size_t wanted_errs;
  std::size_t nblocks_min;
  std::size_t nblocks_max;
  double overhead;
  double chan_pGB;
  double chan_pBG;
  double iid_per;
  std::size_t nCycles;
  // Update also the serialization/deserialization in the SWIG file
};

struct simulation_results {
  std::vector<double> avg_pers;
  std::size_t actual_nblocks;
  std::vector<std::size_t> rec_counts;
  std::vector<std::size_t> err_counts;
  std::size_t dropped_count;
  double avg_enc_time;
  double avg_dec_time;
  // Update also the serialization/deserialization in the SWIG file
};

simulation_results run_uep(const simulation_params &params);

#endif
