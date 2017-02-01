#include "rng.hpp"

#include <iostream>
#include <fstream>

using namespace std;

void print_ideal(int N, int K) {
  // Ideal soliton samples
  ofstream ideal("ideal_soliton_samples.csv");
  ideal.exceptions(ios_base::badbit | ios_base::failbit);
  std::mt19937 g;
  soliton_distribution s_dist(K);
  ideal << "sample" << endl;
  for (int i = 0; i < N; i++) {
    uint_fast32_t sample = s_dist(g);
    ideal << sample << endl;
  }
  ideal.close();
}

void print_robust(int N, int K, double c, double delta) {
  // Robust soliton samples
  ofstream robust("robust_soliton_samples.csv");
  robust.exceptions(ios_base::badbit | ios_base::failbit);
  std::mt19937 g;
  robust_soliton_distribution r_dist(K, c, delta);
  robust << "sample" << endl;
  for (int i = 0; i < N; i++) {
    uint_fast32_t sample = r_dist(g);
    robust << sample << endl;
  }
  robust.close();
}

void print_ideal_pmd(int K) {
  ofstream out("ideal_pmd.csv");
  out.exceptions(ios_base::badbit | ios_base::failbit);
  out << "degree, pmd" << endl;
  for (int i = 1; i <= K; ++i) {
    out << i << ", " << soliton_distribution::soliton_pmd(K,i) << endl;
  }
  out.close();
}

void print_robust_pmd(int K, double c, double delta) {
  ofstream out("robust_pmd.csv");
  out.exceptions(ios_base::badbit | ios_base::failbit);
  out << "degree, pmd" << endl;
  for (int i = 1; i <= K; ++i) {
    out << i << ", " << robust_soliton_distribution::robust_pmd(K,c,delta,i) << endl;
  }
  out.close();
}

int main(int, char**) {
  const int N = 1e4;
  const int K = 1e4;
  print_ideal(N,K);
  const double c = 0.2;
  const double delta = 0.05;
  print_robust(N,K,c, delta);

  print_ideal_pmd(K);
  print_robust_pmd(K,c,delta);
  
  
  return 0;
}
