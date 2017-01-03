#include "rng.hpp"

#include <algorithm>

using namespace std;

soliton_distribution::soliton_distribution(uint_fast32_t input_pkt_count) :
  K_(input_pkt_count) {
  vector<double> weights;
  for (uint_fast32_t d = 1; d <= K_; d++) {
    weights.push_back(soliton_pmd(K_, d));
  }

  typedef typename decltype(distrib)::param_type p_type;
  p_type p(weights.begin(), weights.end());
  distrib.param(p);
}

std::uint_fast32_t soliton_distribution::K() const {
  return K_;
}

double soliton_distribution::soliton_pmd(uint_fast32_t K, uint_fast32_t d) {
  if (d == 1) return 1/(double)K;
  else if (d >= 2 && d <= K) return 1/((double)d*(d-1));
  else return 0;
}

fountain::fountain(uint_fast32_t input_pkt_count) :
  K_(input_pkt_count),
  degree_distr(K_),
  packet_distr(0, K_-1),
  sel_count(0) {
}

fountain::fountain(uint_fast32_t input_pkt_count,
		   generator_type::result_type seed) :
  fountain(input_pkt_count) {
  generator.seed(seed);
}

typename fountain::row_type fountain::next_row() {
  uint_fast32_t degree = degree_distr(generator);
  row_type s;
  s.reserve(degree);
  for (uint_fast32_t i = 0; i < degree; i++) {
    uint_fast32_t si;
    do {
      si = packet_distr(generator);
    }	while (find(s.begin(), s.end(), si) != s.end());
    s.push_back(si);
  }
  sel_count++;
  return s;
}

typename fountain::row_type::size_type fountain::generated_rows() const {
  return sel_count;
}

uint_fast32_t fountain::K() const {
  return K_;
}

void fountain::reset() {
  generator.seed();
  sel_count = 0;
}

void fountain::reset(generator_type::result_type seed) {
  generator.seed(seed);
  sel_count = 0;
}
