#define BOOST_TEST_MODULE test_message_passing

#include <boost/mpl/vector.hpp>
#include <boost/test/unit_test.hpp>

#include "lazy_xor.hpp"
#include "message_passing.hpp"
#include "packets.hpp"

using namespace std;
using namespace uep;

template <class T>
bool operator==(const lazy_xor<T> &lhs, const lazy_xor<T> &rhs) {
  return lhs.evaluate() == rhs.evaluate();
}

struct fpSel {typedef fountain_packet sym_type;};
struct uintSel {typedef unsigned int sym_type;};
struct lazySel {typedef lazy_xor<fountain_packet> sym_type;};

typedef boost::mpl::vector<uintSel, fpSel, lazySel> sym_selectors;

template <class SymSel>
struct sym_builder_t {};

template <>
struct sym_builder_t<fpSel> {
  fountain_packet operator()(char i) const {
    const size_t size = 1024;
    return fountain_packet(size, i);
  }
};

template <>
struct sym_builder_t<uintSel> {
  unsigned int operator()(unsigned int i) const {
    return i;
  }
};

template <>
struct sym_builder_t<lazySel> {
  lazySel::sym_type operator()(char i) {
    const size_t size = 1024;
    fountain_packet *fp = new fountain_packet(size, i);
    deleteme.push_back(fp);
    return lazy_xor<fountain_packet>(fp);
  }

  ~sym_builder_t() {
    for (fountain_packet *fp : deleteme) {
      delete fp;
    }
  }
private:
  std::list<fountain_packet*> deleteme;
};

template <class SymbolSel>
struct mp_setup {
  typedef typename SymbolSel::sym_type sym_type;
  typedef message_passing_context<sym_type> mp_t;
  mp_t *mp;
  mp_t *fail;
  mp_t *partial;
  size_t K = 3;
  vector<sym_type> expected;
  vector<sym_type> out;
  vector<pair<size_t,size_t>> edges, decodeable_edges;
  sym_builder_t<SymbolSel> sym_builder;

  mp_setup() {
    expected.push_back(sym_builder(0x00));
    expected.push_back(sym_builder(0x11));
    expected.push_back(sym_builder(0x22));

    out.push_back(sym_builder(0x11));
    out.push_back(sym_builder(0x22));
    out.push_back(sym_builder(0x33));
    out.push_back(sym_builder(0x44));

    edges.push_back(make_pair(1,0));
    edges.push_back(make_pair(1,2));
    edges.push_back(make_pair(1,3));
    edges.push_back(make_pair(2,2));
    edges.push_back(make_pair(2,3));
    edges.push_back(make_pair(0,1));
    edges.push_back(make_pair(2,1));

    decodeable_edges = edges;

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

BOOST_FIXTURE_TEST_CASE_TEMPLATE(mp_build, SymSel, sym_selectors, mp_setup<SymSel>) {
  typedef mp_setup<SymSel> S;

  BOOST_CHECK_EQUAL(S::mp->input_size(), S::K);
  BOOST_CHECK_EQUAL(S::mp->output_size(), S::out.size());
  BOOST_CHECK(!S::mp->has_decoded());
  BOOST_CHECK_EQUAL(S::mp->decoded_count(), 0);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(mp_decoding, SymSel, sym_selectors, mp_setup<SymSel>) {
  typedef mp_setup<SymSel> S;

  S::mp->run();
  BOOST_CHECK(S::mp->has_decoded());
  BOOST_CHECK(equal(S::mp->decoded_symbols_begin(), S::mp->decoded_symbols_end(), S::expected.cbegin()));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(mp_multiple, SymSel, sym_selectors, mp_setup<SymSel>) {
  typedef mp_setup<SymSel> S;

  S::mp->run();
  S::mp->run();
  S::mp->run();
  BOOST_CHECK(S::mp->has_decoded());
  BOOST_CHECK(equal(S::mp->decoded_symbols_begin(), S::mp->decoded_symbols_end(), S::expected.cbegin()));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(mp_fail, SymSel, sym_selectors, mp_setup<SymSel>) {
  typedef mp_setup<SymSel> S;

  BOOST_CHECK(!S::fail->has_decoded());
  S::fail->run();
  BOOST_CHECK(!S::fail->has_decoded());
  BOOST_CHECK_EQUAL(S::fail->decoded_count(), 0);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(mp_partial, SymSel, sym_selectors, mp_setup<SymSel>) {
  typedef mp_setup<SymSel> S;

  BOOST_CHECK(!S::partial->has_decoded());
  S::partial->run();
  BOOST_CHECK(!S::partial->has_decoded());
  BOOST_CHECK_EQUAL(S::partial->decoded_count(), 1);
  BOOST_CHECK(*(S::partial->decoded_symbols_begin()) == S::expected[1]);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(mp_retry_partial, SymSel, sym_selectors, mp_setup<SymSel>) {
  typedef mp_setup<SymSel> S;

  S::partial->run();
  S::partial->run();
  S::partial->run();
  S::partial->run();
  BOOST_CHECK(!S::partial->has_decoded());
  BOOST_CHECK_EQUAL(S::partial->decoded_count(), 1);
  BOOST_CHECK(*(S::partial->decoded_symbols_begin()) == S::expected[1]);
}

BOOST_AUTO_TEST_CASE(lazy_xor_basic) {
  const char x1 = 0x11, x2 = 0x22;
  lazy_xor<char> lx1(&x1);
  lazy_xor<char> lx2(&x2);
  lazy_xor<char> lx3 = lx1 ^ lx2;
  BOOST_CHECK_EQUAL(lx1.evaluate(), 0x11);
  BOOST_CHECK_EQUAL(lx2.evaluate(), 0x22);
  BOOST_CHECK_EQUAL(lx3.evaluate(), 0x33);
  BOOST_CHECK_THROW(lazy_xor<char>().evaluate(), runtime_error);

  BOOST_CHECK_EQUAL(lx1.size(), 1);
  BOOST_CHECK_EQUAL(lx2.size(), 1);
  BOOST_CHECK_EQUAL(lx3.size(), 2);
  BOOST_CHECK_EQUAL(lazy_xor<char>().size(), 0);

  BOOST_CHECK_EQUAL(x1, 0x11);
  BOOST_CHECK_EQUAL(x2, 0x22);
}

BOOST_AUTO_TEST_CASE(lazy_xor_elision) {
  const char x1 = 0x11, x2 = 0x22;
  lazy_xor<char> lx1(&x1);
  lazy_xor<char> lx2(&x2);
  lazy_xor<char> lx3;
  lx3 ^= lx1;
  BOOST_CHECK_EQUAL(lx3.size(), 1);
  lx3 ^= lx2;
  BOOST_CHECK_EQUAL(lx3.size(), 2);
  lx3 ^= lx1;
  BOOST_CHECK_EQUAL(lx3.size(), 1);
  BOOST_CHECK_EQUAL(lx3.evaluate(), 0x22);
}

BOOST_AUTO_TEST_CASE(lazy_xor_fp) {
  vector<fountain_packet> v;
  for (int i = 0; i < 10; ++i) {
    v.push_back(fountain_packet(1024, 0x11 * (i+1)));
  }
  BOOST_CHECK_EQUAL(v[0][0], 0x11);
  BOOST_CHECK_EQUAL(v[9][0], (char)0xaa);

  lazy_xor<fountain_packet> lx;
  for (auto i = v.cbegin(); i != v.cend(); ++i) {
    lx ^= lazy_xor<fountain_packet>(&(*i));
  }
  BOOST_CHECK_EQUAL(lx.size(), 10);
  BOOST_CHECK(lx.evaluate() == packet(1024, 0xbb));
}

BOOST_AUTO_TEST_CASE(lazy_xor_wrong) {
  fountain_packet p1(11, 0x11);
  fountain_packet p2(10, 0x22);

  lazy_xor<packet> lz1(&p1), lz2(&p2);
  lz1 ^= lz2;
  BOOST_CHECK_THROW(lz1.evaluate(), runtime_error);
  BOOST_CHECK_NO_THROW(lz2.evaluate());

  lazy_xor<fountain_packet> flz1(&p1), flz2(&p2);
  flz1 ^= flz2;
  BOOST_CHECK_THROW(flz1.evaluate(), runtime_error);
  BOOST_CHECK_NO_THROW(flz2.evaluate());
}

struct two_mp_setup : public mp_setup<uintSel> {
  mp_t *mp2;
  const vector<unsigned int> out2{1,2};
  const size_t K2 = 2;
  vector<pair<size_t,size_t>> edges2;

  two_mp_setup() {
    edges2.push_back(make_pair(0,1));
    edges2.push_back(make_pair(1,0));
    mp2 = new mp_t(K2, out2.cbegin(), out2.cend(), edges2.cbegin(), edges2.cend());
  }

  ~two_mp_setup() {
    delete mp2;
  }
};

BOOST_FIXTURE_TEST_CASE(mp_copy, two_mp_setup) {
  mp2->run();
  BOOST_CHECK(mp2->has_decoded());
  BOOST_CHECK(equal(mp2->decoded_symbols_begin(),
		    mp2->decoded_symbols_end(), out2.crbegin()));

  *mp2 = *mp;
  mp->run();
  BOOST_CHECK(!mp2->has_decoded());
  mp2->run();
  BOOST_CHECK(mp2->has_decoded());
  BOOST_CHECK(equal(mp2->decoded_symbols_begin(),
		    mp2->decoded_symbols_end(), expected.cbegin()));
}

BOOST_FIXTURE_TEST_CASE(mp_move, two_mp_setup) {
  mp2->run();
  BOOST_CHECK(mp2->has_decoded());
  BOOST_CHECK_EQUAL(mp2->input_size(), 2);
  BOOST_CHECK(mp2->decoded_begin() != mp2->decoded_end());
  *mp2 = mp_t();
  BOOST_CHECK_EQUAL(mp2->input_size(), 0);
  BOOST_CHECK(mp2->has_decoded());
  BOOST_CHECK(mp2->decoded_begin() == mp2->decoded_end());
}

BOOST_FIXTURE_TEST_CASE(mp_copy_c, two_mp_setup) {
  mp_t mp3(*mp2);
  BOOST_CHECK(!mp3.has_decoded());
  BOOST_CHECK_EQUAL(mp3.input_size(), 2);
  mp2->run();
  BOOST_CHECK(mp2->has_decoded());
  BOOST_CHECK(equal(mp2->decoded_symbols_begin(),
		    mp2->decoded_symbols_end(), out2.crbegin()));

  mp_t mp4(*mp2);
  BOOST_CHECK(mp4.has_decoded());
  BOOST_CHECK(equal(mp4.decoded_symbols_begin(),
		    mp4.decoded_symbols_end(), out2.crbegin()));
}

BOOST_FIXTURE_TEST_CASE(mp_move_c, two_mp_setup) {
  mp_t mp3(mp_t(10));
  BOOST_CHECK_EQUAL(mp3.input_size(), 10);
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(mp_incremental_build,
				 SymSel, sym_selectors, mp_setup<SymSel>) {
  typedef mp_setup<SymSel> S;

  typename S::mp_t mp(3, S::out.cbegin(), S::out.cend());
  BOOST_CHECK_EQUAL(mp.input_size(), 3);
  BOOST_CHECK_EQUAL(mp.output_size(), 4);

  for (auto i = S::decodeable_edges.cbegin();
       i != S::decodeable_edges.cend(); ++i) {
    mp.add_edge(i->first, i->second);
  }

  mp.run();
  BOOST_CHECK(mp.has_decoded());
  BOOST_CHECK(equal(mp.decoded_symbols_begin(),
		    mp.decoded_symbols_end(),
		    S::expected.cbegin()));
}
