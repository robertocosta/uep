#define BOOST_TEST_MODULE test_block_encoder

#include <boost/test/unit_test.hpp>

#include "block_encoder.hpp"

using namespace std;
using namespace uep;

struct setup_packets {
  vector<packet> input;
  const int seed = 0x42424242;
  const size_t L = 1500;
  vector<packet> expected;

  lt_row_generator rowgen;
  block_encoder enc;

  setup_packets() :
    rowgen(robust_soliton_distribution(3, 0.1, 0.5)),
    enc(rowgen) {
    input.push_back(packet(L, 0x22));
    input.push_back(packet(L, 0x55));
    input.push_back(packet(L, 0x44));

    expected.push_back(packet(L, 0x11));
    expected.push_back(packet(L, 0x33));
    expected.push_back(packet(L, 0x33));
    expected.push_back(packet(L, 0x44));
  }
};

BOOST_FIXTURE_TEST_CASE(check_rows, setup_packets) {
  rowgen.reset(seed);
  
  vector<lt_row_generator::row_type> exp_rows;
  exp_rows.push_back({1, 2});
  exp_rows.push_back({2, 0, 1});
  exp_rows.push_back({0, 1, 2});
  exp_rows.push_back({2});

  for (int i = 0; i < 4; ++i) {
    auto r = rowgen.next_row();
    BOOST_CHECK(equal(r.cbegin(), r.cend(), exp_rows[i].cbegin()));
  }
}

BOOST_FIXTURE_TEST_CASE(correct_encoding, setup_packets) {
  BOOST_CHECK(!enc.can_encode());
  BOOST_CHECK_EQUAL(enc.output_count(), 0);
  enc.set_seed(seed);
  BOOST_CHECK_EQUAL(enc.seed(), seed);
  BOOST_CHECK(enc.block_begin() == enc.block_end());
  enc.set_block(input.cbegin(), input.cend());
  BOOST_CHECK(enc.can_encode());

  vector<packet> out;
  for (int i = 0; i < 4; ++i)
    out.push_back(enc.next_coded());
  BOOST_CHECK_EQUAL(enc.output_count(), 4);
  BOOST_CHECK(equal(out.cbegin(), out.cend(), expected.cbegin()));
}
