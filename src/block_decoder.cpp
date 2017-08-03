#include "block_decoder.hpp"

using namespace std;

namespace uep {

block_decoder::block_decoder(const lt_row_generator &rg) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  rowgen(rg), decoded(rg.K()), decoded_count_(0) {
  link_cache.reserve(rg.K());
}

void block_decoder::check_correct_block(const fountain_packet &p) {
  if (received_pkts.empty()) {
    blockno = p.block_number();
    rowgen.reset(p.block_seed());
    pktsize = p.size();
  }
  else if (blockno != static_cast<size_t>(p.block_number()) ||
	   seed() != (seed_t)p.block_seed()) {
    throw std::runtime_error("All packets must belong to the same block");
  }
  else if (pktsize != p.size()) {
    throw std::runtime_error("All packets must have the same size");
  }
}

bool block_decoder::push(fountain_packet &&p) {
  check_correct_block(p);
  size_t p_seqno = p.sequence_number();
  auto ins_ret = received_pkts.insert(move(p));
  if (!ins_ret.second) return false;

  if (link_cache.size() <= p_seqno) {
    size_t prev_size = link_cache.size();
    link_cache.resize(p_seqno+1);
    for (size_t i = prev_size; i < p_seqno+1; ++i) {
      link_cache[i] = rowgen.next_row();
    }
  }

  if (!has_decoded()) { // do partial decoding even with < K pkts
    run_message_passing();
  }

  return true;
}

bool block_decoder::push(const fountain_packet &p) {
  fountain_packet p_copy(p);
  return push(move(p_copy));
}

void block_decoder::reset() {
  //rowgen.reset();
  received_pkts.clear();
  link_cache.clear();
  decoded.assign(block_size(), packet());
  decoded_count_ = 0;
}

block_decoder::seed_t block_decoder::seed() const {
  return rowgen.seed();
}

std::size_t block_decoder::block_number() const {
  return blockno;
}

bool block_decoder::has_decoded() const {
  return decoded_count() == block_size();
}

std::size_t block_decoder::decoded_count() const {
  return decoded_count_;
}

std::size_t block_decoder::received_count() const {
  return received_pkts.size();
}

std::size_t block_decoder::block_size() const {
  return rowgen.K();
}

block_decoder::const_block_iterator block_decoder::block_begin() const {
  return const_block_iterator(decoded.cbegin(),
			      decoded.cend());
}

block_decoder::const_block_iterator block_decoder::block_end() const {
  return const_block_iterator(decoded.cend(),
			      decoded.cend());
}

block_decoder::const_partial_iterator block_decoder::partial_begin() const {
  return decoded.cbegin();
}

block_decoder::const_partial_iterator block_decoder::partial_end() const {
  return decoded.cend();
}

block_decoder::operator bool() const {
  return has_decoded();
}

bool block_decoder::operator!() const {
  return !has_decoded();
}

void block_decoder::run_message_passing() {
  // Setup the context
  typedef boost::transform_iterator<fp2lazy_conv,
				    received_t::const_iterator
				    > mp_ctx_out_iter;
  mp_ctx_out_iter out_b(received_pkts.cbegin(), fp2lazy_conv());
  mp_ctx_out_iter out_e(received_pkts.cend(), fp2lazy_conv());
  mp_ctx_t mp_ctx(rowgen.K(), out_b, out_e);

  size_t mp_out_index = 0;
  for (auto j = received_pkts.cbegin(); j != received_pkts.cend(); ++j) {
    const lt_row_generator::row_type &row = link_cache[j->sequence_number()];
    for (auto i = row.cbegin(); i != row.cend(); ++i) {
      mp_ctx.add_edge(*i, mp_out_index);
    }
    ++mp_out_index;
  }

  // Run mp
  mp_ctx.run();

  // Copy decoded packets
  if (mp_ctx.decoded_count() != 0) {
    mp_packet_iter mp_sb(mp_ctx.input_symbols_begin(), lazy2p_conv());
    mp_packet_iter mp_se(mp_ctx.input_symbols_end(), lazy2p_conv());
    copy(mp_sb, mp_se, decoded.begin());
    decoded_count_ = mp_ctx.decoded_count();
  }

  BOOST_LOG(perf_lg) << "block_decoder::run_message_passing decoded_pkts="
		     << mp_ctx.decoded_count()
		     << " received_pkts="
		     << received_pkts.size();
}

}
