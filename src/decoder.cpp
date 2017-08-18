#include "decoder.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <utility>

using namespace std;

namespace uep {

lt_decoder::lt_decoder(const parameter_set &ps) :
  lt_decoder(ps.K, ps.c, ps.delta) {
}

lt_decoder::lt_decoder(std::size_t K, double c, double delta) :
  lt_decoder(make_robust_lt_row_generator(K, c, delta)) {
}

lt_decoder::lt_decoder(const degree_distribution &distr) :
  lt_decoder(lt_row_generator(distr)) {
}

lt_decoder::lt_decoder(const lt_row_generator &rg) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  the_block_decoder(rg),
  the_output_queue(rg.K()),
  blockno_counter(MAX_BLOCKNO, BLOCK_WINDOW),
  has_enqueued(false),
  uniq_recv_count(0),
  tot_dec_count(0),
  tot_failed_count(0) {
  blockno_counter.set(0);
}

void lt_decoder::push(fountain_packet &&p) {
  // Make a size-one iter range with p, *mv_ptr should be an rvalue
  auto mv_ptr = std::make_move_iterator(&p);
  auto mv_end = std::make_move_iterator(&p + 1);
  push(mv_ptr, mv_end);
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

void lt_decoder::flush() {
  auto bnc_copy(blockno_counter);
  flush_small_blockno(bnc_copy.next());
}

void lt_decoder::flush_n_blocks(std::size_t n) {
  auto bnc_new(blockno_counter);
  std::size_t win = blockno_counter.comparison_window();
  while (n > win) {
    bnc_new.next(win);
    flush_small_blockno(bnc_new.value());
    n -= win;
  }
  bnc_new.next(n);
  flush_small_blockno(bnc_new.value());
}

void lt_decoder::flush(std::size_t blockno_) {
  // This still cannot flush more than MAX_BLOCKNO blocks
  auto target_bn(blockno_counter);
  target_bn = blockno_;
  size_t dist = blockno_counter.forward_distance(target_bn);
  size_t win =  blockno_counter.comparison_window();

  size_t times = dist / win;
  auto bnc_copy(blockno_counter);
  for (size_t i = 0; i < times; ++i) {
    bnc_copy.next(win);
    flush_small_blockno(bnc_copy.value());
  }
  flush_small_blockno(blockno_);
}

void lt_decoder::flush_small_blockno(std::size_t blockno_) {
  auto recv_blockno(blockno_counter);
  recv_blockno.set(blockno_);
  size_t dist = blockno_counter.forward_distance(recv_blockno);

  if (dist > 1 || !has_decoded())
    BOOST_LOG_SEV(basic_lg, log::info) << "Decoder is skipping to block "
				       << blockno_;
  else
    BOOST_LOG_SEV(basic_lg, log::debug) << "Decoder is moving to next block "
					<< blockno_;

  enqueue_partially_decoded();

  // Push dist-1 empty blocks
  if (dist > 1) {
    const std::vector<packet> empty_block(K());
    for (size_t i = 0; i < dist - 1; ++i) {
      the_output_queue.push_shallow(empty_block.cbegin(),
				    empty_block.cend());

    }
    tot_failed_count += K() * (dist - 1);
  }

  // Start to decode the new block
  the_block_decoder.reset();
  has_enqueued = false;
  blockno_counter = recv_blockno;
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

std::size_t lt_decoder::total_failed_count() const {
  return tot_failed_count;
}

double lt_decoder::average_push_time() const {
  return avg_push_t.value();
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
  tot_failed_count += K() - the_block_decoder.decoded_count();

  has_enqueued = true;

  BOOST_LOG(perf_lg) << "lt_decoder::enqueue_partially_decoded"
		     << " blockno="
		     << blockno()
		     << " decoded_pkts="
		     << the_block_decoder.decoded_count()
		     << " avg_mp_time="
		     << the_block_decoder.average_message_passing_time()
		     << " avg_mp_setup_time="
		     << the_block_decoder.average_mp_setup_time()
		     << " avg_push_time="
		     << average_push_time();

  if (the_block_decoder.has_decoded())
    BOOST_LOG_SEV(basic_lg, log::debug) <<
      "Decoder enqueued a fully decoded block";
  else
    BOOST_LOG_SEV(basic_lg, log::debug) <<
      "Decoder enqueued a partially decoded block";
}

}
