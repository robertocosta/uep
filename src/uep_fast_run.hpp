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
  std::size_t nblocks;
  double overhead;
  double chan_pGB;
  double chan_pBG;
  // Update also the serialization/deserialization in the SWIG file
};

struct simulation_results {
  std::vector<double> avg_pers;
  std::vector<std::size_t> rec_counts;
  std::size_t dropped_count;
  double avg_enc_time;
  // Update also the serialization/deserialization in the SWIG file
};

void run_uep(const simulation_params &params, simulation_results &results);

#endif
