#include "encoder.hpp"

#include <stdexcept>
#include <limits>

using namespace std;

fountain_encoder::fountain_encoder(std::uint_fast32_t K) :
  K_(K),
  fount(K),
  blockno_(0),
  seqno_(0) {
  input_block.reserve(K);
}

void fountain_encoder::push_input(const fountain_packet &p) {
  input_queue.push(p);
  check_has_block();
}

fountain_packet fountain_encoder::next_coded() {
  if (!has_block())
    throw runtime_error("Does not have a full block");
  auto sel = fount.next_row();
  auto i = sel.begin();
  fountain_packet first = input_block[*i].clone();
  i++;
  for (; i != sel.end(); i++) {
    first ^=  input_block[*i];
  }
  first.blockno(blockno_);
  first.seqno(seqno_++);
  return first;
}

void fountain_encoder::discard_block() {
  input_block.clear();
  fount.reset();
  if (blockno_ == numeric_limits<uint_fast32_t>::max())
    throw runtime_error("Block number overflow");
  blockno_++;
  seqno_ = 0;
  check_has_block();
}

bool fountain_encoder::has_block() const {
  return input_block.size() == K_;
}

std::uint_fast32_t fountain_encoder::K() const {
  return K_;
}

std::uint_fast32_t fountain_encoder::blockno() const {
  return blockno_;
}

std::uint_fast32_t fountain_encoder::seqno() const {
  return seqno_;
}

const fountain &fountain_encoder::the_fountain() const {
  return fount;
}

const std::vector<fountain_packet> &fountain_encoder::current_block() const {
  return input_block;
}

void fountain_encoder::check_has_block() {
  if (!has_block() && input_queue.size() >= K_) {
    for (uint_fast32_t i = 0; i < K_; i++) {
      input_block.push_back(input_queue.front());
      input_queue.pop();
    }
  }
}
