#define BOOST_TEST_MODULE test_message_passing
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <forward_list>

#include <boost/mpl/vector.hpp>
#include <boost/optional.hpp>

#include "lazy_xor.hpp"
#include "message_passing.hpp"
#include "packets.hpp"

using namespace std;
using namespace uep;
using namespace uep::mp;

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

/* Adapt some symbol types to be used with mp_context.
 * Add a build to construct a symbol starting from a base char.
 * Add missing operator^= and operator==.
 * Prevent copying to ensure that mp_context does not copy symbols.
 */

class my_opt_char : public boost::optional<char> {
public:
  my_opt_char() {}
  my_opt_char(const boost::optional<char> &oc) : boost::optional<char>(oc) {}

  my_opt_char(const my_opt_char&) = delete;
  my_opt_char(my_opt_char&&) = default;
  my_opt_char &operator=(const my_opt_char&) = delete;
  my_opt_char &operator=(my_opt_char&&) = default;
  ~my_opt_char() = default;

  my_opt_char &operator^=(const my_opt_char &rhs) {
    if (!(*this) || !rhs) throw std::runtime_error("Must not be unset");
    (*(*this)) ^= (*rhs);
    return *this;
  }

  static my_opt_char build(char base) {
    return boost::optional<char>(base);
  }
};

class my_packet : public packet {
public:
  my_packet() {}
  my_packet(const packet &p): packet(p) {}

  my_packet(const my_packet&) = delete;
  my_packet(my_packet&&) = default;
  my_packet &operator=(const my_packet&) = delete;
  my_packet &operator=(my_packet&&) = default;
  ~my_packet() = default;

  static my_packet build(char base) {
    return packet(1024, base);
  }
};

class my_lazy_xor_packet : public lazy_xor<packet> {
public:
  my_lazy_xor_packet() {}
  my_lazy_xor_packet(const lazy_xor<packet> &p): lazy_xor<packet>(p) {}

  packet p;

  static forward_list<unique_ptr<packet>> base_packets;
  static my_lazy_xor_packet build(char base) {
    auto bp = make_unique<packet>(1024, base);
    base_packets.push_front(move(bp));
    return lazy_xor<packet>(base_packets.front().get());
  }

  bool operator==(const my_lazy_xor_packet &rhs) const {
    return evaluate() == rhs.evaluate();
  }
};
forward_list<unique_ptr<packet>> my_lazy_xor_packet::base_packets;

class my_lazy_xor_char : public lazy_xor<char> {
public:
  my_lazy_xor_char() {}
  my_lazy_xor_char(const lazy_xor<char> &p): lazy_xor<char>(p) {}

  static forward_list<unique_ptr<char>> base_chars;
  static my_lazy_xor_char build(char base) {
    auto bc = make_unique<char>(base);
    base_chars.push_front(move(bc));
    return lazy_xor<char>(base_chars.front().get());
  }

  bool operator==(const my_lazy_xor_char &rhs) const {
    return evaluate() == rhs.evaluate();
  }
};
forward_list<unique_ptr<char>> my_lazy_xor_char::base_chars;

/** Symbol types used to test the mp_context. */
typedef boost::mpl::vector<my_opt_char,
			   my_packet,
			   my_lazy_xor_packet,
			   my_lazy_xor_char
			   > symbol_types;

/** Setup some basic bipartite graphs to test. */
template <class Sym>
struct mp_setup {
  typedef mp::mp_context<Sym> mp_t;
  typedef Sym sym_t;

  mp_t *mp;
  mp_t *fail;
  mp_t *partial;
  size_t K = 3;
  size_t N = 4;
  vector<sym_t> expected;

  mp_setup() {
    mp = new mp_t(K);
    partial = new mp_t(K);
    fail = new mp_t(K);

    expected.push_back(Sym::build(0x00));
    expected.push_back(Sym::build(0x11));
    expected.push_back(Sym::build(0x22));

    std::vector<size_t> out_edges;

    out_edges = {1};
    mp->add_output(Sym::build(0x11), out_edges.cbegin(), out_edges.cend());
    out_edges = {0,2};
    mp->add_output(Sym::build(0x22), out_edges.cbegin(), out_edges.cend());
    out_edges = {1,2};
    mp->add_output(Sym::build(0x33), out_edges.cbegin(), out_edges.cend());
    out_edges = {1,2};
    mp->add_output(Sym::build(0x33), out_edges.cbegin(), out_edges.cend());

    out_edges = {1};
    partial->add_output(Sym::build(0x11), out_edges.cbegin(), out_edges.cend());
    out_edges = {0,2};
    partial->add_output(Sym::build(0x22), out_edges.cbegin(), out_edges.cend());
    out_edges = {0,1,2};
    partial->add_output(Sym::build(0x33), out_edges.cbegin(), out_edges.cend());
    out_edges = {0,1,2};
    partial->add_output(Sym::build(0x33), out_edges.cbegin(), out_edges.cend());

    out_edges = {0,1};
    fail->add_output(Sym::build(0x11), out_edges.cbegin(), out_edges.cend());
    out_edges = {0,2};
    fail->add_output(Sym::build(0x22), out_edges.cbegin(), out_edges.cend());
    out_edges = {0,1,2};
    fail->add_output(Sym::build(0x33), out_edges.cbegin(), out_edges.cend());
    out_edges = {0,1,2};
    fail->add_output(Sym::build(0x33), out_edges.cbegin(), out_edges.cend());
  }

  ~mp_setup() {
    delete mp;
    delete fail;
    delete partial;
  }
};

BOOST_FIXTURE_TEST_CASE_TEMPLATE(mp_build, Sym, symbol_types, mp_setup<Sym>) {
  typedef mp_setup<Sym> S;

  BOOST_CHECK_EQUAL(S::mp->input_size(), S::K);
  BOOST_CHECK_EQUAL(S::mp->output_size(), S::N);
  BOOST_CHECK(!S::mp->has_decoded());
  BOOST_CHECK_EQUAL(S::mp->decoded_count(), 0);
  for (auto i = S::mp->input_symbols_begin();
       i != S::mp->input_symbols_end();
       ++i) {
    const typename S::sym_t &sym = *i;
    BOOST_CHECK(!static_cast<bool>(sym));
  }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(mp_decoding, Sym, symbol_types, mp_setup<Sym>) {
  typedef mp_setup<Sym> S;

  S::mp->run();
  BOOST_CHECK(S::mp->has_decoded());
  BOOST_CHECK(equal(S::mp->decoded_symbols_begin(), S::mp->decoded_symbols_end(), S::expected.cbegin()));
  BOOST_CHECK(equal(S::mp->input_symbols_begin(), S::mp->input_symbols_end(), S::expected.cbegin()));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(mp_multiple, Sym, symbol_types, mp_setup<Sym>) {
  typedef mp_setup<Sym> S;

  S::mp->run();
  S::mp->run();
  S::mp->run();
  BOOST_CHECK(S::mp->has_decoded());
  BOOST_CHECK(equal(S::mp->input_symbols_begin(), S::mp->input_symbols_end(), S::expected.cbegin()));
  BOOST_CHECK(equal(S::mp->decoded_symbols_begin(), S::mp->decoded_symbols_end(), S::expected.cbegin()));
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(mp_fail, Sym, symbol_types, mp_setup<Sym>) {
  typedef mp_setup<Sym> S;

  BOOST_CHECK(!S::fail->has_decoded());
  S::fail->run();
  BOOST_CHECK(!S::fail->has_decoded());
  BOOST_CHECK_EQUAL(S::fail->decoded_count(), 0);
  for (auto i = S::fail->input_symbols_begin();
       i != S::fail->input_symbols_end();
       ++i) {
    const typename S::sym_t &sym = *i;
    BOOST_CHECK(!static_cast<bool>(sym));
  }
  BOOST_CHECK(S::fail->decoded_symbols_begin() ==
	      S::fail->decoded_symbols_end());
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(mp_partial, Sym, symbol_types, mp_setup<Sym>) {
  typedef mp_setup<Sym> S;

  BOOST_CHECK(!S::partial->has_decoded());
  S::partial->run();
  BOOST_CHECK(!S::partial->has_decoded());
  BOOST_CHECK_EQUAL(S::partial->decoded_count(), 1);
  BOOST_CHECK(*(S::partial->decoded_symbols_begin()) == S::expected[1]);

  auto i = S::partial->input_symbols_begin();
  BOOST_CHECK(!(*i));
  ++i;
  BOOST_CHECK(static_cast<bool>(*i));
  BOOST_CHECK(*i == S::expected[1]);
  ++i;
  BOOST_CHECK(!(*i));
  ++i;
  BOOST_CHECK(i == S::partial->input_symbols_end());
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(mp_retry_partial, Sym, symbol_types, mp_setup<Sym>) {
  typedef mp_setup<Sym> S;

  S::partial->run();
  S::partial->run();
  S::partial->run();
  S::partial->run();
  BOOST_CHECK(!S::partial->has_decoded());
  BOOST_CHECK_EQUAL(S::partial->decoded_count(), 1);
  BOOST_CHECK(*(S::partial->decoded_symbols_begin()) == S::expected[1]);

  auto i = S::partial->input_symbols_begin();
  BOOST_CHECK(!(*i));
  ++i;
  BOOST_CHECK(static_cast<bool>(*i));
  BOOST_CHECK(*i == S::expected[1]);
  ++i;
  BOOST_CHECK(!(*i));
  ++i;
  BOOST_CHECK(i == S::partial->input_symbols_end());
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(mp_move_after_run, Sym, symbol_types, mp_setup<Sym>) {
  typedef mp_setup<Sym> S;

  S::mp->run();
  BOOST_CHECK(S::mp->has_decoded());
  BOOST_CHECK(equal(S::mp->input_symbols_begin(),
		    S::mp->input_symbols_end(), S::expected.cbegin()));

  typename S::mp_t *other = new typename S::mp_t(99);
  BOOST_CHECK(!other->has_decoded());
  BOOST_CHECK_EQUAL(other->input_size(), 99);
  BOOST_CHECK_NE(S::mp->input_size(), 99);

  swap(*other, *S::mp);

  BOOST_CHECK(other->has_decoded());
  BOOST_CHECK_EQUAL(other->input_size(), S::K);
  BOOST_CHECK(equal(other->input_symbols_begin(),
		    other->input_symbols_end(), S::expected.cbegin()));

  BOOST_CHECK_EQUAL(S::mp->input_size(), 99);
  BOOST_CHECK(!S::mp->has_decoded());

  delete other;
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(mp_move_before_run, Sym, symbol_types, mp_setup<Sym>) {
  typedef mp_setup<Sym> S;

  typename S::mp_t *other = new typename S::mp_t(99);
  swap(*other, *S::mp);
  other->run();

  BOOST_CHECK(other->has_decoded());
  BOOST_CHECK(equal(other->input_symbols_begin(),
		    other->input_symbols_end(), S::expected.cbegin()));

  BOOST_CHECK_EQUAL(S::mp->input_size(), 99);
  BOOST_CHECK(!S::mp->has_decoded());

  S::mp->run();

  BOOST_CHECK(other->has_decoded());
  BOOST_CHECK(equal(other->input_symbols_begin(),
		    other->input_symbols_end(), S::expected.cbegin()));

  BOOST_CHECK_EQUAL(S::mp->input_size(), 99);
  BOOST_CHECK(!S::mp->has_decoded());

  delete other;
}

BOOST_AUTO_TEST_CASE_TEMPLATE(mp_reset, Sym, symbol_types) {
  size_t K = 3;
  size_t N = 4;
  vector<Sym> expected;

  expected.push_back(Sym::build(0x00));
  expected.push_back(Sym::build(0x11));
  expected.push_back(Sym::build(0x22));

  mp_context<Sym> mp(K);
  std::vector<size_t> out_edges;

  out_edges = {1};
  mp.add_output(Sym::build(0x11), out_edges.cbegin(), out_edges.cend());
  out_edges = {0,2};
  mp.add_output(Sym::build(0x22), out_edges.cbegin(), out_edges.cend());
  out_edges = {1,2};
  mp.add_output(Sym::build(0x33), out_edges.cbegin(), out_edges.cend());
  out_edges = {1,2};
  mp.add_output(Sym::build(0x33), out_edges.cbegin(), out_edges.cend());

  BOOST_CHECK_EQUAL(mp.output_size(), N);
  BOOST_CHECK_EQUAL(mp.decoded_count(), 0);
  BOOST_CHECK(!mp.has_decoded());
  BOOST_CHECK(mp.decoded_symbols_begin() == mp.decoded_symbols_end());

  mp.run();

  BOOST_CHECK_EQUAL(mp.output_size(), N);
  BOOST_CHECK_EQUAL(mp.decoded_count(), K);
  BOOST_CHECK(mp.has_decoded());
  BOOST_CHECK(equal(mp.decoded_symbols_begin(), mp.decoded_symbols_end(),
		    expected.cbegin()));

  mp.reset();

  BOOST_CHECK_EQUAL(mp.output_size(), 0);
  BOOST_CHECK_EQUAL(mp.decoded_count(), 0);
  BOOST_CHECK(!mp.has_decoded());
  BOOST_CHECK(mp.decoded_symbols_begin() == mp.decoded_symbols_end());

  out_edges = {1};
  mp.add_output(Sym::build(0x11), out_edges.cbegin(), out_edges.cend());
  out_edges = {0,2};
  mp.add_output(Sym::build(0x22), out_edges.cbegin(), out_edges.cend());
  out_edges = {1,2};
  mp.add_output(Sym::build(0x33), out_edges.cbegin(), out_edges.cend());
  out_edges = {1,2};
  mp.add_output(Sym::build(0x33), out_edges.cbegin(), out_edges.cend());

  BOOST_CHECK_EQUAL(mp.output_size(), N);
  BOOST_CHECK_EQUAL(mp.decoded_count(), 0);
  BOOST_CHECK(!mp.has_decoded());
  BOOST_CHECK(mp.decoded_symbols_begin() == mp.decoded_symbols_end());

  mp.run();

  BOOST_CHECK_EQUAL(mp.output_size(), N);
  BOOST_CHECK_EQUAL(mp.decoded_count(), K);
  BOOST_CHECK(mp.has_decoded());
  BOOST_CHECK(equal(mp.decoded_symbols_begin(), mp.decoded_symbols_end(),
		    expected.cbegin()));
}
