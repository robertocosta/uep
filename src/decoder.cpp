#include "decoder.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

using namespace std;

namespace uep {

lt_decoder::lt_decoder(std::size_t K, double c, double delta) :
  lt_decoder(make_robust_lt_row_generator(K, c, delta)) {
}

lt_decoder::lt_decoder(const degree_distribution &distr) :
  lt_decoder(lt_row_generator(distr)) {
}

lt_decoder::lt_decoder(const lt_row_generator &rg) :
  the_block_decoder(rg),
  the_output_queue(rg.K()),
  blockno_counter(MAX_BLOCKNO),
  has_enqueued(false) {
  blockno_counter.set(0);
}

void lt_decoder::push(fountain_packet &&p) {
  if (blockno_counter.last() != static_cast<size_t>(p.block_number())) {
    decltype(blockno_counter) recv_blockno(MAX_BLOCKNO);
    recv_blockno.set(p.block_number());
    size_t dist = blockno_counter.forward_distance(recv_blockno);

    if (dist > BLOCK_WINDOW) {
      // PUT_STAT_COUNTER(loggers.old_dropped);
      // BOOST_LOG_SEV(loggers.text, info) << "Drop packet for an old block: " << p;
      return;
    }
    else {
      the_block_decoder.reset();
      has_enqueued = false;
      blockno_counter = recv_blockno;
    }
  }

  the_block_decoder.push(move(p));

  if (!has_enqueued && the_block_decoder ) {
    the_output_queue.push_shallow(the_block_decoder.block_begin(),
				  the_block_decoder.block_end());
    has_enqueued = true;
  }
}

void lt_decoder::push(const fountain_packet &p) {
  fountain_packet p_copy(p);
  push(move(p_copy));
}

packet lt_decoder::next_decoded() {
  packet p = the_output_queue.front().shallow_copy();
  the_output_queue.pop();
  return p;
}

lt_decoder::const_block_iterator lt_decoder::decoded_begin() const {
  return the_block_decoder.block_begin();
}

lt_decoder::const_block_iterator lt_decoder::decoded_end() const {
  return the_block_decoder.block_end();
}

bool lt_decoder::has_decoded() const {
  return the_block_decoder.has_decoded();
}

std::size_t lt_decoder::K() const {
  return the_block_decoder.block_size();
}

std::size_t lt_decoder::blockno() const {
  return blockno_counter.last();
}

int lt_decoder::block_seed() const {
  return the_block_decoder.seed();
}

size_t lt_decoder::received_count() const {
  return the_block_decoder.received_count();
}

size_t lt_decoder::decoded_count() const {
  return the_block_decoder.decoded_count();
}

size_t lt_decoder::queue_size() const {
  return the_output_queue.size();
}



}
