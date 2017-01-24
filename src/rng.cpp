#include "rng.hpp"

#include <cmath>
#include <functional>

using namespace std;
using namespace std::placeholders;

/** Use a PMD evaluated in [1,K] to initialize a discrete distribution */
void init_distrib_with_pmd(discrete_distribution<uint_fast32_t> &distrib,
			   const function<double(uint_fast32_t)> &pmd,
			   uint_fast32_t K) {
  vector<double> weights;
  for (uint_fast32_t d = 1; d <= K; d++) {
    weights.push_back(pmd(d));
  }
  
  typedef discrete_distribution<uint_fast32_t>::param_type p_type;
  p_type p(weights.begin(), weights.end());
  distrib.param(p);
}

soliton_distribution::soliton_distribution(uint_fast32_t input_pkt_count) :
  K_(input_pkt_count) {
  init_distrib_with_pmd(distrib, bind(soliton_pmd, K_, _1), K_);
}

std::uint_fast32_t soliton_distribution::K() const {
  return K_;
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

double robust_soliton_distribution::tau(std::uint_fast32_t K,
					 double c,
					 double delta,
					 std::uint_fast32_t i) {
  double S = robust_soliton_distribution::S(K, c, delta);
  
  if (i >= 1 && i <= K/S - 1)
    return S/(i*K);
  else if (i == K/S)
    return S/K * log(S/delta);
  else
    return 0;
}

double robust_soliton_distribution::beta(std::uint_fast32_t K,
					 double c,
					 double delta) {
  double sum = 0;
  for (std::uint_fast32_t i = 1; i <= K; ++i) {
    sum += tau(K,c,delta,i) + soliton_distribution::soliton_pmd(K,i);
  }
  return sum;
}

double robust_soliton_distribution::robust_pmd(std::uint_fast32_t K,
					       double c,
					       double delta,
					       std::uint_fast32_t d) {
  if (d >= 1 && d <= K)
    return (soliton_distribution::soliton_pmd(K,d) +
	    tau(K,c,delta,d)) / beta(K,c,delta);
  else
    return 0;
}

robust_soliton_distribution::robust_soliton_distribution(uint_fast32_t input_pkt_count,
							 double c,
							 double delta) :
  K_(input_pkt_count), c_(c), delta_(delta) {
  init_distrib_with_pmd(distrib, bind(robust_pmd, K_, c_, delta_, _1), K_);
}

std::uint_fast32_t robust_soliton_distribution::K() const {
  return K_;
}

double robust_soliton_distribution::c() const {
  return c_;
}

double robust_soliton_distribution::delta() const {
  return delta_;
}
