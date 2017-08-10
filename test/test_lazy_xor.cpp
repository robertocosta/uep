#define BOOST_TEST_MODULE test_lazy_xor
#include <boost/test/unit_test.hpp>

#include <boost/mpl/vector.hpp>

#include "lazy_xor.hpp"
#include "packets.hpp"

using namespace uep;
using namespace std;

/** Symbol types used to test the lazy_xor. */
typedef boost::mpl::vector<char,
			   unsigned long,
			   packet
			   > symbol_types;

template <class Sym>
struct symbol_traits {};

template <>
struct symbol_traits<char> {
  typedef char sym_t;
  static sym_t build(char base) {
    return base;
  };
};

template <>
struct symbol_traits<unsigned long> {
  typedef unsigned long sym_t;
  static sym_t build(char base) {
    return static_cast<sym_t>(base);
  };
};

template <>
struct symbol_traits<packet> {
  typedef packet sym_t;
  static sym_t build(char base) {
    return packet(1024, base);
  };
};

BOOST_AUTO_TEST_CASE_TEMPLATE(lazy_xor_basic, Sym, symbol_types) {
  const Sym x1 = symbol_traits<Sym>::build(0x11);
  const Sym x2 = symbol_traits<Sym>::build(0x22);
  const Sym x3 = symbol_traits<Sym>::build(0x33);
  lazy_xor<Sym,10> lx1(&x1);
  lazy_xor<Sym,10> lx2(&x2);
  lazy_xor<Sym,10> lx3 = lx1 ^ lx2;
  BOOST_CHECK(lx1.evaluate() == x1);
  BOOST_CHECK(lx2.evaluate() == x2);
  BOOST_CHECK(lx3.evaluate() == x3);
  lazy_xor<Sym,10> empty;
  BOOST_CHECK_THROW(empty.evaluate(), runtime_error);

  BOOST_CHECK_EQUAL(lx1.size(), 1);
  BOOST_CHECK_EQUAL(lx2.size(), 1);
  BOOST_CHECK_EQUAL(lx3.size(), 2);
  BOOST_CHECK_EQUAL(empty.size(), 0);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(lazy_xor_shorthand, Sym, symbol_types) {
  const Sym x1 = symbol_traits<Sym>::build(0x11);
  const Sym x2 = symbol_traits<Sym>::build(0x22);
  const Sym x3 = symbol_traits<Sym>::build(0x33);
  lazy_xor<Sym,10> lx1(&x1);
  BOOST_CHECK(lx1.evaluate() == x1);
  lx1.xor_with(&x2);
  BOOST_CHECK(lx1.evaluate() == x3);
}

// BOOST_AUTO_TEST_CASE_TEMPLATE(lazy_xor_elision, Sym, symbol_types) {
//   const Sym x1 = symbol_traits<Sym>::build(0x11);
//   const Sym x2 = symbol_traits<Sym>::build(0x22);
//   lazy_xor<Sym> lx1(&x1);
//   lazy_xor<Sym> lx2(&x2);
//   lazy_xor<Sym> lx3;
//   lx3 ^= lx1;
//   BOOST_CHECK_EQUAL(lx3.size(), 1);
//   lx3 ^= lx2;
//   BOOST_CHECK_EQUAL(lx3.size(), 2);
//   lx3 ^= lx1;
//   BOOST_CHECK_EQUAL(lx3.size(), 1);
//   BOOST_CHECK(lx3.evaluate() == x2);
// }

// BOOST_AUTO_TEST_CASE(lazy_xor_fp) {
//   vector<fountain_packet> v;
//   for (int i = 0; i < 10; ++i) {
//     v.push_back(fountain_packet(1024, 0x11 * (i+1)));
//   }
//   BOOST_CHECK_EQUAL(v[0][0], 0x11);
//   BOOST_CHECK_EQUAL(v[9][0], (char)0xaa);

//   lazy_xor<fountain_packet> lx;
//   for (auto i = v.cbegin(); i != v.cend(); ++i) {
//     lx ^= lazy_xor<fountain_packet>(&(*i));
//   }
//   BOOST_CHECK_EQUAL(lx.size(), 10);
//   BOOST_CHECK(lx.evaluate() == packet(1024, 0xbb));
// }

BOOST_AUTO_TEST_CASE(lazy_xor_pktsize) {
  fountain_packet p1(11, 0x11);
  fountain_packet p2(10, 0x22);

  lazy_xor<packet,2> lz1(&p1), lz2(&p2);
  lz1 ^= lz2;
  BOOST_CHECK_THROW(lz1.evaluate(), runtime_error);
  BOOST_CHECK_NO_THROW(lz2.evaluate());

  lazy_xor<fountain_packet,2> flz1(&p1), flz2(&p2);
  flz1 ^= flz2;
  BOOST_CHECK_THROW(flz1.evaluate(), runtime_error);
  BOOST_CHECK_NO_THROW(flz2.evaluate());
}
