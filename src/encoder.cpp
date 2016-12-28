#include "encoder.hpp"

#include <stdexcept>
#include <limits>

using namespace std;

fountain_encoder::fountain_encoder(int K_) :
  fountain_encoder(K_, K_) {
}

fountain_encoder::fountain_encoder(int K_, int N_) :
  K(K_),
  N(N_),
  f(K_),
  blockno(0),
  seqno(0) {
}

void fountain_encoder::input_block(const uncoded_block &ub) {
  if (ub.size() != K)
    throw runtime_error("Input blocks must be of size K");
  input = ub;
  seqno = 0;
  next_blockno();
  f.reset(); // TODO: what seed?
}

fountain_packet fountain_encoder::next_packet() {
  auto sel = f.next_packet_selection();
  auto i = sel.begin();
  packet_data first = input[(*i)-1];
  i++;
  fountain_packet p(first);
  for (; i != sel.end(); i++) {
    packet_data to_xor = input[(*i)-1];
    p.cumulative_xor(to_xor);
  }
  p.block_number(blockno);
  p.sequence_number(next_seqno());
  return p;
}


int fountain_encoder::next_seqno() {
  if (seqno == numeric_limits<int>::max())
    throw runtime_error("Sequence number overflow");
  return ++seqno;
}

int fountain_encoder::next_blockno() {
  if (blockno == numeric_limits<int>::max())
    throw runtime_error("Block number overflow");
  return ++blockno;
}
