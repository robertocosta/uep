#define BOOST_TEST_MODULE test_message_passing

#include <boost/mpl/vector.hpp>
#include <boost/test/unit_test.hpp>

#include "message_passing.hpp"
#include "packets.hpp"

using namespace std;
using namespace uep;

struct fpSel {typedef fountain_packet sym_type;};
struct uintSel {typedef unsigned int sym_type;};

typedef boost::mpl::vector<uintSel, fpSel> sym_selectors;

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
  vector<pair<size_t,size_t>> edges;
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
