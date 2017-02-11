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
  fountain_encoder<> enc(deg);

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
  fountain_encoder<> enc(deg);

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

struct stat_fixture {
  boost::shared_ptr<default_stat_sink> the_stat_sink;
  
  stat_fixture() {
    boost::shared_ptr<std::ofstream> ofs(new std::ofstream("test_log.json", std::ios_base::trunc));
    the_stat_sink = make_stat_sink(ofs);
    boost::log::core::get()->add_sink(the_stat_sink);
  }

  ~stat_fixture() {
    boost::log::core::get()->remove_sink(the_stat_sink);
  }
};

BOOST_FIXTURE_TEST_CASE(N_histo, stat_fixture) {
  default_logger logger;

  int L = 100;
  int K = 10000;
  double c = 0.03;
  double delta = 0.5;
  int N = 12000;
  int npkts = 10000;
  double p = 0;

  degree_distribution deg = robust_soliton_distribution(K,c,delta);
  fountain_encoder<> enc(deg);
  fountain_decoder dec(deg);

  vector<packet> original;
  map<int, int> required_N_histogram;

  for (int i = 0; i < npkts; i++) {
    packet p = random_pkt(L);
    original.push_back(p);
    enc.push_input(fountain_packet(move(p)));
  }
  // Encoder generato, npkts pacchetti lunghi L generati a random

  // Init the histogram with zeros
  for (int i = K; i <= N; ++i) {
    required_N_histogram.insert(make_pair(i, 0));
  }

  int i = 0;
  for(;;) {
    if (!enc.has_block()) { // se ha almeno k pacchetti in coda
      BOOST_LOG_SEV(logger,info) << "Out of blocks";
      break;
    }
    // codifica: prende un pacchetto lungo L, genera la riga della matrice per codificare, fa lo xor e lo mette in c (fountain_packet)

    
    fountain_packet c = enc.next_coded();
    if (rand() > p) dec.push_coded(move(c));

    ++i;
    //if (i % 100 == 0) cout << "Packet num. " << i << endl;

    
    if (dec.has_decoded()) {
      bool equal = std::equal(dec.decoded_begin(), dec.decoded_end(),
			      original.begin() + dec.blockno() * K);
      BOOST_CHECK(equal);
      
      required_N_histogram[enc.seqno()]++;
      enc.discard_block();
    }
    if (c.sequence_number() == N-1) {
      enc.discard_block();
    }
  }

  int histo_sum = accumulate(required_N_histogram.cbegin(),
			     required_N_histogram.cend(),
			     0,
			     [](int s, const std::pair<int,int> &p) -> int {
			       return s + p.second;
			     });

  BOOST_CHECK_EQUAL(histo_sum, npkts/K);
  
  histogram_print_csv(required_N_histogram, "required_N_histogram.csv");
}
