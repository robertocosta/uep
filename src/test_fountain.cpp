#include <queue>
#include <iostream>
#include "packets.hpp"
#include "encoder.hpp"

const int L = 1024;
const int K = 10;

packet_data data_pkt() {
  static int i = 1;
  packet_data d(L, i++);
  return d;
}

using namespace std;


int main(int, char**) {

  vector<packet_data> in_queue;
  for (int i = 0; i < K; i++) {
    in_queue.push_back(data_pkt());
  }

  fountain_encoder enc(K);
  enc.input_block(in_queue);

  for (int i=0; i < 25; i++) {
    fountain_packet p = enc.next_packet();
    cout << p.sequence_number() << endl;
  }
  
  return 0;
}
