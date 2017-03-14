#define BOOST_TEST_MODULE test_encoder_decoder
#include <boost/test/unit_test.hpp>

#include "decoder.hpp"
#include "encoder.hpp"
#include "log.hpp"

#include <climits>
#include <map>
#include <fstream>

using namespace std;
using namespace uep;

// namespace expr = boost::log::expressions;
// namespace keywords = boost::log::keywords;
// namespace logging = boost::log;
// namespace sinks = boost::log::sinks;
// namespace src = boost::log::sources;

packet random_pkt(int size) {
  static std::independent_bits_engine<std::mt19937, CHAR_BIT, unsigned char> g;
  packet p;
  p.resize(size);
  for (int i=0; i < size; i++) {
    p[i] = g();
  }
  return p;
}

// void histogram_print_csv(const std::map<int, int> &h, const std::string &fname) {
//   using namespace std;
//   ofstream ofs(fname, ios_base::trunc);
//   ofs << "required_packets, histo_count" << endl;
//   for (auto i = h.cbegin(); i != h.cend(); ++i) {
//     int x = i->first;
//     int y = i->second;
//     ofs << x << ", " << y << endl;
//   }
//   ofs.close();
// }

struct global_fixture {
  boost::shared_ptr<default_cerr_sink> the_cerr_sink;

  global_fixture() {
    the_cerr_sink = make_cerr_sink(warning);
    boost::log::core::get()->add_sink(the_cerr_sink);
  }

  ~global_fixture() {
    boost::log::core::get()->remove_sink(the_cerr_sink);
  }
};

BOOST_GLOBAL_FIXTURE(global_fixture);

BOOST_AUTO_TEST_CASE(check_encoder_counters) {
  const size_t L = 10;
  const size_t K = 100;
  const double c = 0.1;
  const double delta = 0.5;

  vector<packet> original;
  lt_encoder<std::mt19937> enc(K, c, delta);
  BOOST_CHECK(!enc.has_block());
  for (size_t i = 0; i < 10*K; ++i) {
    packet p = random_pkt(L);
    original.push_back(p);
    enc.push(p);
  }
  BOOST_CHECK(enc.has_block());
  BOOST_CHECK_EQUAL(enc.K(), K);
  BOOST_CHECK_EQUAL(enc.size(), 10*K);
  BOOST_CHECK_EQUAL(enc.queue_size(), 9*K);

  BOOST_CHECK_EQUAL(enc.blockno(), 0);

  for (size_t i = 0; i < 2*K; ++i) {
    fountain_packet fp = enc.next_coded();
    BOOST_CHECK_EQUAL(fp.size(), L);
    BOOST_CHECK_EQUAL(fp.sequence_number(), i);
    BOOST_CHECK_EQUAL(fp.block_number(), 0);
    BOOST_CHECK_EQUAL(fp.block_seed(), enc.block_seed());
  }
  BOOST_CHECK_EQUAL(enc.seqno(), 2*K-1);

  enc.next_block();

  BOOST_CHECK_EQUAL(enc.blockno(), 1);
  fountain_packet fp = enc.next_coded();
  BOOST_CHECK_EQUAL(enc.seqno(), 0);
  BOOST_CHECK_EQUAL(fp.size(), L);
  BOOST_CHECK_EQUAL(fp.sequence_number(), 0);
  BOOST_CHECK_EQUAL(fp.block_number(), 1);
  BOOST_CHECK_EQUAL(fp.block_seed(), enc.block_seed());
};

// BOOST_AUTO_TEST_CASE(seqno_overflows) {
//   int L = 10;
//   int K = 10;
//   double c = 0.03;
//   double delta = 0.5;

//   degree_distribution deg = robust_soliton_distribution(K,c,delta);
//   fountain_encoder<std::mt19937> enc(deg);

//   for (int i = 0; i < K; i++) {
//     packet p = random_pkt(L);
//     enc.push_input(move(p));
//   }

//   BOOST_CHECK_NO_THROW(for (int i = 0; i <= fountain_encoder<>::MAX_SEQNO; ++i) {
//       enc.next_coded();
//     });

//   BOOST_CHECK_THROW(enc.next_coded(), exception);
// }

// BOOST_AUTO_TEST_CASE(blockno_overflows) {
//   int L = 10;
//   int K = 2;
//   double c = 0.03;
//   double delta = 0.5;

//   degree_distribution deg = robust_soliton_distribution(K,c,delta);
//   fountain_encoder<std::mt19937> enc(deg);

//   for (int i = 0; i < fountain_encoder<>::MAX_BLOCKNO; i++) {
//     for (int j = 0; j < K; ++j) {
//       packet p = random_pkt(L);
//       enc.push_input(move(p));
//     }
//     BOOST_CHECK_NO_THROW(enc.discard_block());
//     BOOST_CHECK_EQUAL(enc.blockno(), i+1);
//   }
//   BOOST_CHECK_NO_THROW(enc.discard_block());
//   BOOST_CHECK_EQUAL(enc.blockno(), 0);
// }

struct encdec_setup {
  size_t L;
  size_t K;
  double c;
  double delta;
  vector<packet> original;
  lt_row_generator rowgen;
  lt_encoder<std::mt19937> enc;
  lt_decoder dec;

  encdec_setup(size_t L_, size_t K_, double c_, double delta_) :
    L(L_), K(K_), c(c_), delta(delta_),
    rowgen(make_robust_lt_row_generator(K, c, delta)),
    enc(rowgen),
    dec(rowgen) {
    gen_pkts(K);
  }

  void gen_pkts(size_t num) {
    for (size_t i = 0; i < num; i++) {
      packet p = random_pkt(L);
      original.push_back(move(p));
    }
  }
};

BOOST_AUTO_TEST_CASE(correct_decoding) {
  encdec_setup s(1500, 100, 0.1, 0.5);

  for (auto i = s.original.cbegin(); i != s.original.cend(); ++i) {
    s.enc.push(*i);
  }

  BOOST_CHECK_EQUAL(s.enc.size(), s.K);
  BOOST_CHECK(s.enc.has_block());

  BOOST_CHECK(!s.dec.has_decoded());
  while (!s.dec.has_decoded()) {
    fountain_packet p = s.enc.next_coded();
    s.dec.push(p);
  }
  BOOST_CHECK(s.dec.has_decoded());
  for (auto i = s.original.cbegin(); i != s.original.cend(); ++i) {
    packet out = s.dec.next_decoded();
    BOOST_CHECK(*i == out);
  }
  BOOST_CHECK(equal(s.dec.decoded_begin(), s.dec.decoded_end(),
		    s.original.cbegin()));
}

BOOST_AUTO_TEST_CASE(multiple_blocks) {
  encdec_setup s(4, 10, 0.1, 0.5);
  s.gen_pkts((50-1)*s.K);
  for (auto i = s.original.cbegin(); i != s.original.cend(); ++i) {
    s.enc.push(*i);
  }
  int i = 0;
  while (s.enc.has_block()) {
    do {
      fountain_packet p = s.enc.next_coded();
      s.dec.push(p);
    } while (!s.dec.has_decoded());
    BOOST_CHECK(equal(s.dec.decoded_begin(), s.dec.decoded_end(),
		      s.original.cbegin()+i*s.K));
    ++i;
    s.enc.next_block();
  }

  BOOST_CHECK_EQUAL(s.dec.queue_size(), 50*s.K);

  for (auto i = s.original.cbegin(); i != s.original.cend(); ++i) {
    packet out = s.dec.next_decoded();
    BOOST_CHECK(*i == out);
  }
}

BOOST_AUTO_TEST_CASE(drop_packets) {
  encdec_setup s(4, 500, 0.1, 0.5);
  double p = 0.9;
  mt19937 drop_gen;
  bernoulli_distribution drop_dist(p);

  for (auto i = s.original.cbegin(); i != s.original.cend(); ++i) {
    s.enc.push(*i);
  }

  do {
    fountain_packet p = s.enc.next_coded();
    if (!drop_dist(drop_gen))
      s.dec.push(p);
  } while (!s.dec.has_decoded());
  BOOST_CHECK(equal(s.dec.decoded_begin(), s.dec.decoded_end(),
		    s.original.cbegin()));
}

BOOST_AUTO_TEST_CASE(drop_blocks) {
  encdec_setup s(4, 10, 0.1, 0.5);
  s.gen_pkts((100-1)*s.K);
  double p = 0.6;
  mt19937 drop_gen;
  bernoulli_distribution drop_dist(p);

  for (auto i = s.original.cbegin(); i != s.original.cend(); ++i) {
    s.enc.push(*i);
  }

  int i = 0;
  while (s.enc.has_block()) {
    if (!drop_dist(drop_gen)) {
      do {
	fountain_packet p = s.enc.next_coded();
	s.dec.push(p);
      } while (!s.dec.has_decoded());
      BOOST_CHECK(equal(s.dec.decoded_begin(), s.dec.decoded_end(),
			s.original.cbegin()+i*s.K));
    }
    ++i;
    s.enc.next_block();
  }
}

BOOST_AUTO_TEST_CASE(reorder_drop_packets) {
  encdec_setup s(4, 500, 0.1, 0.5);

  for (auto i = s.original.cbegin(); i != s.original.cend(); ++i) {
    s.enc.push(*i);
  }

  vector<fountain_packet> coded;
  for (size_t i = 0; i < 3*s.K; ++i)
    coded.push_back(s.enc.next_coded());

  shuffle(coded.begin(), coded.end(), std::mt19937());

  for (auto i = coded.cbegin(); i != coded.cend(); ++i) {
    s.dec.push(*i);
  }

  BOOST_CHECK(s.dec.has_decoded());
  BOOST_CHECK(equal(s.dec.decoded_begin(), s.dec.decoded_end(),
		    s.original.cbegin()));
}

struct decoder_overflow_fixture {
  int L = 10;
  int K = 2;
  double c = 0.03;
  double delta = 0.01;
  int N = 100;
  int nblocks = 0xffff * 2;

  int last_block_passed = 0xffff - 50;

  robust_soliton_distribution deg;
  lt_encoder<std::mt19937> enc;
  lt_decoder dec;

  vector<packet> original;

  decoder_overflow_fixture() :
    deg(K,c,delta), enc(deg), dec(deg) {
    for (int i = 0; i < nblocks*K; i++) {
      packet p = random_pkt(L);
      original.push_back(p);
      enc.push(move(p));
    }

    for (int i = 0; i <= last_block_passed; ++i) {
      pass_next_block(i);
    }
  }

  void pass_next_block(int i) {
    for (int j = 0; j < N; ++j) {
      fountain_packet p = enc.next_coded();
      dec.push(move(p));
      if (dec.has_decoded()) break;
    }
    BOOST_CHECK(dec.has_decoded());
    BOOST_CHECK(equal(original.cbegin() + i*K,
		      original.cbegin() + (i+1)*K,
		      dec.decoded_begin()));
    enc.next_block();
  }
};

BOOST_FIXTURE_TEST_CASE(decoder_overflow, decoder_overflow_fixture) {
  for (++last_block_passed;
       last_block_passed <= 0xffff;
       ++last_block_passed) {
    pass_next_block(last_block_passed);
  }
  BOOST_CHECK_EQUAL(enc.blockno(), 0);
  BOOST_CHECK_EQUAL(dec.blockno(), 0xffff);

  pass_next_block(last_block_passed);
  BOOST_CHECK_EQUAL(dec.blockno(), 0);
  pass_next_block(++last_block_passed);
  BOOST_CHECK_EQUAL(dec.blockno(), 1);
}

BOOST_FIXTURE_TEST_CASE(decoder_overflow_jump, decoder_overflow_fixture) {
  for (int i = 0; i < 0xffff - last_block_passed + 10; ++i)
    enc.next_block();
  BOOST_CHECK_EQUAL(enc.blockno(), 10);
  pass_next_block(0xffff + 11);
  BOOST_CHECK_EQUAL(dec.blockno(), 10);
}

BOOST_FIXTURE_TEST_CASE(decoder_ignore_jump_back, decoder_overflow_fixture) {
  vector<fountain_packet> old_block;
  size_t old_blockno = last_block_passed+1;
  for (int i = 0; i < N; ++i) {
    old_block.push_back(enc.next_coded());
  }
  enc.next_block();

  pass_next_block(last_block_passed+2);
  pass_next_block(last_block_passed+3);

  for (auto i = old_block.cbegin(); i != old_block.cend(); ++i) {
    dec.push(*i);
  }
  BOOST_CHECK_EQUAL(dec.blockno(), last_block_passed+3);

  for (int i = last_block_passed+4; i <= 0xffff + 3; ++i) {
    pass_next_block(i);
  }

  BOOST_CHECK(old_blockno > dec.blockno());
  for (auto i = old_block.cbegin(); i != old_block.cend(); ++i) {
    dec.push(*i);
  }
  BOOST_CHECK_EQUAL(dec.blockno(), (0xffff+3) & 0xffff);
}

// struct stat_fixture {
//   boost::shared_ptr<default_stat_sink> the_stat_sink;

//   stat_fixture() {
//     boost::shared_ptr<std::ofstream>
//       ofs(new std::ofstream("stat_log.json", std::ios_base::trunc));
//     the_stat_sink = make_stat_sink(ofs);
//     boost::log::core::get()->add_sink(the_stat_sink);
//   }

//   ~stat_fixture() {
//     boost::log::core::get()->remove_sink(the_stat_sink);
//   }
// };
