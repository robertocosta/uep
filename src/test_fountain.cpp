#include "decoder.hpp"
#include "encoder.hpp"
#include "packets.hpp"

#include <algorithm>
#include <queue>
#include <iostream>

packet random_pkt(int size) {
  static std::independent_bits_engine<std::mt19937, 8, uint8_t> g;
  packet p;
  p.resize(size);
  for (int i=0; i < size; i++) {
    uint8_t rnd = g();
    p[i] = rnd;
  }
  return p;
}

using namespace std;

const int L = 1024;
const int K = 10;
const int N = 13;


int main(int, char**) {
  fountain_encoder enc(K);
  fountain_decoder dec(K);
  std::vector<packet> original;
  for (int i = 0; i < 1008; i++) {
    packet p = random_pkt(L);
    original.push_back(p);
    enc.push_input(fountain_packet(move(p)));
  }

  for(;;) {
    if (!enc.has_block()) {
      cout << "Out of blocks" << endl;
      break;
    }
    fountain_packet c = enc.next_coded();
    cout << "Coded pkt " << c.block_number() << ":" << c.sequence_number() << endl;
    if (c.sequence_number() == N-1) {
      cout << "Generated N="<<N<<" pkts" << endl;
      enc.discard_block();
    }

    dec.push_coded(move(c));
    if (dec.has_decoded()) {
      cout << "Decoded block "<<dec.blockno()<<endl;

      bool equal = std::equal(dec.decoded_begin(), dec.decoded_end(),
			      original.begin() + dec.blockno() * K);
      if (equal) cout << "Successful reception" << endl;
      else cout << "Error in reception" << endl;
      enc.discard_block();
    }
  }
  
  return 0;
}
