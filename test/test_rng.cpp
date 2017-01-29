#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE rng_test
#include <boost/test/unit_test.hpp>

#include "rng.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

BOOST_AUTO_TEST_CASE(soliton_pmd) {
  BOOST_CHECK_EQUAL(soliton_distribution::soliton_pmd(10000, 1), 1/10000.0);
  BOOST_CHECK_EQUAL(soliton_distribution::soliton_pmd(10000, 2), 0.5);

  BOOST_CHECK_EQUAL(soliton_distribution::soliton_pmd(10000, 0), 0);
  BOOST_CHECK_EQUAL(soliton_distribution::soliton_pmd(10000, 10001), 0);
}

BOOST_AUTO_TEST_CASE(soliton_histogram) {
  using namespace std;
  int num = 100000;
  int K = 10000;
  soliton_distribution s(K);
  mt19937 gen;
  vector<uint_fast32_t> samples;
  samples.reserve(num);
  for (int i = 0; i < num; ++i)
    samples.push_back(s(gen));
  //sort(samples.begin(), samples.end());
  int rho_0 = count(samples.cbegin(), samples.cend(), 0);
  int rho_1 = count(samples.cbegin(), samples.cend(), 1);
  int rho_2 = count(samples.cbegin(), samples.cend(), 2);
  int rho_3 = count(samples.cbegin(), samples.cend(), 3);
  BOOST_CHECK_EQUAL(rho_0, 0);
  BOOST_CHECK_CLOSE(rho_1, soliton_distribution::soliton_pmd(K, 1) * num, 40);
  BOOST_CHECK_CLOSE(rho_2, soliton_distribution::soliton_pmd(K, 2) * num, 5);
  BOOST_CHECK_CLOSE(rho_3, soliton_distribution::soliton_pmd(K, 3) * num, 5);

  double sum = accumulate(samples.cbegin(), samples.cend(), 0.0);
  BOOST_CHECK_CLOSE(sum / num, log(K), 10);
}

