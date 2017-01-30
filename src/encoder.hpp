#ifndef ENCODER_HPP
#define ENCODER_HPP

#include "packets.hpp"
#include "rng.hpp"

#include <limits>
#include <queue>
#include <stdexcept>
#include <vector>

template<class Gen = std::random_device>
class fountain_encoder {
public:
  typedef Gen seed_generator_type;

  explicit fountain_encoder(const degree_distribution &distr);
  explicit fountain_encoder(const fountain &f);

  void push_input(const packet &p);
  void push_input(packet &&p);
  fountain_packet next_coded();
  void discard_block();

  bool has_block() const;
  int K() const;
  int blockno() const;
  int seqno() const;
  int block_seed() const;

  fountain the_fountain() const;
  seed_generator_type the_seed_generator() const;
  std::vector<packet>::const_iterator current_block_begin() const;
  std::vector<packet>::const_iterator current_block_end() const;

private:
  fountain fount;
  int blockno_;
  int block_seed_;
  int seqno_;
  std::queue<packet> input_queue;
  std::vector<packet> input_block;
  seed_generator_type seed_gen;

  void check_has_block();
  int next_seed();
};

// Templates definition

template<class Gen>
fountain_encoder<Gen>::fountain_encoder(const degree_distribution &distr) :
  fountain_encoder(fountain(distr)) {
}

template<class Gen>
fountain_encoder<Gen>::fountain_encoder(const fountain &f) :
  fount(f), blockno_(0), seqno_(0) {
  input_block.reserve(f.K());
  block_seed_ = next_seed();
  fount.reset(block_seed_);
}

template<class Gen>
void fountain_encoder<Gen>::push_input(packet &&p) {
  input_queue.push(move(p));
  check_has_block();
}

template<class Gen>
void fountain_encoder<Gen>::push_input(const packet &p) {
  packet p_copy(p);
  push_input(move(p_copy));
}

template<class Gen>
fountain_packet fountain_encoder<Gen>::next_coded() {
  if (!has_block())
    throw std::runtime_error("Does not have a full block");
  auto sel = fount.next_row(); // genera riga
  auto i = sel.cbegin(); // iteratore sulla riga
  fountain_packet first(input_block[*i]); // mette dentro first una copia dell'input
  i++;
  for (; i != sel.end(); i++) { // begin == end è la condizione di fine ciclo
    first ^= input_block[*i];
  }
  first.block_number(blockno_);
  first.block_seed(block_seed_);
  first.sequence_number(seqno_); // seqno: = prossimo seq. number del pacchetto da inviare
  if (seqno_ == std::numeric_limits<int>::max())
    throw std::runtime_error("Seqno overflow");
  ++seqno_;
  return first;
}

template<class Gen>
void fountain_encoder<Gen>::discard_block() {
  input_block.clear();
  block_seed_ = next_seed();
  fount.reset(block_seed_);
  if (blockno_ == std::numeric_limits<int>::max())
    throw std::runtime_error("Block number overflow");
  blockno_++;
  seqno_ = 0;
  check_has_block();
}

template<class Gen>
bool fountain_encoder<Gen>::has_block() const {
  return input_block.size() == (size_t)K();
}

template<class Gen>
int fountain_encoder<Gen>::K() const {
  return fount.K();
}

template<class Gen>
int fountain_encoder<Gen>::blockno() const {
  return blockno_;
}

template<class Gen>
int fountain_encoder<Gen>::seqno() const {
  return seqno_;
}

template<class Gen>
int fountain_encoder<Gen>::block_seed() const {
  return block_seed_;
}

template<class Gen>
fountain fountain_encoder<Gen>::the_fountain() const {
  return fount;
}

template<class Gen>
Gen fountain_encoder<Gen>::the_seed_generator() const {
  return seed_gen;
}

template<class Gen>
std::vector<packet>::const_iterator
fountain_encoder<Gen>::current_block_begin() const {
  return input_block.cbegin();
}

template<class Gen>
std::vector<packet>::const_iterator
fountain_encoder<Gen>::current_block_end() const {
  return input_block.cend();
}

template<class Gen>
void fountain_encoder<Gen>::check_has_block() {
  if (!has_block() && input_queue.size() >= (size_t)K()) {
    for (int i = 0; i < K(); i++) {
      packet p;
      std::swap(p, input_queue.front());
      input_queue.pop();
      input_block.push_back(move(p));
    }
  }
}

template<class Gen>
int fountain_encoder<Gen>::next_seed() {
  return seed_gen();
}

#endif
