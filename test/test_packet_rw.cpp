#define BOOST_TEST_MODULE packet_rw_test
#include <boost/test/unit_test.hpp>

#include "packets_rw.hpp"

#include <sstream>

using namespace std;

BOOST_AUTO_TEST_CASE(read_write_test) {
  fountain_packet test_fp;
  test_fp.block_number(0x4);
  test_fp.sequence_number(0xedde);
  test_fp.block_seed(0xffee00bb);
  test_fp.resize(3);
  test_fp[0] = 0x11;
  test_fp[1] = 0x22;
  test_fp[2] = 0x33;

  vector<char> raw = build_raw_packet(test_fp);
  const char *expected_raw = "\x00\x00\x04\xed\xde\xff\xee\x00\xbb\x00\x03\x11\x22\x33";

  for (size_t i = 0; i != raw.size(); ++i) {
    BOOST_CHECK_EQUAL(raw[i], expected_raw[i]);
  }

  fountain_packet decoded_fp = parse_raw_data_packet(raw);
  BOOST_CHECK_EQUAL(test_fp, decoded_fp);

  decoded_fp.block_seed(0);
  BOOST_CHECK_NE(test_fp, decoded_fp);
  decoded_fp.block_seed(test_fp.block_seed());
  decoded_fp[0] = 0x44;
  BOOST_CHECK_NE(test_fp, decoded_fp);
}

BOOST_AUTO_TEST_CASE(limit_test) {
  fountain_packet test_fp;
  test_fp.block_number(0xffff);
  test_fp.sequence_number(0xffff);
  test_fp.block_seed(0xffffffff);
  test_fp.resize(0xffff);
  BOOST_CHECK_NO_THROW(build_raw_packet(test_fp));
  test_fp.resize(0);
  BOOST_CHECK_NO_THROW(build_raw_packet(test_fp));

  test_fp = fountain_packet();
  test_fp.block_number(0xffff + 1);
  BOOST_CHECK_THROW(build_raw_packet(test_fp), exception);

  test_fp = fountain_packet();
  test_fp.sequence_number(0xffff + 1);
  BOOST_CHECK_THROW(build_raw_packet(test_fp), exception);
}

BOOST_AUTO_TEST_CASE(wrong_raw) {
  const char *raw = "\x00\x00\x04\xed\xde\xff\xee\x00\xbb\x00\x03\x11\x22\x33";
  const vector<char> v(raw, raw + data_header_size+3);
  vector<char> u;

  u = v;
  u[0] = 4;
  BOOST_CHECK_THROW(parse_raw_data_packet(u), exception);

  u = v;
  u[10] = 2;
  BOOST_CHECK_THROW(parse_raw_data_packet(u), exception);

  u = v;
  u[10] = 4;
  BOOST_CHECK_THROW(parse_raw_data_packet(u), exception);

  u = v;
  u.resize(data_header_size + 2);
  BOOST_CHECK_THROW(parse_raw_data_packet(u), exception);

  u = v;
  u.resize(data_header_size + 4);
  BOOST_CHECK_THROW(parse_raw_data_packet(u), exception);

  u = v;
  u.resize(data_header_size + 2);
  u[10] = 2;
  BOOST_CHECK_NO_THROW(parse_raw_data_packet(u));
}
