#define BOOST_TEST_MODULE test_block_decoder

#include <boost/test/unit_test.hpp>

#include "block_decoder.hpp"

using namespace std;
using namespace uep;

// Set globally the log severity level
struct global_fixture {
  global_fixture() {
    uep::log::init();
    auto warn_filter = boost::log::expressions::attr<
      uep::log::severity_level>("Severity") >= uep::log::warning;
    boost::log::core::get()->set_filter(warn_filter);
  }

  ~global_fixture() {
  }
};
BOOST_GLOBAL_FIXTURE(global_fixture);

BOOST_AUTO_TEST_CASE(check_rows) {
  const int seed = 0x42424242;
  lt_row_generator rowgen(robust_soliton_distribution(3, 0.1, 0.5));
  rowgen.reset(seed);

  vector<lt_row_generator::row_type> expected;
  expected.push_back({1, 2});
  expected.push_back({2, 0, 1});
  expected.push_back({0, 1, 2});
  expected.push_back({2});

  for (int i = 0; i < 4; ++i) {
    auto r = rowgen.next_row();
    BOOST_CHECK(equal(r.cbegin(), r.cend(), expected[i].cbegin()));
  }
}

struct setup_packets {
  const size_t L = 1024;
  const int seed = 0x42424242;

  vector<fountain_packet> received;
  vector<packet> expected;

  setup_packets() {
    received.push_back(fountain_packet(L, 0x11));
    received.back().block_seed(seed);
    received.back().block_number(42);
    received.back().sequence_number(0);
    received.push_back(fountain_packet(L, 0x33));
    received.back().block_seed(seed);
    received.back().block_number(42);
    received.back().sequence_number(1);
    received.push_back(fountain_packet(L, 0x33));
    received.back().block_seed(seed);
    received.back().block_number(42);
    received.back().sequence_number(2);
    received.push_back(fountain_packet(L, 0x44));
    received.back().block_seed(seed);
    received.back().block_number(42);
    received.back().sequence_number(3);

    expected.push_back(packet(L, 0x22));
    expected.push_back(packet(L, 0x55));
    expected.push_back(packet(L, 0x44));
  }
};

BOOST_FIXTURE_TEST_CASE(correct_decoding, setup_packets) {
  block_decoder dec(lt_row_generator(robust_soliton_distribution(3,0.1,0.5)));

  BOOST_CHECK(!dec.has_decoded());
  dec.push(received[0]);
  BOOST_CHECK_EQUAL(dec.seed(), seed);
  BOOST_CHECK_EQUAL(dec.block_number(), 42);
  BOOST_CHECK(!dec.has_decoded());
  dec.push(received[1]);
  BOOST_CHECK(!dec.has_decoded());
  dec.push(received[2]);
  BOOST_CHECK(!dec.has_decoded());
  dec.push(received[3]);
  BOOST_CHECK(dec.has_decoded());
  BOOST_CHECK_EQUAL(dec.decoded_count(), 3);

  BOOST_CHECK_EQUAL(dec.seed(), seed);
  BOOST_CHECK_EQUAL(dec.block_number(), 42);

  auto i = dec.block_begin();
  auto j = expected.cbegin();
  while (i != dec.block_end()) {
    BOOST_CHECK(*i == *j);
    ++i; ++j;
  }
}

BOOST_FIXTURE_TEST_CASE(reject_wrong_seed, setup_packets) {
  block_decoder dec(lt_row_generator(robust_soliton_distribution(3,0.1,0.5)));
  dec.push(received[0]);
  BOOST_CHECK_EQUAL(dec.seed(), seed);
  BOOST_CHECK_EQUAL(dec.block_number(), 42);
  BOOST_CHECK_THROW(dec.push(fountain_packet(42, 11, 1234)), runtime_error);
}

BOOST_FIXTURE_TEST_CASE(reject_wrong_blockno, setup_packets) {
  block_decoder dec(lt_row_generator(robust_soliton_distribution(3,0.1,0.5)));
  dec.push(received[0]);
  BOOST_CHECK_EQUAL(dec.seed(), seed);
  BOOST_CHECK_EQUAL(dec.block_number(), 42);
  BOOST_CHECK_THROW(dec.push(fountain_packet(44, 11, seed)), runtime_error);
}

BOOST_FIXTURE_TEST_CASE(reject_wrong_size, setup_packets) {
  block_decoder dec(lt_row_generator(robust_soliton_distribution(3,0.1,0.5)));
  dec.push(received[0]);
  fountain_packet wrong_size(42, 3, seed, 1500, 0x44);
  BOOST_CHECK_THROW(dec.push(wrong_size), runtime_error);
}

BOOST_FIXTURE_TEST_CASE(duplicate_packets, setup_packets) {
  block_decoder dec(lt_row_generator(robust_soliton_distribution(3,0.1,0.5)));
  for (int i = 0; i < 4; ++i) {
    const fountain_packet &p = received[i];
    BOOST_CHECK(dec.push(p));
    for (int j = 1; j < 100; ++j) {
      BOOST_CHECK(!dec.push(p));
    }
  }
  BOOST_CHECK(dec.has_decoded());
  auto i = dec.block_begin();
  auto j = expected.cbegin();
  while (i != dec.block_end()) {
    BOOST_CHECK(*i == *j);
    ++i; ++j;
  }
}

BOOST_FIXTURE_TEST_CASE(out_of_order, setup_packets) {
  block_decoder dec(lt_row_generator(robust_soliton_distribution(3,0.1,0.5)));
  vector<fountain_packet> recv;
  std::mt19937 g;
  for (int i = 0; i < 10; ++i)
    recv.insert(recv.cend(), received.cbegin(), received.cend());
  std::shuffle(recv.begin(), recv.end(), g);

  for (auto i = recv.cbegin(); i != recv.cend(); ++i)
    dec.push(*i);
  recv.clear();

  BOOST_CHECK(dec.has_decoded());
  auto i = dec.block_begin();
  auto j = expected.cbegin();
  while (i != dec.block_end()) {
    BOOST_CHECK(*i == *j);
    ++i; ++j;
  }
}
