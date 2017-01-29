#include "rng.hpp"

#include <cmath>

using namespace std;
using namespace std::placeholders;

degree_distribution::degree_distribution(std::uint_fast32_t K, const pmd_t &pmd) :
  K_(K), pmd_(pmd) {
  vector<double> weights;
  for (uint_fast32_t d = 1; d <= K; ++d) {
    weights.push_back(pmd_(d));
  }

  typedef decltype(distrib)::param_type p_type;
  p_type p(weights.begin(), weights.end());
  distrib.param(p);
}

std::uint_fast32_t degree_distribution::K() const {
  return K_;
}

degree_distribution::pmd_t degree_distribution::pmd() const {
  return pmd_;
}

soliton_distribution::soliton_distribution(uint_fast32_t input_pkt_count) :
  degree_distribution(input_pkt_count, bind(soliton_pmd, input_pkt_count, _1)) {
}

double soliton_distribution::soliton_pmd(uint_fast32_t K, uint_fast32_t d) {
  if (d == 1) return 1/(double)K;
  else if (d >= 2 && d <= K) return 1/((double)d*(d-1));
  else return 0;
}

double robust_soliton_distribution::S(std::uint_fast32_t K,
				      double c,
				      double delta) {
  return c * log(K/delta) * sqrt(K);
}

double robust_soliton_distribution::tau(std::uint_fast32_t K_S,
					double S_delta,
					std::uint_fast32_t i) {
  if (i >= 1 && i <= K_S - 1)
    return 1/((double)K_S * i);
  else if (i == K_S)
    return log(S_delta) / (double) K_S;
  else
    return 0;
}

double robust_soliton_distribution::beta(std::uint_fast32_t K,
					 std::uint_fast32_t K_S,
					 double S_delta) {
  double sum = 0;
  for (std::uint_fast32_t i = 1; i <= K; ++i) {
    sum += soliton_distribution::soliton_pmd(K,i) + tau(K_S, S_delta, i);
  }
  return sum;
}

double robust_soliton_distribution::robust_pmd(std::uint_fast32_t K,
					       double c,
					       double delta,
					       std::uint_fast32_t d) {
  if (d >= 1 && d <= K) {
    double S_ = S(K, c, delta);
    uint_fast32_t K_S = lround(K/S_);
    double S_delta = S_/delta;
    double beta_ = beta(K, K_S, S_delta);
    return (soliton_distribution::soliton_pmd(K,d) +
	    tau(K_S,S_delta,d)) / beta_;
  }
  else return 0;
}

robust_soliton_distribution::robust_soliton_distribution(uint_fast32_t input_pkt_count,
							 double c,
							 double delta) :
  degree_distribution(input_pkt_count,
		      bind(robust_pmd, input_pkt_count, c, delta, _1)),
  c_(c), delta_(delta) {
}

double robust_soliton_distribution::c() const {
  return c_;
}

double robust_soliton_distribution::delta() const {
  return delta_;
}

fountain::fountain(const degree_distribution &deg) :
  degree_distr(deg),
  packet_distr(0, deg.K()-1),
  sel_count(0) {
}

fountain::fountain(const degree_distribution &deg,
		   generator_type::result_type seed) :
  fountain(deg) {
  generator.seed(seed);
}

fountain::row_type fountain::next_row() {
  uint_fast32_t degree = degree_distr(generator);
  row_type s;
  s.reserve(degree);
  for (uint_fast32_t i = 0; i < degree; ++i) {
    uint_fast32_t si;
    do {
      si = packet_distr(generator);
    }	while (find(s.begin(), s.end(), si) != s.end());
    s.push_back(si);
  }
  ++sel_count;
  return s;
}

typename fountain::row_type::size_type fountain::generated_rows() const {
  return sel_count;
}

std::uint_fast32_t fountain::K() const {
  return degree_distr.K();
}

void fountain::reset() {
  generator.seed();
  sel_count = 0;
}

void fountain::reset(generator_type::result_type seed) {
  generator.seed(seed);
  sel_count = 0;
}
