#ifndef ENCODER_HPP
#define ENCODER_HPP

#include "packets.hpp"
#include "rng.hpp"

#include <queue>
#include <vector>

class fountain_encoder {
public:
  explicit fountain_encoder(const degree_distribution &distr);
  explicit fountain_encoder(const fountain &f);

  void push_input(const packet &p);
  void push_input(packet &&p);
  fountain_packet next_coded();
  void discard_block();

  bool has_block() const;
  std::uint_fast16_t K() const;
  std::uint_fast16_t blockno() const;
  std::uint_fast16_t seqno() const;
  std::uint_fast32_t block_seed() const;

  fountain the_fountain() const;
  std::vector<packet>::const_iterator current_block_begin() const;
  std::vector<packet>::const_iterator current_block_end() const;

private:
  fountain fount;
  std::uint_fast16_t blockno_;
  std::uint_fast32_t block_seed_;
  std::uint_fast16_t seqno_;
  std::queue<packet> input_queue;
  std::vector<packet> input_block;
  std::random_device rd;

  void check_has_block();
  std::uint_fast32_t next_seed();
};

#endif
