#ifndef ENCODER_HPP
#define ENCODER_HPP

#include "packets.hpp"
#include "rng.hpp"

typedef std::vector<fountain_packet> coded_block;
typedef std::vector<packet_data> uncoded_block;

class fountain_encoder {
public:
  explicit fountain_encoder(int K_);
  explicit fountain_encoder(int K_, int N_);

  void input_block(const uncoded_block &ub);
  fountain_packet next_packet();

private:
  const int K;
  int N;
  fountain<int> f;
  int blockno;
  int seqno;
  uncoded_block input;

  int next_seqno();
  int next_blockno();
};

#endif
