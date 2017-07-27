#include "decoder.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

using namespace std;

namespace uep {

lt_decoder::lt_decoder(const parameter_set &ps) :
  lt_decoder(ps.K, ps.c, ps.delta) {}

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
  has_enqueued(false),
  uniq_recv_count(0),
  tot_dec_count(0) {
  blockno_counter.set(0);
}

void lt_decoder::push(fountain_packet &&p) {
  if (blockno_counter.last() != static_cast<size_t>(p.block_number())) {
    // Check if more recent
    decltype(blockno_counter) recv_blockno(MAX_BLOCKNO);
    recv_blockno.set(p.block_number());
    size_t dist = blockno_counter.forward_distance(recv_blockno);

    if (dist > BLOCK_WINDOW) {
      // This is an old block
      // PUT_STAT_COUNTER(loggers.old_dropped);
      // BOOST_LOG_SEV(loggers.text, info) << "Drop packet for an old block: " << p;
      return;
    }
    else {
      enqueue_partially_decoded();
      // If there is a gap in the blocks fill with empty pkts
      if (dist > 1) {
	const std::vector<packet> empty_block(K());
	for (int i = 0; i < dist - 1; ++i) {
	  the_output_queue.push_shallow(empty_block.cbegin(),
					empty_block.cend());
	}
      }
      // Start to decode the new block
      the_block_decoder.reset();
      has_enqueued = false;
      blockno_counter = recv_blockno;
    }
  }

  bool uniq = the_block_decoder.push(move(p));
  if (uniq) ++uniq_recv_count;

  // Extract if fully decoded block (just once)
  if (the_block_decoder) {
    enqueue_partially_decoded();
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

circular_counter<std::size_t> lt_decoder::block_number_counter() const {
  return blockno_counter;
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

bool lt_decoder::has_queued_packets() const {
  return queue_size() > 0;
}

std::size_t lt_decoder::total_received_count() const {
  return uniq_recv_count;
}

std::size_t lt_decoder::total_decoded_count() const {
  return tot_dec_count;
}

lt_decoder::operator bool() const {
  return has_queued_packets();
}

bool lt_decoder::operator!() const {
  return !has_queued_packets();
}

void lt_decoder::enqueue_partially_decoded() {
  if (has_enqueued) return;

  the_output_queue.push_shallow(the_block_decoder.partial_begin(),
				the_block_decoder.partial_end());
  tot_dec_count += the_block_decoder.decoded_count();

  has_enqueued = true;
}

}
