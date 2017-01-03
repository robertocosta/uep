#ifndef ENCODER_HPP
#define ENCODER_HPP

#include "packets.hpp"
#include "rng.hpp"

#include <queue>
#include <vector>

class fountain_encoder {
public:
  explicit fountain_encoder(std::uint_fast32_t K);

  void push_input(const fountain_packet &p);
  fountain_packet next_coded();
  void discard_block();

  bool has_block() const;
  std::uint_fast32_t K() const;
  std::uint_fast32_t blockno() const;
  std::uint_fast32_t seqno() const;

  const fountain &the_fountain() const;
  const std::vector<fountain_packet> &current_block() const;
private:
  const std::uint_fast32_t K_;
  std::uint_fast32_t blockno_;
  std::uint_fast32_t seqno_;
  fountain fount;
  std::queue<fountain_packet> input_queue;
  std::vector<fountain_packet> input_block;

  void check_has_block();
};

#endif
