#include "decoder.hpp"
#include "encoder.hpp"
#include "packets.hpp"

#include <algorithm>
#include <queue>
#include <iostream>

fountain_packet random_pkt(int size) {
  static std::independent_bits_engine<std::mt19937, 8, uint8_t> g;
  uint8_t *data = new uint8_t[size];
  std::shared_ptr<uint8_t> ptr(data, std::default_delete<uint8_t[]>());
  for (int i=0; i < size; i++) {
    data[i] = g();
  }
  return fountain_packet(ptr, size);
}

using namespace std;

const int L = 1024;
const int K = 10;
const int N = 15;


int main(int, char**) {
  fountain_encoder enc(K);
  fountain_decoder dec(K);
  std::vector<fountain_packet> original;
  for (int i = 0; i < 1008; i++) {
    fountain_packet p = random_pkt(L);
    original.push_back(p.clone());
    enc.push_input(p);
  }

  for(;;) {
    if (!enc.has_block()) {
      cout << "Out of blocks" << endl;
      break;
    }
    fountain_packet c = enc.next_coded();
    cout << "Coded pkt " << c.blockno() << ":" << c.seqno() << endl;
    if (c.seqno() == N-1) {
      cout << "Generated N="<<N<<" pkts" << endl;
      enc.discard_block();
    }

    dec.push_coded(c);
    if (dec.has_decoded()) {
      cout << "Decoded block "<<dec.blockno()<<endl;

      bool equal = std::equal(dec.decoded_begin(), dec.decoded_end(),
			      original.begin() + dec.blockno() * K,
      		      packet_data_equal());
      if (equal) cout << "Successful reception" << endl;
      else cout << "Error in reception" << endl;
    }
  }
  
  return 0;
}
