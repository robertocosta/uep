#define BOOST_TEST_MODULE encdec_test
#include <boost/test/unit_test.hpp>

#include "encoder.hpp"
#include "decoder.hpp"
#include "log.hpp"

#include <climits>
#include <map>

using namespace std;

packet random_pkt(int size) {
  static std::independent_bits_engine<std::mt19937, CHAR_BIT, unsigned char> g;
  packet p;
  p.resize(size);
  for (int i=0; i < size; i++) {
    p[i] = g();
  }
  return p;
}

struct test_ctx {

};

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level);

BOOST_FIXTURE_TEST_SUITE(encdec, test_ctx)

BOOST_AUTO_TEST_CASE(N_histo) {
  namespace expr = boost::log::expressions;
  namespace keywords = boost::log::keywords;
  //  init_sink();
  default_logger logger;


  //boost::log::core::get()->set_filter(expr::attr<severity_level>("Severity") >= info);
  boost::log::core::get()->set_logging_enabled(false);

  int L = 100;
  int K = 10000;
  double c = 0.1;
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
    if (i % 100 == 0) cout << "Packet num. " << i << endl;

    
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
}

BOOST_AUTO_TEST_SUITE_END()
