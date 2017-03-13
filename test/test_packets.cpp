#define BOOST_TEST_MODULE packets_test
#include <boost/test/unit_test.hpp>

#include "packets.hpp"

using namespace std;

BOOST_AUTO_TEST_CASE(packet_shallow_copy) {
  packet p(5, 0x11);
  packet q(p);
  q[0] = 0x22;
  BOOST_CHECK(q != p);
  packet r = p;
  r[1] = 0x33;
  BOOST_CHECK(r != p);

  BOOST_CHECK_EQUAL(p.shared_count(), 1);
  packet pp = p.shallow_copy();
  BOOST_CHECK_EQUAL(p.shared_count(), 2);
  pp[2] = 0x44;
  BOOST_CHECK(pp == p);
}

BOOST_AUTO_TEST_CASE(fountain_packet_shallow_copy) {
  fountain_packet p(1,2,3, 5, 0x11);
  fountain_packet q(p);
  q[0] = 0x22;
  BOOST_CHECK(q != p);
  fountain_packet r = p;
  r[1] = 0x33;
  BOOST_CHECK(r != p);

  BOOST_CHECK_EQUAL(p.shared_count(), 1);
  fountain_packet pp = p.shallow_copy();
  BOOST_CHECK_EQUAL(p.shared_count(), 2);

  BOOST_CHECK_EQUAL(pp.block_number(), 1);
  pp[2] = 0x44;
  BOOST_CHECK(pp == p);

  q = p;
  q.block_seed(42);
  BOOST_CHECK(q != p);
}

BOOST_AUTO_TEST_CASE(packet_xor) {
  packet p(10, 0x11);
  packet q(10, 0x22);
  packet expected(10, 0x33);

  BOOST_CHECK((p ^ q) == expected);
}

BOOST_AUTO_TEST_CASE(packet_wrong_xor) {
  packet p(10, 0x11);
  packet q(9, 0x22);
  fountain_packet r(11, 0x33);
  BOOST_CHECK_THROW(p ^ q, runtime_error);
  BOOST_CHECK_THROW(q ^ p, runtime_error);
  BOOST_CHECK_THROW(r ^ p, runtime_error);
}
