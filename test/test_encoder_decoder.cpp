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

BOOST_FIXTURE_TEST_SUITE(encdec, test_ctx)

BOOST_AUTO_TEST_CASE(N_histo) {
  //  init_sink();

  int L = 1024;
  int K = 10;
  double c = 0.1;
  double delta = 0.5;
  int N = 100;
  int npkts = 12000;
  double p = 0.2;

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

  for(;;) {
    if (!enc.has_block()) { // se ha almeno k pacchetti in coda
      cout << "Out of blocks" << endl;
      break;
    }
    // codifica: prende un pacchetto lungo L, genera la riga della matrice per codificare, fa lo xor e lo mette in c (fountain_packet)
    fountain_packet c = enc.next_coded();

    if (rand() > p) dec.push_coded(move(c));

    if (dec.has_decoded()) {
      cout << "Decoded block "<<dec.blockno()<<endl;

      bool equal = std::equal(dec.decoded_begin(), dec.decoded_end(),
				original.begin() + dec.blockno() * K);
      if (equal) {
	cout << "Successful reception" << endl;
	required_N_histogram[enc.seqno()]++;
      }
      else cout << "Error in reception" << endl;
      enc.discard_block();
    }
    if (c.sequence_number() == N-1) {
      enc.discard_block();
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()
