#define BOOST_TEST_MODULE encdec_test
#include <boost/test/unit_test.hpp>

#include "decoder.hpp"
#include "encoder.hpp"
#include "log.hpp"

#include <climits>
#include <map>
#include <fstream>

using namespace std;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace src = boost::log::sources;

packet random_pkt(int size) {
  static std::independent_bits_engine<std::mt19937, CHAR_BIT, unsigned char> g;
  packet p;
  p.resize(size);
  for (int i=0; i < size; i++) {
    p[i] = g();
  }
  return p;
}

void histogram_print_csv(const std::map<int, int> &h, const std::string &fname) {
  using namespace std;
  ofstream ofs(fname, ios_base::trunc);
  ofs << "required_packets, histo_count" << endl;
  for (auto i = h.cbegin(); i != h.cend(); ++i) {
    int x = i->first;
    int y = i->second;
    ofs << x << ", " << y << endl;
  }
  ofs.close();
}

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

BOOST_AUTO_TEST_CASE(seqno_overflows) {
  int L = 10;
  int K = 10;
  double c = 0.03;
  double delta = 0.5;

  degree_distribution deg = robust_soliton_distribution(K,c,delta);
  fountain_encoder<std::mt19937> enc(deg);

  for (int i = 0; i < K; i++) {
    packet p = random_pkt(L);
    enc.push_input(move(p));
  }

  BOOST_CHECK_NO_THROW(for (int i = 0; i <= fountain_encoder<>::MAX_SEQNO; ++i) {
      enc.next_coded();
    });

  BOOST_CHECK_THROW(enc.next_coded(), exception);
}

BOOST_AUTO_TEST_CASE(blockno_overflows) {
  int L = 10;
  int K = 2;
  double c = 0.03;
  double delta = 0.5;

  degree_distribution deg = robust_soliton_distribution(K,c,delta);
  fountain_encoder<std::mt19937> enc(deg);

  for (int i = 0; i < fountain_encoder<>::MAX_BLOCKNO; i++) {
    for (int j = 0; j < K; ++j) {
      packet p = random_pkt(L);
      enc.push_input(move(p));
    }
    BOOST_CHECK_NO_THROW(enc.discard_block());
    BOOST_CHECK_EQUAL(enc.blockno(), i+1);
  }
  BOOST_CHECK_NO_THROW(enc.discard_block());
  BOOST_CHECK_EQUAL(enc.blockno(), 0);
}

BOOST_AUTO_TEST_CASE(correct_decoding) {
  int L = 1024;
  int K = 100;
  double c = 0.03;
  double delta = 0.01;
  int N = 200;
  int nblocks = 4;

  robust_soliton_distribution deg(K,c,delta);
  fountain_encoder<std::mt19937> enc(deg);
  fountain_decoder dec(deg);

  vector<packet> original;

  for (int i = 0; i < nblocks*K; i++) {
    packet p = random_pkt(L);
    original.push_back(p);
    enc.push_input(move(p));
    if (i == 0 || i == K-2) {
      BOOST_CHECK(!enc.has_block());
      BOOST_CHECK_THROW(enc.next_coded(), exception);
    }
    else if (i == K-1) {
      BOOST_CHECK(enc.has_block());
    }
  }
  BOOST_CHECK(enc.has_block());
  BOOST_CHECK(equal(original.cbegin(), original.cbegin() + K, enc.current_block_begin()));
  BOOST_CHECK_EQUAL(enc.queue_size(), (nblocks-1)*K);

  for (int i = 0; i < nblocks; ++i) {
    fountain_packet p = enc.next_coded();
    dec.push_coded(p);
    BOOST_CHECK_EQUAL(enc.blockno(), dec.blockno());
    BOOST_CHECK_EQUAL(enc.block_seed(), dec.block_seed());
    for (int j = 1; j < N; ++j) {
      p = enc.next_coded();
      dec.push_coded(p);
      if (j < K-1) {
	BOOST_CHECK(!dec.has_decoded());
      }
    }
    BOOST_CHECK(dec.has_decoded());
    BOOST_CHECK_EQUAL(enc.blockno(), dec.blockno());
    BOOST_CHECK_EQUAL(enc.block_seed(), dec.block_seed());

    BOOST_CHECK(equal(original.cbegin() + i*K,
		      original.cbegin() + (i+1)*K,
		      dec.decoded_begin()));

    enc.discard_block();
  }
  BOOST_CHECK_EQUAL(enc.queue_size(), 0);
}

BOOST_AUTO_TEST_CASE(drop_packets) {
  int L = 1024;
  int K = 100;
  double c = 0.03;
  double delta = 0.01;
  int N = 400;
  int nblocks = 4;
  double p = 0.5;

  robust_soliton_distribution deg(K,c,delta);
  fountain_encoder<std::mt19937> enc(deg);
  fountain_decoder dec(deg);

  vector<packet> original;
  mt19937 drop_gen;
  bernoulli_distribution drop_dist(p);

  for (int i = 0; i < nblocks*K; i++) {
    packet p = random_pkt(L);
    original.push_back(p);
    enc.push_input(move(p));
  }

  for (int i = 0; i < nblocks; ++i) {
    int not_dropped = 0;
    for (int j = 0; j < N; ++j) {
      fountain_packet p = enc.next_coded();
      if (!drop_dist(drop_gen)) {
	dec.push_coded(p);
	++not_dropped;
      }
      if (not_dropped > 0 && not_dropped < K) {
	BOOST_CHECK(!dec.has_decoded());
      }
      else if (not_dropped == 0) {
	if (enc.blockno() > 0)
	  BOOST_CHECK_EQUAL(dec.blockno(), enc.blockno()-1);
	else
	  BOOST_CHECK(!dec.has_decoded());
      }
    }
    BOOST_CHECK_EQUAL(not_dropped, dec.received_count());
    BOOST_CHECK(dec.has_decoded());

    BOOST_CHECK(equal(original.cbegin() + i*K,
		      original.cbegin() + (i+1)*K,
		      dec.decoded_begin()));

    enc.discard_block();
  }
  BOOST_CHECK_EQUAL(enc.queue_size(), 0);
}

BOOST_AUTO_TEST_CASE(drop_blocks) {
  int L = 1024;
  int K = 100;
  double c = 0.03;
  double delta = 0.01;
  int N = 200;
  int nblocks = 10;
  double pblock = 0.5;

  robust_soliton_distribution deg(K,c,delta);
  fountain_encoder<std::mt19937> enc(deg);
  fountain_decoder dec(deg);

  vector<packet> original;
  mt19937 drop_gen;
  bernoulli_distribution drop_dist(pblock);

  for (int i = 0; i < nblocks*K; i++) {
    packet p = random_pkt(L);
    original.push_back(p);
    enc.push_input(move(p));
  }

  for (int i = 0; i < nblocks; ++i) {
    bool drop_block = drop_dist(drop_gen);
    for (int j = 0; j < N; ++j) {
      fountain_packet p = enc.next_coded();
      if (!drop_block) {
	dec.push_coded(p);
      }
    }
    if (!drop_block) {
      BOOST_CHECK_EQUAL(enc.blockno(), dec.blockno());
      BOOST_CHECK_EQUAL(enc.block_seed(), dec.block_seed());
      BOOST_CHECK_EQUAL(dec.blockno(), i);
      BOOST_CHECK(dec.has_decoded());

      BOOST_CHECK(equal(original.cbegin() + i*K,
			original.cbegin() + (i+1)*K,
			dec.decoded_begin()));
    }

    enc.discard_block();
  }
  BOOST_CHECK_EQUAL(enc.queue_size(), 0);
}

BOOST_AUTO_TEST_CASE(reorder_packets) {
  int L = 1024;
  int K = 100;
  double c = 0.03;
  double delta = 0.01;
  int N = 200;
  int nblocks = 4;

  robust_soliton_distribution deg(K,c,delta);
  fountain_encoder<std::mt19937> enc(deg);
  fountain_decoder dec(deg);

  vector<packet> original;
  mt19937 shuffle_eng;

  for (int i = 0; i < nblocks*K; i++) {
    packet p = random_pkt(L);
    original.push_back(p);
    enc.push_input(move(p));
  }

  for (int i = 0; i < nblocks; ++i) {
    vector<fountain_packet> coded_block;
    for (int j = 0; j < N; ++j) {
      fountain_packet p = enc.next_coded();
      coded_block.push_back(move(p));
    }
    shuffle(coded_block.begin(), coded_block.end(), shuffle_eng);
    for (size_t j = 0; j != coded_block.size(); ++j) {
      dec.push_coded(coded_block[j]);
      if (j < (size_t)K-1) {
	BOOST_CHECK(!dec.has_decoded());
      }
    }
    BOOST_CHECK_EQUAL(enc.blockno(), dec.blockno());
    BOOST_CHECK_EQUAL(enc.block_seed(), dec.block_seed());
    BOOST_CHECK_EQUAL(dec.blockno(), i);
    BOOST_CHECK(dec.has_decoded());

    BOOST_CHECK(equal(original.cbegin() + i*K,
		      original.cbegin() + (i+1)*K,
		      dec.decoded_begin()));
    enc.discard_block();
  }
  BOOST_CHECK_EQUAL(enc.queue_size(), 0);
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
  fountain_encoder<std::mt19937> enc;
  fountain_decoder dec;

  vector<packet> original;

  decoder_overflow_fixture() :
    deg(K,c,delta), enc(deg), dec(deg) {
    for (int i = 0; i < nblocks*K; i++) {
      packet p = random_pkt(L);
      original.push_back(p);
      enc.push_input(move(p));
    }

    for (int i = 0; i <= last_block_passed; ++i) {
      pass_next_block(i);
    }
  }

  void pass_next_block(int i) {
    for (int j = 0; j < N; ++j) {
      fountain_packet p = enc.next_coded();
      dec.push_coded(p);
      if (dec.has_decoded()) break;
    }
    BOOST_CHECK(dec.has_decoded());
    BOOST_CHECK(equal(original.cbegin() + i*K,
		      original.cbegin() + (i+1)*K,
		      dec.decoded_begin()));
    enc.discard_block();
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
    enc.discard_block();
  BOOST_CHECK_EQUAL(enc.blockno(), 10);
  pass_next_block(0xffff + 11);
  BOOST_CHECK_EQUAL(dec.blockno(), 10);
}

BOOST_FIXTURE_TEST_CASE(decoder_ignore_jump_back, decoder_overflow_fixture) {
  vector<fountain_packet> old_block;
  int old_blockno = last_block_passed+1;
  for (int i = 0; i < N; ++i) {
    old_block.push_back(enc.next_coded());
  }
  enc.discard_block();

  pass_next_block(last_block_passed+2);
  pass_next_block(last_block_passed+3);

  for (auto i = old_block.cbegin(); i != old_block.cend(); ++i) {
    dec.push_coded(*i);
  }
  BOOST_CHECK_EQUAL(dec.blockno(), last_block_passed+3);

  for (int i = last_block_passed+4; i <= 0xffff + 3; ++i) {
    pass_next_block(i);
  }

  BOOST_CHECK(old_blockno > dec.blockno());
  for (auto i = old_block.cbegin(); i != old_block.cend(); ++i) {
    dec.push_coded(*i);
  }
  BOOST_CHECK_EQUAL(dec.blockno(), (0xffff+3) & 0xffff);
}

struct stat_fixture {
  boost::shared_ptr<default_stat_sink> the_stat_sink;

  stat_fixture() {
    boost::shared_ptr<std::ofstream>
      ofs(new std::ofstream("stat_log.json", std::ios_base::trunc));
    the_stat_sink = make_stat_sink(ofs);
    boost::log::core::get()->add_sink(the_stat_sink);
  }

  ~stat_fixture() {
    boost::log::core::get()->remove_sink(the_stat_sink);
  }
};
