#define BOOST_TEST_MODULE test_uep_encdec
#include <boost/test/unit_test.hpp>

#include "uep_decoder.hpp"
#include "uep_encoder.hpp"

#include <climits>
#include <map>
#include <fstream>

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

packet random_pkt(int size) {
  static std::independent_bits_engine<std::mt19937, CHAR_BIT, unsigned char> g;
  packet p;
  p.resize(size);
  for (int i=0; i < size; i++) {
    p[i] = g();
  }
  return p;
}

BOOST_AUTO_TEST_CASE(check_encoder_counters) {
  size_t L = 10;
  size_t K_uep = 6;
  size_t K = 16;
  lt_uep_parameter_set ps;
  ps.Ks = {2, 4};
  ps.RFs = {2, 1};
  ps.EF = 2;
  ps.c = 0.1;
  ps.delta = 0.5;

  uep_encoder<std::mt19937> enc(ps);
  BOOST_CHECK_EQUAL(enc.block_size_out(), K);
  BOOST_CHECK_EQUAL(enc.block_size_in(), K_uep);
  BOOST_CHECK(!enc.has_block());

  vector<fountain_packet> original;
  for (size_t i = 0; i < 10; ++i) {
    for (size_t j = 0; j < ps.Ks[0]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(0);
      original.push_back(p);
      enc.push(std::move(p));
    }
    for (size_t j = 0; j < ps.Ks[1]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(1);
      original.push_back(p);
      enc.push(std::move(p));
    }
  }
  BOOST_CHECK_EQUAL(original.size(), 10*K_uep);
  BOOST_CHECK(enc.has_block());
  BOOST_CHECK_EQUAL(enc.queue_size(), 9*K_uep);
  BOOST_CHECK_EQUAL(enc.size(), 10*K_uep);
  BOOST_CHECK_EQUAL(enc.blockno(), 0);

  for (size_t i = 0; i < 2*K; ++i) {
    fountain_packet fp = enc.next_coded();
    BOOST_CHECK_EQUAL(fp.size(), L+4);
    BOOST_CHECK_EQUAL(fp.sequence_number(), i);
    BOOST_CHECK_EQUAL(fp.block_number(), 0);
    BOOST_CHECK_EQUAL(fp.block_seed(), enc.block_seed());
  }
  BOOST_CHECK_EQUAL(enc.seqno(), 2*K-1);

  enc.next_block();

  BOOST_CHECK_EQUAL(enc.queue_size(), 8*K_uep);
  BOOST_CHECK_EQUAL(enc.size(), 9*K_uep);
  BOOST_CHECK_EQUAL(enc.blockno(), 1);

  fountain_packet fp = enc.next_coded();
  BOOST_CHECK_EQUAL(enc.seqno(), 0);
  BOOST_CHECK_EQUAL(fp.size(), L+4);
  BOOST_CHECK_EQUAL(fp.sequence_number(), 0);
  BOOST_CHECK_EQUAL(fp.block_number(), 1);
  BOOST_CHECK_EQUAL(fp.block_seed(), enc.block_seed());
};

BOOST_AUTO_TEST_CASE(correct_decoding) {
  size_t L = 1500;
  size_t K_uep = 100;
  size_t K = 250;
  lt_uep_parameter_set ps;
  ps.Ks = {25, 75};
  ps.RFs = {2, 1};
  ps.EF = 2;
  ps.c = 0.1;
  ps.delta = 0.5;

  size_t nblocks = 1;

  uep_encoder<std::mt19937> enc(ps);
  uep_decoder dec(ps);

  vector<fountain_packet> original;
  for (size_t i = 0; i < nblocks; ++i) {
    for (size_t j = 0; j < ps.Ks[0]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(0);
      original.push_back(p);
      enc.push(std::move(p));
    }
    for (size_t j = 0; j < ps.Ks[1]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(1);
      original.push_back(p);
      enc.push(std::move(p));
    }
  }

  BOOST_CHECK_EQUAL(enc.size(), K_uep);
  BOOST_CHECK(enc.has_block());

  BOOST_CHECK(!dec.has_decoded());
  while (!dec.has_decoded()) {
    fountain_packet p = enc.next_coded();
    dec.push(std::move(p));
  }
  BOOST_CHECK(dec.has_decoded());

  BOOST_CHECK_EQUAL(dec.queue_size(), K_uep);

  for (auto i = original.cbegin(); i != original.cend(); ++i) {
    fountain_packet out = dec.next_decoded();
    BOOST_CHECK(i->buffer() == out.buffer());
    BOOST_CHECK_EQUAL(i->getPriority(), out.getPriority());
  }
}

BOOST_AUTO_TEST_CASE(multiple_blocks) {
  size_t L = 1500;
  size_t K_uep = 100;
  size_t K = 250;
  lt_uep_parameter_set ps;
  ps.Ks = {25, 75};
  ps.RFs = {2, 1};
  ps.EF = 2;
  ps.c = 0.1;
  ps.delta = 0.5;

  size_t nblocks = 50;

  uep_encoder<std::mt19937> enc(ps);
  uep_decoder dec(ps);

  vector<fountain_packet> original;
  for (size_t i = 0; i < nblocks; ++i) {
    for (size_t j = 0; j < ps.Ks[0]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(0);
      original.push_back(p);
      enc.push(std::move(p));
    }
    for (size_t j = 0; j < ps.Ks[1]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(1);
      original.push_back(p);
      enc.push(std::move(p));
    }
  }

  int i = 0;
  while (enc.has_block()) {
    do {
      fountain_packet p = enc.next_coded();
      dec.push(std::move(p));
    } while (!dec.has_decoded());
    ++i;
    BOOST_CHECK_EQUAL(dec.queue_size(), i*K_uep);
    enc.next_block();
  }

  BOOST_CHECK_EQUAL(dec.queue_size(), 50*K_uep);

  for (auto i = original.cbegin(); i != original.cend(); ++i) {
    fountain_packet out = dec.next_decoded();
    BOOST_CHECK(i->buffer() == out.buffer());
    BOOST_CHECK_EQUAL(i->getPriority(), out.getPriority());
  }
}

BOOST_AUTO_TEST_CASE(drop_packets) {
  size_t L = 1500;
  size_t K_uep = 100;
  size_t K = 250;
  lt_uep_parameter_set ps;
  ps.Ks = {25, 75};
  ps.RFs = {2, 1};
  ps.EF = 2;
  ps.c = 0.1;
  ps.delta = 0.5;

  size_t nblocks = 1;

  uep_encoder<std::mt19937> enc(ps);
  uep_decoder dec(ps);

  vector<fountain_packet> original;
  for (size_t i = 0; i < nblocks; ++i) {
    for (size_t j = 0; j < ps.Ks[0]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(0);
      original.push_back(p);
      enc.push(std::move(p));
    }
    for (size_t j = 0; j < ps.Ks[1]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(1);
      original.push_back(p);
      enc.push(std::move(p));
    }
  }

  double p = 0.9;
  mt19937 drop_gen;
  bernoulli_distribution drop_dist(p);

  do {
    fountain_packet p = enc.next_coded();
    if (!drop_dist(drop_gen))
      dec.push(p);
  } while (!dec.has_decoded());

  for (auto i = original.cbegin(); i != original.cend(); ++i) {
    fountain_packet out = dec.next_decoded();
    BOOST_CHECK(i->buffer() == out.buffer());
    BOOST_CHECK_EQUAL(i->getPriority(), out.getPriority());
  }
}

BOOST_AUTO_TEST_CASE(drop_blocks) {
  size_t L = 4;
  size_t K_uep = 6;
  size_t K = 16;
  lt_uep_parameter_set ps;
  ps.Ks = {2, 4};
  ps.RFs = {2, 1};
  ps.EF = 2;
  ps.c = 0.1;
  ps.delta = 0.5;

  size_t nblocks = 100;

  uep_encoder<std::mt19937> enc(ps);
  uep_decoder dec(ps);

  vector<fountain_packet> original;
  for (size_t i = 0; i < nblocks; ++i) {
    for (size_t j = 0; j < ps.Ks[0]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(0);
      original.push_back(p);
      enc.push(p);
    }
    for (size_t j = 0; j < ps.Ks[1]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(1);
      original.push_back(p);
      enc.push(p);
    }
  }

  double p = 0.6;
  mt19937 drop_gen;
  bernoulli_distribution drop_dist(p);

  std::vector<bool> blockmask(nblocks, false);
  while (enc.has_block()) {
    if (!drop_dist(drop_gen)) {
      do {
	fountain_packet p = enc.next_coded();
	dec.push(std::move(p));
      } while (!dec.has_decoded());
      blockmask[dec.blockno()] = true;
    }
    enc.next_block();
  }

  size_t j = 0;
  size_t blockno = 0;
  while (dec.has_queued_packets()) {
    fountain_packet out = dec.next_decoded();

    if (out.empty()) {
      BOOST_CHECK(!blockmask[blockno]);
    }
    else {
      BOOST_CHECK(blockmask[blockno]);
      BOOST_CHECK(original[j].buffer() == out.buffer());
      BOOST_CHECK_EQUAL(original[j].getPriority(), out.getPriority());
    }

    ++j;
    if (j % K_uep == 0) ++blockno;
  }
  BOOST_CHECK_EQUAL(blockno, dec.blockno()+1);
}

BOOST_AUTO_TEST_CASE(reorder_drop_packets) {
  size_t L = 4;
  size_t K_uep = 500;
  size_t K = 1400;
  lt_uep_parameter_set ps;
  ps.Ks = {200, 300};
  ps.RFs = {2, 1};
  ps.EF = 2;
  ps.c = 0.1;
  ps.delta = 0.5;

  size_t nblocks = 1;

  uep_encoder<std::mt19937> enc(ps);
  uep_decoder dec(ps);

  vector<fountain_packet> original;
  for (size_t i = 0; i < nblocks; ++i) {
    for (size_t j = 0; j < ps.Ks[0]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(0);
      original.push_back(p);
      enc.push(p);
    }
    for (size_t j = 0; j < ps.Ks[1]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(1);
      original.push_back(p);
      enc.push(p);
    }
  }

  vector<fountain_packet> coded;
  for (size_t i = 0; i < 3*K; ++i)
    coded.push_back(enc.next_coded());

  shuffle(coded.begin(), coded.end(), std::mt19937());

  for (auto i = coded.cbegin(); i != coded.cend(); ++i) {
    dec.push(*i);
  }

   BOOST_CHECK(dec.has_decoded());

   for (auto i = original.cbegin(); i != original.cend(); ++i) {
     fountain_packet out = dec.next_decoded();
     BOOST_CHECK(i->buffer() == out.buffer());
     BOOST_CHECK_EQUAL(i->getPriority(), out.getPriority());
   }
}

struct decoder_overflow_fixture {
  size_t L = 10;
  size_t K_uep = 2;
  size_t K = 6;
  lt_uep_parameter_set ps;

  size_t nblocks = 0xffff * 2;
  size_t last_block_passed = 0xffff - 50;

  uep_encoder<std::mt19937> enc;
  uep_decoder dec;

  vector<packet> original;

  decoder_overflow_fixture() :
    ps{0,0,2,{1,1},{2,1},0.03,0.01},
    enc(ps), dec(ps) {
      vector<fountain_packet> original;
      for (size_t i = 0; i < nblocks; ++i) {
	for (size_t j = 0; j < ps.Ks[0]; ++j) {
	  fountain_packet p(random_pkt(L));
	  p.setPriority(0);
	  original.push_back(p);
	  enc.push(p);
	}
	for (size_t j = 0; j < ps.Ks[1]; ++j) {
	  fountain_packet p(random_pkt(L));
	  p.setPriority(1);
	  original.push_back(p);
	  enc.push(p);
	}
      }

      for (int i = 0; i <= last_block_passed; ++i) {
	do {
	  fountain_packet p = enc.next_coded();
	  dec.push(move(p));
	} while (!dec.has_decoded());
	enc.next_block();
      }
  }
};

BOOST_FIXTURE_TEST_CASE(decoder_overflow, decoder_overflow_fixture) {
  for (++last_block_passed;
       last_block_passed <= 0xffff;
       ++last_block_passed) {
    do {
      fountain_packet p = enc.next_coded();
      dec.push(move(p));
    } while (!dec.has_decoded());
    enc.next_block();
  }
  BOOST_CHECK_EQUAL(enc.blockno(), 0);
  BOOST_CHECK_EQUAL(dec.blockno(), 0xffff);
  do {
    fountain_packet p = enc.next_coded();
    dec.push(move(p));
  } while (!dec.has_decoded());
  enc.next_block();
  BOOST_CHECK_EQUAL(dec.blockno(), 0);
  do {
    fountain_packet p = enc.next_coded();
    dec.push(move(p));
  } while (!dec.has_decoded());
  enc.next_block();
  BOOST_CHECK_EQUAL(dec.blockno(), 1);
}

BOOST_FIXTURE_TEST_CASE(decoder_overflow_jump, decoder_overflow_fixture) {
  for (size_t i = 0; i < 0xffff - last_block_passed + 10; ++i)
    enc.next_block();
  BOOST_CHECK_EQUAL(enc.blockno(), 10);
  do {
    fountain_packet p = enc.next_coded();
    dec.push(move(p));
  } while (!dec.has_decoded());
  enc.next_block();
  BOOST_CHECK_EQUAL(dec.blockno(), 10);
}

BOOST_FIXTURE_TEST_CASE(decoder_ignore_jump_back, decoder_overflow_fixture) {
  vector<fountain_packet> old_block;
  size_t old_blockno = last_block_passed+1;
  for (int i = 0; i < 50*K; ++i) {
    old_block.push_back(enc.next_coded());
  }
  enc.next_block();

  for (int n = 0; n < 2; ++n) {
    do {
      fountain_packet p = enc.next_coded();
      dec.push(move(p));
    } while (!dec.has_decoded());
    enc.next_block();
  }

  for (auto i = old_block.cbegin(); i != old_block.cend(); ++i) {
    dec.push(*i);
  }
  BOOST_CHECK_EQUAL(dec.blockno(), last_block_passed+3);

  for (int i = last_block_passed+4; i <= 0xffff + 3; ++i) {
    do {
      fountain_packet p = enc.next_coded();
      dec.push(move(p));
    } while (!dec.has_decoded());
    enc.next_block();
  }

  BOOST_CHECK(old_blockno > dec.blockno());
  for (auto i = old_block.cbegin(); i != old_block.cend(); ++i) {
    dec.push(*i);
  }
  BOOST_CHECK_EQUAL(dec.blockno(), (0xffff+3) & 0xffff);
}

BOOST_AUTO_TEST_CASE(skip_multiple_blocks) {
  size_t L = 10;
  size_t K_uep = 100;
  //size_t K = 250;
  lt_uep_parameter_set ps;
  ps.Ks = {25, 75};
  ps.RFs = {2, 1};
  ps.EF = 2;
  ps.c = 0.1;
  ps.delta = 0.5;

  size_t nblocks = 30;

  uep_encoder<std::mt19937> enc(ps);
  uep_decoder dec(ps);

  vector<fountain_packet> original;
  for (size_t i = 0; i < nblocks; ++i) {
    for (size_t j = 0; j < ps.Ks[0]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(0);
      original.push_back(p);
      enc.push(std::move(p));
    }
    for (size_t j = 0; j < ps.Ks[1]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(1);
      original.push_back(p);
      enc.push(std::move(p));
    }
  }

  BOOST_CHECK_EQUAL(enc.blockno(), 0);
  BOOST_CHECK_EQUAL(enc.size(), 30*K_uep);
  enc.next_block(0);
  BOOST_CHECK_EQUAL(enc.blockno(), 0);
  BOOST_CHECK_EQUAL(enc.size(), 30*K_uep);

  enc.next_block(0xff00);
  BOOST_CHECK_EQUAL(enc.blockno(), 0);
  BOOST_CHECK_EQUAL(enc.size(), 30*K_uep);

  enc.next_block(1);
  BOOST_CHECK_EQUAL(enc.blockno(), 1);
  BOOST_CHECK_EQUAL(enc.size(), 29*K_uep);

  enc.next_block(20);
  BOOST_CHECK_EQUAL(enc.blockno(), 20);
  BOOST_CHECK_EQUAL(enc.size(), 10*K_uep);

  enc.next_block(0);
  BOOST_CHECK_EQUAL(enc.blockno(), 20);
  BOOST_CHECK_EQUAL(enc.size(), 10*K_uep);

  enc.next_block(29);
  BOOST_CHECK_EQUAL(enc.blockno(), 29);
  BOOST_CHECK_EQUAL(enc.size(), K_uep);
  // BOOST_CHECK(std::equal(enc.current_block_begin(),
  //			 enc.current_block_end(),
  //			 original.cbegin() + 29*K_uep,
  //			 [](const packet &lhs, const packet &rhs){
  //			   return lhs.buffer() == rhs.buffer();
  //			 }));

  BOOST_CHECK_NO_THROW(enc.next_block());
  BOOST_CHECK_EQUAL(enc.size(), 0);
  BOOST_CHECK_THROW(enc.next_block(), std::logic_error);
  BOOST_CHECK_THROW(enc.next_block(100), std::logic_error);
}

BOOST_AUTO_TEST_CASE(missing_decoded) {
  size_t L = 10;
  size_t K_uep = 100;
  //size_t K = 250;
  lt_uep_parameter_set ps;
  ps.Ks = {25, 75};
  ps.RFs = {2, 1};
  ps.EF = 2;
  ps.c = 0.1;
  ps.delta = 0.5;

  size_t nblocks = 30;

  uep_encoder<std::mt19937> enc(ps);
  uep_decoder dec(ps);

  vector<fountain_packet> original;
  for (size_t i = 0; i < nblocks; ++i) {
    for (size_t j = 0; j < ps.Ks[0]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(0);
      original.push_back(p);
      enc.push(std::move(p));
    }
    for (size_t j = 0; j < ps.Ks[1]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(1);
      original.push_back(p);
      enc.push(std::move(p));
    }
  }

  // Skip first block
  enc.next_block();
  fountain_packet fp = enc.next_coded();
  dec.push(fp.shallow_copy());
  BOOST_CHECK_EQUAL(dec.received_count(), 1);
  BOOST_CHECK_EQUAL(dec.queue_size(), K_uep);

  // Finish second block
  while (!dec.has_decoded()) {
    fp = enc.next_coded();
    dec.push(move(fp));
  }
  BOOST_CHECK_EQUAL(dec.queue_size(), 2*K_uep);

  // Skip to last block
  enc.next_block(nblocks-1);
  fp = enc.next_coded();
  dec.push(fp.shallow_copy());
  while (!dec.has_decoded()) {
    fp = enc.next_coded();
    dec.push(move(fp));
  }
  BOOST_CHECK_EQUAL(dec.queue_size(), nblocks*K_uep);

  // Check correctness
  auto i = original.cbegin();
  for (size_t l = 0; l < K_uep; ++l) { // block 0
    fountain_packet p = dec.next_decoded();
    BOOST_CHECK(!p);
    ++i;
  }
  for (size_t l = 0; l < K_uep; ++l) { // block 1
    fountain_packet p = dec.next_decoded();
    BOOST_CHECK(p.buffer() == i->buffer());
    BOOST_CHECK(p.getPriority() == i->getPriority());
    ++i;
  }
  for (size_t l = 0; l < (nblocks - 3)*K_uep; ++l) { // blocks 2-28
    fountain_packet p = dec.next_decoded();
    BOOST_CHECK(!p);
    ++i;
  }
  for (size_t l = 0; l < K_uep; ++l) { // block 29
    fountain_packet p = dec.next_decoded();
    BOOST_CHECK(p.buffer() == i->buffer());
    BOOST_CHECK(p.getPriority() == i->getPriority());
    ++i;
  }
  BOOST_CHECK(i == original.cend());
}

BOOST_AUTO_TEST_CASE(check_decoder_counters) {
  size_t L = 10;
  size_t K_uep = 100;
  size_t K = 250;
  lt_uep_parameter_set ps;
  ps.Ks = {25, 75};
  ps.RFs = {2, 1};
  ps.EF = 2;
  ps.c = 0.1;
  ps.delta = 0.5;

  size_t nblocks = 4;

  uep_encoder<std::mt19937> enc(ps);
  uep_decoder dec(ps);

  vector<fountain_packet> original;
  for (size_t i = 0; i < nblocks; ++i) {
    for (size_t j = 0; j < ps.Ks[0]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(0);
      original.push_back(p);
      enc.push(std::move(p));
    }
    for (size_t j = 0; j < ps.Ks[1]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(1);
      original.push_back(p);
      enc.push(std::move(p));
    }
  }

  // Initial state
  BOOST_CHECK_EQUAL(dec.total_received_count(), 0);
  BOOST_CHECK_EQUAL(dec.total_decoded_count(), 0);
  BOOST_CHECK_EQUAL(dec.total_failed_count(), 0);

  // Current block does not increment counters until complete
  for (size_t i = 0; i < K-1; ++i)
    dec.push(enc.next_coded());
  BOOST_CHECK_EQUAL(dec.received_count(), K-1);
  //BOOST_CHECK(dec.decoded_count() > 0);
  BOOST_CHECK_EQUAL(dec.total_received_count(), K-1);
  BOOST_CHECK_EQUAL(dec.total_decoded_count(), 0);
  BOOST_CHECK_EQUAL(dec.total_failed_count(), 0);

  // Finish first block
  while (!dec.has_decoded())
    dec.push(enc.next_coded());
  enc.next_block();
  //BOOST_CHECK_EQUAL(dec.decoded_count(), K);
  BOOST_CHECK_GE(dec.total_received_count(), K);
  BOOST_CHECK_EQUAL(dec.total_decoded_count(), K_uep);
  BOOST_CHECK_EQUAL(dec.total_failed_count(), 0);

  // Skip two blocks
  enc.next_block(3);
  dec.push(enc.next_coded());
  while (!dec.has_decoded())
    dec.push(enc.next_coded());
  enc.next_block();

  // Check counters
  BOOST_CHECK_EQUAL(dec.total_received_count(), enc.total_coded_count());
  //BOOST_CHECK_EQUAL(dec.decoded_count(), K);
  BOOST_CHECK_EQUAL(dec.total_decoded_count(), 2*K_uep); // the current
						     // block is
						     // enqueued when
						     // decoded
  BOOST_CHECK_EQUAL(dec.total_failed_count(), (nblocks-2)*K_uep);
  BOOST_CHECK_EQUAL(dec.total_decoded_count() +
		    dec.total_failed_count(),
		    nblocks * K_uep);
}

BOOST_AUTO_TEST_CASE(check_decoder_flush) {
  size_t L = 10;
  size_t K_uep = 10;
  //size_t K = 26;
  lt_uep_parameter_set ps;
  ps.Ks = {3, 7};
  ps.RFs = {2, 1};
  ps.EF = 2;
  ps.c = 0.1;
  ps.delta = 0.5;

  size_t nblocks = 70000;

  uep_encoder<std::mt19937> enc(ps);
  uep_decoder dec(ps);

  vector<fountain_packet> original;
  for (size_t i = 0; i < nblocks; ++i) {
    for (size_t j = 0; j < ps.Ks[0]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(0);
      original.push_back(p);
      enc.push(std::move(p));
    }
    for (size_t j = 0; j < ps.Ks[1]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(1);
      original.push_back(p);
      enc.push(std::move(p));
    }
  }

  BOOST_CHECK_EQUAL(dec.total_failed_count(), 0);
  BOOST_CHECK_EQUAL(dec.total_decoded_count(), 0);
  BOOST_CHECK_EQUAL(dec.blockno(), 0);

  // Skip one block
  dec.flush();
  BOOST_CHECK_EQUAL(dec.total_failed_count(), K_uep);
  BOOST_CHECK_EQUAL(dec.total_decoded_count(), 0);
  BOOST_CHECK_EQUAL(dec.blockno(), 1);

  // Skip to block 50
  dec.flush(50);
  BOOST_CHECK_EQUAL(dec.total_failed_count(), 50*K_uep);
  BOOST_CHECK_EQUAL(dec.total_decoded_count(), 0);
  BOOST_CHECK_EQUAL(dec.blockno(), 50);

  // Decode block 50
  enc.next_block(50);
  while (!dec.has_decoded())
    dec.push(enc.next_coded());

  BOOST_CHECK_EQUAL(dec.total_failed_count(), 50*K_uep);
  BOOST_CHECK_EQUAL(dec.total_decoded_count(), K_uep);
  BOOST_CHECK_EQUAL(dec.blockno(), 50);

  // Partial block 51
  size_t before = dec.total_decoded_count();
  enc.next_block();
  dec.push(enc.next_coded());
  for (size_t i = 0; i < 3*K_uep/2; ++i) {
    dec.push(enc.next_coded());
  }
  dec.flush();
  size_t partial = dec.total_decoded_count() - before;

  BOOST_CHECK_EQUAL(dec.total_failed_count(), 51*K_uep - partial);
  BOOST_CHECK_EQUAL(dec.total_decoded_count(), K_uep + partial);
  BOOST_CHECK_EQUAL(dec.blockno(), 52);

  // Skip to block 50 again (+ 2^16 blocks)
  dec.flush(50);

  // failed 0--49, partially failed 51, skip 52--2^16-1, skip 0--49
  size_t failed_pkts = 51*K_uep -partial + (static_cast<size_t>(pow(2,16))-2)*K_uep;
  size_t good_pkts = K_uep + partial;
  BOOST_CHECK_EQUAL(dec.total_failed_count(), failed_pkts);
  BOOST_CHECK_EQUAL(dec.total_decoded_count(), good_pkts);
  BOOST_CHECK_EQUAL(dec.blockno(), 50);
}

BOOST_AUTO_TEST_CASE(check_decoder_flush_n) {
  size_t L = 10;
  size_t K_uep = 10;
  //size_t K = 26;
  lt_uep_parameter_set ps;
  ps.Ks = {3, 7};
  ps.RFs = {2, 1};
  ps.EF = 2;
  ps.c = 0.1;
  ps.delta = 0.5;

  size_t nblocks = 70000;

  uep_encoder<std::mt19937> enc(ps);
  uep_decoder dec(ps);

  vector<fountain_packet> original;
  for (size_t i = 0; i < nblocks; ++i) {
    for (size_t j = 0; j < ps.Ks[0]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(0);
      original.push_back(p);
      enc.push(std::move(p));
    }
    for (size_t j = 0; j < ps.Ks[1]; ++j) {
      fountain_packet p(random_pkt(L));
      p.setPriority(1);
      original.push_back(p);
      enc.push(std::move(p));
    }
  }

  BOOST_CHECK_EQUAL(dec.total_failed_count(), 0);
  BOOST_CHECK_EQUAL(dec.total_decoded_count(), 0);
  BOOST_CHECK_EQUAL(dec.blockno(), 0);

  // Skip one block
  dec.flush();
  BOOST_CHECK_EQUAL(dec.total_failed_count(), K_uep);
  BOOST_CHECK_EQUAL(dec.total_decoded_count(), 0);
  BOOST_CHECK_EQUAL(dec.blockno(), 1);

  // Skip to block 50
  dec.flush(50);
  BOOST_CHECK_EQUAL(dec.total_failed_count(), 50*K_uep);
  BOOST_CHECK_EQUAL(dec.total_decoded_count(), 0);
  BOOST_CHECK_EQUAL(dec.blockno(), 50);

  // Decode block 50
  enc.next_block(50);
  while (!dec.has_decoded())
    dec.push(enc.next_coded());

  BOOST_CHECK_EQUAL(dec.total_failed_count(), 50*K_uep);
  BOOST_CHECK_EQUAL(dec.total_decoded_count(), K_uep);
  BOOST_CHECK_EQUAL(dec.blockno(), 50);

  // Partial block 51
  size_t before = dec.total_decoded_count();
  enc.next_block();
  for (size_t i = 0; i < 3*K_uep/2; ++i) {
    dec.push(enc.next_coded());
  }
  dec.flush();
  size_t partial = dec.total_decoded_count() - before;

  // Skip to end
  dec.flush_n_blocks(nblocks - 52);

  size_t failed_pkts = nblocks*K_uep - K_uep - partial;
  size_t good_pkts = K_uep + partial;
  BOOST_CHECK_EQUAL(dec.total_failed_count(), failed_pkts);
  BOOST_CHECK_EQUAL(dec.total_decoded_count(), good_pkts);
  BOOST_CHECK_EQUAL(dec.blockno(), nblocks % static_cast<size_t>(pow(2,16)));
}

BOOST_AUTO_TEST_CASE(uep_random_order) {
  size_t L = 10;
  size_t K_uep = 100;
  //size_t K = 26;
  lt_uep_parameter_set ps;
  ps.Ks = {25, 75};
  ps.RFs = {2, 1};
  ps.EF = 2;
  ps.c = 0.1;
  ps.delta = 0.5;

  size_t nblocks = 50;

  uep_encoder<std::mt19937> enc(ps);
  uep_decoder dec(ps);

  vector<fountain_packet> original;
  for (size_t i = 0; i < nblocks*ps.Ks[0]; ++i) {
    fountain_packet p(random_pkt(L));
    p.setPriority(0);
    original.push_back(p);
  }
  for (size_t i = 0; i < nblocks*ps.Ks[1]; ++i) {
    fountain_packet p(random_pkt(L));
    p.setPriority(1);
    original.push_back(p);
  }
  BOOST_CHECK_EQUAL(original.size(), nblocks*K_uep);

  shuffle(original.begin(), original.end(), mt19937());

  for (const fountain_packet &fp : original) {
    enc.push(fp);
  }

  while (enc.has_block()) {
    do {
      dec.push(enc.next_coded());
    } while (!dec.has_decoded());
    enc.next_block();
  }

  BOOST_CHECK_EQUAL(dec.queue_size(), nblocks*K_uep);
  auto i = original.cbegin();
  while (dec.has_queued_packets()) {
    fountain_packet fp = dec.next_decoded();
    BOOST_CHECK(fp.buffer() == i->buffer());
    BOOST_CHECK(fp.getPriority() == i->getPriority());
    ++i;
  }
}

BOOST_AUTO_TEST_CASE(uep_random_order_drop) {
  size_t L = 10;
  size_t K_uep = 100;
  size_t K = 250;
  lt_uep_parameter_set ps;
  ps.Ks = {25, 75};
  ps.RFs = {2, 1};
  ps.EF = 2;
  ps.c = 0.1;
  ps.delta = 0.5;

  size_t nblocks = 50;

  uep_encoder<std::mt19937> enc(ps);
  uep_decoder dec(ps);

  vector<fountain_packet> original;
  for (size_t i = 0; i < nblocks*ps.Ks[0]; ++i) {
    fountain_packet p(random_pkt(L));
    p.setPriority(0);
    original.push_back(p);
  }
  for (size_t i = 0; i < nblocks*ps.Ks[1]; ++i) {
    fountain_packet p(random_pkt(L));
    p.setPriority(1);
    original.push_back(p);
  }
  BOOST_CHECK_EQUAL(original.size(), nblocks*K_uep);

  shuffle(original.begin(), original.end(), mt19937());

  for (const fountain_packet &fp : original) {
    enc.push(fp);
  }

  std::size_t max_num = 1.3 * K;

  while (enc.has_block()) {
    do {
      dec.push(enc.next_coded());
      if (dec.received_count() >= max_num) break;
    } while (!dec.has_decoded());
    enc.next_block();
  }

  BOOST_CHECK_EQUAL(dec.queue_size(), nblocks*K_uep);
  auto i = original.cbegin();
  while (dec.has_queued_packets()) {
    fountain_packet fp = dec.next_decoded();
    if (!fp.buffer().empty()) {
      BOOST_CHECK(fp.buffer() == i->buffer());
      BOOST_CHECK(fp.getPriority() == i->getPriority());
    }
    ++i;
  }
}

BOOST_AUTO_TEST_CASE(uep_padding) {
  size_t L = 10;
  size_t K_uep = 100;
  size_t K = 250;
  auto Ks = {25, 75};
  auto RFs = {2, 1};
  size_t EF = 2;
  double c = 0.1;
  double delta = 0.5;

  size_t nblocks = 50;

  uep_encoder<std::mt19937> enc(Ks.begin(), Ks.end(),
				RFs.begin(), RFs.end(),
				EF, c, delta);
  uep_decoder dec(Ks.begin(), Ks.end(),
		  RFs.begin(), RFs.end(),
		  EF, c, delta);

  vector<fountain_packet> original;
  for (size_t i = 0; i < 4; ++i) {
    fountain_packet p(random_pkt(L));
    p.setPriority(0);
    original.push_back(p);
  }
  for (size_t i = 0; i < 4; ++i) {
    fountain_packet p(random_pkt(L));
    p.setPriority(1);
    original.push_back(p);
  }
  BOOST_CHECK_EQUAL(original.size(), 8);

  for (const fountain_packet &fp : original) {
    enc.push(fp);
  }

  BOOST_CHECK(!enc.has_block());
  enc.pad_partial_block();
  BOOST_CHECK(enc.has_block());

  while (!dec.has_decoded()) {
    dec.push(enc.next_coded());
  }

  BOOST_CHECK_EQUAL(dec.queue_size(), 8);
  BOOST_CHECK_EQUAL(dec.total_decoded_count(), 8);
  BOOST_CHECK_EQUAL(dec.total_failed_count(), 8);
  auto i = original.cbegin();
  while (dec.has_queued_packets()) {
    fountain_packet fp = dec.next_decoded();
    BOOST_CHECK(fp.buffer() == i->buffer());
    BOOST_CHECK(fp.getPriority() == i->getPriority());
    ++i;
  }
}
