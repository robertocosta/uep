#define BOOST_TEST_MODULE test_message_passing

#include <boost/test/unit_test.hpp>

#include "message_passing.hpp"

using namespace std;
using namespace uep;

struct mp_setup {
  typedef message_passing_context<unsigned int> mp_t;
  mp_t *mp;
  mp_t *fail;
  mp_t *partial;
  const size_t K = 3;
  const vector<unsigned int> expected{0x00, 0x11, 0x22};
  const vector<unsigned int> out{0x11,0x22,0x33,0x44};
  vector<pair<size_t,size_t>> edges;

  mp_setup() {
    edges.push_back(make_pair(1,0));
    edges.push_back(make_pair(1,2));
    edges.push_back(make_pair(1,3));
    edges.push_back(make_pair(2,2));
    edges.push_back(make_pair(2,3));
    edges.push_back(make_pair(0,1));
    edges.push_back(make_pair(2,1));

    mp = new mp_t(K, out.cbegin(), out.cend(), edges.cbegin(), edges.cend());
    edges.push_back(make_pair(0,2));
    edges.push_back(make_pair(0,3));
    partial = new mp_t(K, out.cbegin(), out.cend(), edges.cbegin(), edges.cend());
    edges.push_back(make_pair(0,0));
    fail = new mp_t(K, out.cbegin(), out.cend(), edges.cbegin(), edges.cend());
  }

  ~mp_setup() {
    delete mp;
    delete fail;
    delete partial;
  }
};

BOOST_FIXTURE_TEST_CASE(mp_build, mp_setup) {
  BOOST_CHECK_EQUAL(mp->input_size(), K);
  BOOST_CHECK_EQUAL(mp->output_size(), out.size());
  BOOST_CHECK(!mp->has_decoded());
  BOOST_CHECK_EQUAL(mp->decoded_count(), 0);
}

BOOST_FIXTURE_TEST_CASE(mp_decoding, mp_setup) {
  mp->run();
  BOOST_CHECK(mp->has_decoded());
  BOOST_CHECK(equal(mp->decoded_begin(), mp->decoded_end(), expected.cbegin()));
}

BOOST_FIXTURE_TEST_CASE(mp_multiple, mp_setup) {
  mp->run();
  mp->run();
  mp->run();
  BOOST_CHECK(mp->has_decoded());
  BOOST_CHECK(equal(mp->decoded_begin(), mp->decoded_end(), expected.cbegin()));
}

BOOST_FIXTURE_TEST_CASE(mp_fail, mp_setup) {
  BOOST_CHECK(!fail->has_decoded());
  fail->run();
  BOOST_CHECK(!fail->has_decoded());
  BOOST_CHECK_EQUAL(fail->decoded_count(), 0);
}

BOOST_FIXTURE_TEST_CASE(mp_partial, mp_setup) {
  BOOST_CHECK(!partial->has_decoded());
  partial->run();
  BOOST_CHECK(!partial->has_decoded());
  BOOST_CHECK_EQUAL(partial->decoded_count(), 1);
  BOOST_CHECK(*(partial->decoded_begin()) == expected[1]);
}

BOOST_FIXTURE_TEST_CASE(mp_retry_partial, mp_setup) {
  partial->run();
  partial->run();
  partial->run();
  partial->run();
  BOOST_CHECK(!partial->has_decoded());
  BOOST_CHECK_EQUAL(partial->decoded_count(), 1);
  BOOST_CHECK(*(partial->decoded_begin()) == expected[1]);
}
