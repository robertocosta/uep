#include "rng.hpp"

#include <cmath>

using namespace std;
using namespace std::placeholders;

degree_distribution::degree_distribution(int K, const pmd_t &pmd) :
  K_(K), pmd_(pmd) {
  vector<double> weights;
  for (int d = 1; d <= K; ++d) {
    weights.push_back(pmd_(d));
  }

  typedef decltype(distrib)::param_type p_type;
  p_type p(weights.begin(), weights.end());
  distrib.param(p);
}

int degree_distribution::K() const {
  return K_;
}

degree_distribution::pmd_t degree_distribution::pmd() const {
  return pmd_;
}

soliton_distribution::soliton_distribution(int input_pkt_count) :
  degree_distribution(input_pkt_count, bind(soliton_pmd, input_pkt_count, _1)) {
}

double soliton_distribution::soliton_pmd(int K, int d) {
  if (d == 1) return 1/(double)K;
  else if (d >= 2 && d <= K) return 1/((double)d*(d-1));
  else return 0;
}

double robust_soliton_distribution::S(int K, double c, double delta) {
  return c * log(K/delta) * sqrt(K);
}

double robust_soliton_distribution::tau(int K_S, double S_delta, int i) {
  if (i >= 1 && i <= K_S - 1)
    return 1/((double)K_S * i);
  else if (i == K_S)
    return log(S_delta) / (double) K_S;
  else
    return 0;
}

double robust_soliton_distribution::beta(int K, int K_S, double S_delta) {
  double sum = 0;
  for (int i = 1; i <= K; ++i) {
    sum += soliton_distribution::soliton_pmd(K,i) + tau(K_S, S_delta, i);
  }
  return sum;
}

double robust_soliton_distribution::robust_pmd(int K, double c, double delta,
					       int d) {
  if (d >= 1 && d <= K) {
    double S_ = S(K, c, delta);
    int K_S = lround(K/S_);
    double S_delta = S_/delta;
    double beta_ = beta(K, K_S, S_delta);
    return (soliton_distribution::soliton_pmd(K,d) +
	    tau(K_S,S_delta,d)) / beta_;
  }
  else return 0;
}

robust_soliton_distribution::robust_soliton_distribution(int input_pkt_count,
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

double robust_soliton_distribution::beta() const {
  double S_ = S(K(), c_, delta_);
  int K_S = lround(K()/S_);
  double S_delta = S_/delta_;
  return beta(K(), K_S, S_delta);
}

fountain::fountain(const degree_distribution &deg) :
  degree_distr(deg),
  packet_distr(0, deg.K()-1),
  sel_count(0) {
}

fountain::fountain(const degree_distribution &deg, int seed) :
  fountain(deg) {
  generator.seed(seed);
}

fountain::row_type fountain::next_row() {
  int degree = degree_distr(generator);
  row_type s;
  s.reserve(degree);
  for (int i = 0; i < degree; ++i) {
    int si;
    do {
      si = packet_distr(generator);
    }	while (find(s.begin(), s.end(), si) != s.end());
    s.push_back(si);
  }
  ++sel_count;
  return s;
}

int fountain::generated_rows() const {
  return sel_count;
}

int fountain::K() const {
  return degree_distr.K();
}

void fountain::reset() {
  generator.seed();
  sel_count = 0;
}

void fountain::reset(int seed) {
  generator.seed(seed);
  sel_count = 0;
}
