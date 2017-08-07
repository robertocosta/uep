#include "block_decoder.hpp"

#include <chrono>

using namespace std;
using namespace std::chrono;

namespace uep {

block_decoder::block_decoder(const lt_row_generator &rg) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  rowgen(rg), mp_ctx(rg.K()) {
  link_cache.reserve(rg.K());
}

void block_decoder::check_correct_block(const fountain_packet &p) {
  // First packet: set blockno, length, seed
  if (received_pkts.empty()) {
    blockno = p.block_number();
    rowgen.reset(p.block_seed());
    pktsize = p.size();
  }
  // Other packets: check blockno, seed
  else if (blockno != static_cast<size_t>(p.block_number()) ||
	   seed() != static_cast<seed_t>(p.block_seed())) {
    throw std::runtime_error("All packets must belong to the same block");
  }
  // Check length
  else if (pktsize != p.size()) {
    throw std::runtime_error("All packets must have the same size");
  }
}

bool block_decoder::push(fountain_packet &&p) {
  check_correct_block(p);
  size_t p_seqno = p.sequence_number();

  // Ignore packets after successful decoding
  if (has_decoded()) {
    return false;
  }

  auto ins_ret = received_pkts.insert(move(p));
  // Ignore duplicates
  if (!ins_ret.second) {
    return false;
  }

  // Generate enough output links
  if (link_cache.size() <= p_seqno) {
    size_t prev_size = link_cache.size();
    link_cache.resize(p_seqno+1);
    for (size_t i = prev_size; i < p_seqno+1; ++i) {
      link_cache[i] = rowgen.next_row();
    }
  }

  run_message_passing();
  return true;
}

bool block_decoder::push(const fountain_packet &p) {
  fountain_packet p_copy(p);
  return push(move(p_copy));
}

void block_decoder::reset() {
  rowgen.reset();
  received_pkts.clear();
  link_cache.clear();
  mp_ctx.reset();
  avg_mp.reset();
}

block_decoder::seed_t block_decoder::seed() const {
  if (received_pkts.empty()) throw std::runtime_error("No received packets");
  return rowgen.seed();
}

std::size_t block_decoder::block_number() const {
  if (received_pkts.empty()) throw std::runtime_error("No received packets");
  return blockno;
}

bool block_decoder::has_decoded() const {
  return mp_ctx.has_decoded();
}

std::size_t block_decoder::decoded_count() const {
  return mp_ctx.decoded_count();
}

std::size_t block_decoder::received_count() const {
  return received_pkts.size();
}

std::size_t block_decoder::block_size() const {
  return rowgen.K();
}

block_decoder::const_block_iterator block_decoder::block_begin() const {
  return const_block_iterator(mp_ctx.decoded_symbols_begin(), lazy2p_conv());
}

block_decoder::const_block_iterator block_decoder::block_end() const {
  return const_block_iterator(mp_ctx.decoded_symbols_end(), lazy2p_conv());
}

block_decoder::const_partial_iterator block_decoder::partial_begin() const {
  return const_partial_iterator(mp_ctx.input_symbols_begin(), lazy2p_conv());
}

block_decoder::const_partial_iterator block_decoder::partial_end() const {
  return const_partial_iterator(mp_ctx.input_symbols_end(), lazy2p_conv());
}

double block_decoder::average_message_passing_time() const {
  return avg_mp.value();
}

block_decoder::operator bool() const {
  return has_decoded();
}

bool block_decoder::operator!() const {
  return !has_decoded();
}

void block_decoder::run_message_passing() {
  auto tic = high_resolution_clock::now();

  // Rebuild the context each time
  mp_ctx.reset();
  for (auto i = received_pkts.cbegin(); i != received_pkts.cend(); ++i) {
    const lt_row_generator::row_type &row = link_cache[i->sequence_number()];
    mp_ctx.add_output(lazy_xor<packet>(&(*i)), row.cbegin(), row.cend());
  }

  mp_ctx.run();

  auto mp_tdiff =
    duration_cast<duration<double>>(high_resolution_clock::now() - tic);

  BOOST_LOG(perf_lg) << "block_decoder::run_message_passing mp_time="
		     << mp_tdiff.count();
  avg_mp.add_sample(mp_tdiff.count());
  BOOST_LOG(perf_lg) << "block_decoder::run_message_passing decoded_pkts="
		     << mp_ctx.decoded_count()
		     << " received_pkts="
		     << received_pkts.size();
}

}
