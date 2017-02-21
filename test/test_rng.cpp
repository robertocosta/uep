#define BOOST_TEST_MODULE rng_test
#include <boost/test/unit_test.hpp>

#include "rng.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <vector>

using namespace std;

BOOST_AUTO_TEST_CASE(soliton_pmd) {
  BOOST_CHECK_EQUAL(soliton_distribution::soliton_pmd(10000, 1), 1/10000.0);
  BOOST_CHECK_EQUAL(soliton_distribution::soliton_pmd(10000, 2), 0.5);

  BOOST_CHECK_EQUAL(soliton_distribution::soliton_pmd(10000, 0), 0);
  BOOST_CHECK_EQUAL(soliton_distribution::soliton_pmd(10000, 10001), 0);
}

BOOST_AUTO_TEST_CASE(soliton_histogram) {
  size_t num = 100000;
  size_t K = 10000;
  soliton_distribution s(K);
  mt19937 gen;
  vector<size_t> samples;
  samples.reserve(num);
  for (size_t i = 0; i < num; ++i)
    samples.push_back(s(gen));
  //sort(samples.begin(), samples.end());
  size_t rho_0 = count(samples.cbegin(), samples.cend(), 0);
  size_t rho_1 = count(samples.cbegin(), samples.cend(), 1);
  size_t rho_2 = count(samples.cbegin(), samples.cend(), 2);
  size_t rho_3 = count(samples.cbegin(), samples.cend(), 3);
  BOOST_CHECK_EQUAL(rho_0, 0);
  BOOST_CHECK_CLOSE(rho_1, soliton_distribution::soliton_pmd(K, 1) * num, 40);
  BOOST_CHECK_CLOSE(rho_2, soliton_distribution::soliton_pmd(K, 2) * num, 5);
  BOOST_CHECK_CLOSE(rho_3, soliton_distribution::soliton_pmd(K, 3) * num, 5);

  double sum = accumulate(samples.cbegin(), samples.cend(), 0.0);
  BOOST_CHECK_CLOSE(sum / num, log(K), 10);
}

BOOST_AUTO_TEST_CASE(robust_pmd) {
  size_t K = 10000;
  double delta = 0.05;
  double c = 0.2;
  double S = robust_soliton_distribution::S(K,c,delta);
  BOOST_CHECK_CLOSE(S, 244, 1);
  size_t K_S = lround(K / S);
  BOOST_CHECK_EQUAL(K_S, 41);
  double beta = robust_soliton_distribution::beta(K, K_S, S/delta);
  BOOST_CHECK_CLOSE(beta, 1.3, 1);

  vector<double> pmd;
  for (size_t d = 1; d <= K; ++d)
    pmd.push_back(robust_soliton_distribution::robust_pmd(K,c,delta,d));
  BOOST_CHECK_CLOSE(accumulate(pmd.cbegin(), pmd.cend(), 0.0), 1.0, 1e-1);

  BOOST_CHECK_CLOSE(robust_soliton_distribution::tau(K_S, S/delta, 41), 0.207, 1);
  BOOST_CHECK_CLOSE(robust_soliton_distribution::tau(K_S, S/delta, 1), 1/41.0, 1);

  BOOST_CHECK_CLOSE(robust_soliton_distribution::robust_pmd(K,c,delta,2), 0.394, 1);
}

BOOST_AUTO_TEST_CASE(robust_histogram) {
  size_t num = 100000;
  size_t K = 10000;
  double delta = 0.05;
  double c = 0.2;
  robust_soliton_distribution s(K,c,delta);
  mt19937 gen;
  vector<size_t> samples;
  samples.reserve(num);
  for (size_t i = 0; i < num; ++i)
    samples.push_back(s(gen));
  //sort(samples.begin(), samples.end());
  size_t rho_0 = count(samples.cbegin(), samples.cend(), 0);
  size_t rho_1 = count(samples.cbegin(), samples.cend(), 1);
  size_t rho_2 = count(samples.cbegin(), samples.cend(), 2);
  size_t rho_3 = count(samples.cbegin(), samples.cend(), 3);
  size_t rho_41 = count(samples.cbegin(), samples.cend(), 41);
  BOOST_CHECK_EQUAL(rho_0, 0);
  BOOST_CHECK_CLOSE(rho_1, robust_soliton_distribution::robust_pmd(K,c,delta,1) * num, 5);
  BOOST_CHECK_CLOSE(rho_2, robust_soliton_distribution::robust_pmd(K,c,delta,2) * num, 5);
  BOOST_CHECK_CLOSE(rho_3, robust_soliton_distribution::robust_pmd(K,c,delta,3) * num, 5);
  BOOST_CHECK_CLOSE(rho_41, robust_soliton_distribution::robust_pmd(K,c,delta,41) * num, 5);

  // double sum = accumulate(samples.cbegin(), samples.cend(), 0.0);
  // BOOST_CHECK_CLOSE(sum / num, log(K), 10);
}

BOOST_AUTO_TEST_CASE(row_generation) {
  const size_t K = 10000;
  double c = 0.2;
  double delta = 0.05;
  robust_soliton_distribution rs(K,c,delta);
  lt_row_generator f(rs);
  size_t nrows = 100000;
  size_t outdeg_one = 0;
  for (size_t i = 0; i < nrows; ++i) {
    lt_row_generator::row_type r = f.next_row();
    BOOST_CHECK(r.size() > 0);
    BOOST_CHECK(r.size() <= K);
    BOOST_CHECK(all_of(r.cbegin(), r.cend(),
		       [K](size_t i) -> bool {return i >= 0 && i < K;}));
    if (r.size() == 1)
      ++outdeg_one;
  }
  BOOST_CHECK_EQUAL(f.generated_rows(), nrows);
  BOOST_CHECK_CLOSE(outdeg_one,
		    robust_soliton_distribution::robust_pmd(K,c,delta,1) * nrows,
		    1);
}
