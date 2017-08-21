#define BOOST_TEST_MODULE packet_rw_test
#include <boost/test/unit_test.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include "packets_rw.hpp"

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
  BOOST_CHECK_NO_THROW(parse_raw_data_packet(u));

  u = v;
  u[10] = 4;
  BOOST_CHECK_THROW(parse_raw_data_packet(u), exception);

  u = v;
  u.resize(data_header_size + 2);
  BOOST_CHECK_THROW(parse_raw_data_packet(u), exception);

  u = v;
  u.resize(data_header_size + 4);
  BOOST_CHECK_NO_THROW(parse_raw_data_packet(u));

  u = v;
  u.resize(data_header_size + 2);
  u[10] = 2;
  BOOST_CHECK_NO_THROW(parse_raw_data_packet(u));

  u = v;
  u.resize(1);
  BOOST_CHECK_THROW(parse_raw_data_packet(u), exception);
}

BOOST_AUTO_TEST_CASE(build_ack) {
  const char *expected1 = "\x01\x00\xff";
  const char *expected2 = "\x01\xff\x00";

  std::size_t blockno1 = 0xff, blockno2 = 0xff00;
  std::vector<char> ack1 = build_raw_ack(blockno1);
  std::vector<char> ack2 = build_raw_ack(blockno2);

  BOOST_CHECK(equal(ack1.cbegin(), ack1.cend(), expected1));
  BOOST_CHECK(equal(ack2.cbegin(), ack2.cend(), expected2));
}

BOOST_AUTO_TEST_CASE(parse_ack) {
  const char *raw_str1 = "\x01\x00\xff";
  const char *raw_str2 = "\x01\xff\x00";
  const std::vector<char> raw1(raw_str1, raw_str1+ack_header_size);
  const std::vector<char> raw2(raw_str2, raw_str2+ack_header_size);

  const std::size_t blockno1 = 0xff, blockno2 = 0xff00;

  BOOST_CHECK_EQUAL(blockno1, parse_raw_ack_packet(raw1));
  BOOST_CHECK_EQUAL(blockno2, parse_raw_ack_packet(raw2));
}

BOOST_AUTO_TEST_CASE(ack_build_parse) {
  const std::size_t bns[] = {0, 0xffff, 0x1234, 1, 0x1000};

  for (int i = 0; i < 5; ++i) {
    std::vector<char> raw = build_raw_ack(bns[i]);
    size_t parsed = parse_raw_ack_packet(raw);
    BOOST_CHECK_EQUAL(parsed, bns[i]);
  }
}

BOOST_AUTO_TEST_CASE(ack_fail) {
  std::vector<char> shr(2, '\x01');
  std::vector<char> lng(4, '\x01');
  std::vector<char> empty;

  BOOST_CHECK_THROW(parse_raw_ack_packet(empty), runtime_error);
  BOOST_CHECK_THROW(parse_raw_ack_packet(shr), runtime_error);
  BOOST_CHECK_NO_THROW(parse_raw_ack_packet(lng));

  std::size_t overflow_bn = 0x10000;
  BOOST_CHECK_THROW(build_raw_ack(overflow_bn), boost::numeric::positive_overflow);
}

BOOST_AUTO_TEST_CASE(wrong_type) {
  std::vector<char> raw_ack(ack_header_size);
  raw_ack[0] = raw_packet_type::block_ack;
  std::vector<char> raw_data(data_header_size);
  raw_data[0] = raw_packet_type::data;

  BOOST_CHECK_THROW(parse_raw_data_packet(raw_ack), runtime_error);
  BOOST_CHECK_THROW(parse_raw_ack_packet(raw_data), runtime_error);

  raw_data[0] = 0xff;
  BOOST_CHECK_THROW(parse_raw_data_packet(raw_data), runtime_error);
  BOOST_CHECK_THROW(parse_raw_ack_packet(raw_data), runtime_error);
}
