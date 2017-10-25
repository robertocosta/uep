#ifndef UEP_BLOCK_DECODER_HPP
#define UEP_BLOCK_DECODER_HPP

#include <forward_list>
#include <set>
#include <vector>

#include "counter.hpp"
#include "lazy_xor.hpp"
#include "log.hpp"
#include "message_passing.hpp"
#include "packets.hpp"
#include "rng.hpp"
#include "utils.hpp"

namespace uep {

/** Converter to evaluate lazy_xors into packets. */
template <std::size_t MAX_SIZE>
struct lazy2p_conv {
  packet operator()(const lazy_xor<buffer_type,MAX_SIZE> &lx) const {
    if (lx) {
      packet p;
      p.buffer() = lx.evaluate();
      return p;
    }
    else {
      return packet();
    }
  }
};

/** Iterator adaptor that converts from lazy_xors to packets. */
template <class BaseIter, std::size_t MAX_SIZE>
using lazy2p_iterator = boost::transform_iterator<lazy2p_conv<MAX_SIZE>, BaseIter>;

/** Class to decode a single LT-encoded block of packets.
 *  The LT-code parameters are given by the lt_row_generator passed to
 *  the constructor. The seed is read from the fountain_packets.
 */
class block_decoder {
private:
  /** Max size for the lazy_xors. */
  static constexpr std::size_t LX_MAX_SIZE = 1;

  /** Type of the symbols used for the mp algorithm. */
  typedef lazy_xor<buffer_type,LX_MAX_SIZE> sym_t;
  /** Type of the underlying message passing context. */
  typedef mp::mp_context<sym_t> mp_ctx_t;
  /** Type of the container used to cache the row generator output. */
  typedef std::vector<base_row_generator::row_type> link_cache_t;

public:
  /** Iterator over the input packets, either decoded or empty. */
  typedef lazy2p_iterator<mp_ctx_t::inputs_iterator,
			  LX_MAX_SIZE> const_partial_iterator;
  /** Iterator over the decoded input packets. */
  typedef lazy2p_iterator<mp_ctx_t::decoded_iterator,
			  LX_MAX_SIZE> const_block_iterator;
  /** Type of the seed used by the row generator. */
  typedef lt_row_generator::rng_type::result_type seed_t;

  /** Construct with a copy of the given lt_row_generator. */
  explicit block_decoder(const lt_row_generator &rg);
  /** Construct with the given row generator. */
  explicit block_decoder(std::unique_ptr<base_row_generator> &&rg);

  /** Reset the decoder to the initial state. */
  void reset();
  /** Add an encoded packet to the current block.
   * If the packet is a duplicate (same sequence_number), it is
   * ignored and the method returns false.
   */
  bool push(fountain_packet &&p);
  /** \sa push(fountain_packet&&) */
  bool push(const fountain_packet &p);

  /** Add many packets to the current block. Try to decode only once
   *  all packets have been pushed. Return the number of unique
   *  packets that were added to the block. To move from the packets
   *  the iterators can be wrapped in std::move_iterator.
   */
  template <class Iter>
  std::size_t push(Iter in_first, Iter in_last);

  /** Return the seed used to decode the current block. */
  seed_t seed() const;
  /** Return the current block's block_number. */
  std::size_t block_number() const;
  /** Return true when the entire input block has been decoded. */
  bool has_decoded() const;
  /** Number of input packets that have been successfully decoded. */
  std::size_t decoded_count() const;
  /** Number of received unique packets. */
  std::size_t received_count() const;
  /** Block size. */
  std::size_t block_size() const;

  /** Return an iterator to the beginning of the decoded packets.
   * The interval [block_begin(), block_end()) always contains the
   * decoded_count() packets that have been decoded.
   * \sa block_end()
   */
  const_block_iterator block_begin() const;
  /** Return an iterator to the end of the decoded packets.
   * \sa block_begin()
   */
  const_block_iterator block_end() const;

  /** Return an iterator to the beginning of the partially decoded
   *  block.
   *  The partially decoded block has always size block_size() but
   *  some of the packets may be empty.
   *  \sa partial_end()
   */
  const_partial_iterator partial_begin() const;
  /** Return an iterator to the end of the partially decoded
   *  block.
   *  \sa partial_begin()
   */
  const_partial_iterator partial_end() const;

  /** Return the average time to run message passing measured since
   *  the last reset.
   */
  double average_message_passing_time() const;
  /** Return the average time to setup the mp context measured since
   *  the last reset.
   */
  double average_mp_setup_time() const;

  /** Return true when the decoder has decoded a full block. */
  explicit operator bool() const;
  /** Return true when the encoder does not have decoded a full block. */
  bool operator!() const;

private:
  log::default_logger basic_lg, perf_lg;

  std::unique_ptr<base_row_generator> rowgen;
  std::set<std::size_t> received_seqnos;
  link_cache_t link_cache;
  std::forward_list<fountain_packet> last_received;
  mp_ctx_t mp_ctx; /**< Context used to run the mp algorithm and hold
		    *   the result.
		    */
  mp_ctx_t mp_pristine; /**< Keep a version of the context that was
			 *   never "runned".
			 */
  std::size_t blockno;
  std::size_t pktsize;

  stat::average_counter avg_mp; /**< Average time to run the message
				 *   passing algorithm.
				 */
  stat::average_counter avg_setup; /**< Average time to setup
				    *	mp_ctx before each run.
				    */

  /** Check the blockno, seqno and seed of the packet and raise an
   *  exception if they don't match the current block.
   */
  void check_correct_block(const fountain_packet &p);
  /** Run the message passing algortihm over the currently received
   *  packets.
   */
  void run_message_passing();
};

//		  block_decoder template definitions
template <class Iter>
std::size_t block_decoder::push(Iter in_first, Iter in_last) {
  // Ignore packets after successful decoding
  if (has_decoded()) {
    return 0;
  }

  std::size_t pushed = 0;
  std::size_t max_seqno = 0;
  for (auto i = in_first; i != in_last; ++i) {
    fountain_packet p(*i);
    check_correct_block(p);
    size_t p_seqno = p.sequence_number();

    auto ins_ret = received_seqnos.insert(p_seqno);
    // Ignore duplicates
    if (!ins_ret.second) {
      break;
    }

    last_received.push_front(std::move(p));
    ++pushed;
    if (max_seqno < p_seqno)
      max_seqno = p_seqno;
  }

  // Generate enough output links
  if (link_cache.size() <= max_seqno) {
    size_t prev_size = link_cache.size();
    link_cache.resize(max_seqno+1);
    for (size_t i = prev_size; i < max_seqno+1; ++i) {
      link_cache[i] = rowgen->next_row();
    }
  }

  if (pushed > 0) run_message_passing();
  return pushed;
}

}

#endif
