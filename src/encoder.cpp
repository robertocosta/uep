#include "encoder.hpp"

#include <stdexcept>
#include <limits>

using namespace std;

fountain_encoder::fountain_encoder(std::uint_fast16_t K) :
  K_(K),
  fount(K),
  blockno_(0),
  seqno_(0) {
  input_block.reserve(K);
  block_seed_ = next_seed();
  fount.reset(block_seed_);
}

void fountain_encoder::push_input(fountain_packet &&p) {
  input_queue.push(move(p));
  check_has_block();
}

void fountain_encoder::push_input(const fountain_packet &p) {
  fountain_packet p_copy(p);
  push_input(move(p_copy));
}

fountain_packet fountain_encoder::next_coded() {
  if (!has_block())
    throw runtime_error("Does not have a full block");
  auto sel = fount.next_row();
  auto i = sel.begin();
  fountain_packet first(input_block[*i]);
  i++;
  for (; i != sel.end(); i++) {
    first ^= input_block[*i];
  }
  first.block_number(blockno_);
  first.block_seed(block_seed_);
  if (seqno_ == numeric_limits<uint_fast16_t>::max())
    throw new runtime_error("Seqno overflow");
  first.sequence_number(seqno_++);
  return first;
}

void fountain_encoder::discard_block() {
  input_block.clear();
  block_seed_ = next_seed();
  fount.reset(block_seed_);
  if (blockno_ == numeric_limits<uint_fast16_t>::max())
    throw runtime_error("Block number overflow");
  blockno_++;
  seqno_ = 0;
  check_has_block();
}

bool fountain_encoder::has_block() const {
  return input_block.size() == K_;
}

std::uint_fast16_t fountain_encoder::K() const {
  return K_;
}

std::uint_fast16_t fountain_encoder::blockno() const {
  return blockno_;
}

std::uint_fast16_t fountain_encoder::seqno() const {
  return seqno_;
}

std::uint_fast32_t fountain_encoder::block_seed() const {
  return block_seed_;
}

const fountain &fountain_encoder::the_fountain() const {
  return fount;
}

std::vector<fountain_packet>::const_iterator
fountain_encoder::current_block_begin() const {
  return input_block.cbegin();
}

std::vector<fountain_packet>::const_iterator
fountain_encoder::current_block_end() const {
  return input_block.cend();
}

void fountain_encoder::check_has_block() {
  if (!has_block() && input_queue.size() >= K_) {
    for (uint_fast16_t i = 0; i < K_; i++) {
      fountain_packet p;
      swap(p, input_queue.front());
      input_queue.pop();
      input_block.push_back(move(p));
    }
  }
}

std::uint_fast32_t fountain_encoder::next_seed() {
  return rd();
  //return mt19937::default_seed;
}
