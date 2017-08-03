#ifndef UEP_BLOCK_DECODER_HPP
#define UEP_BLOCK_DECODER_HPP

#include <functional>
#include <stdexcept>
#include <unordered_set>
#include <set>
#include <utility>
#include <vector>

#include "counter.hpp"
#include "lazy_xor.hpp"
#include "log.hpp"
#include "message_passing.hpp"
#include "packets.hpp"
#include "rng.hpp"
#include "utils.hpp"

namespace uep {

/** Comparator used to sort fountain_packets using < on their seqno. */
struct seqno_less {
  bool operator()(const fountain_packet &lhs, const fountain_packet &rhs) {
    return lhs.sequence_number() < rhs.sequence_number();
  }
};

/** Converter to evaluate lazy_xors into packets. */
struct lazy2p_conv {
  packet operator()(const lazy_xor<packet> &lx) const {
    if (lx)
      return lx.evaluate();
    else
      return packet();
  }
};

/** Converter to evaluate a (index,lazy_xor) pair. */
struct sym2pair_conv {
  typedef std::pair<std::size_t,packet> out_t;
  typedef std::pair<std::size_t,lazy_xor<packet>> in_t;

  out_t operator()(const in_t &p) const {
    return std::make_pair(p.first, p.second.evaluate());
  }
};

/** Converter to build lazy_xors from fountain_packets. */
struct fp2lazy_conv {
  lazy_xor<packet> operator()(const fountain_packet &p) const {
    return lazy_xor<packet>(&p);
  }
};

/** Converter to extract the second element from a pair. */
template <class T, class U>
struct pair2second_conv {
  U operator()(const std::pair<T,U> &p) const {
    return p.second;
  }
};

/** Class to decode a single LT-encoded block of packets.
 *  The LT-code parameters are given by the lt_row_generator passed to
 *  the constructor. The seed is read from the fountain_packets.
 */
class block_decoder {
  typedef lazy_xor<packet> mp_symbol_t;
  typedef mp::mp_context<mp_symbol_t> mp_ctx_t;
  typedef std::set<fountain_packet, seqno_less> received_t;
  typedef std::vector<lt_row_generator::row_type> link_cache_t;
  typedef std::vector<packet> decoded_t;

  typedef boost::transform_iterator<lazy2p_conv,
				    mp_ctx_t::input_symbols_iterator
				    > mp_packet_iter;

public:
  typedef decoded_t::const_iterator const_partial_iterator;
  typedef utils::skip_false_iterator<decoded_t::const_iterator
				     > const_block_iterator;

  typedef lt_row_generator::rng_type::result_type seed_t;

  explicit block_decoder(const lt_row_generator &rg);

  /** Reset the decoder to the initial state. */
  void reset();
  /** Add an encoded packet to the current block.
   * If the packet is a duplicate (same sequence_number), it is
   * ignored and the method returns false.
   */
  bool push(fountain_packet &&p);
  /** \sa push(fountain_packet&&) */
  bool push(const fountain_packet &p);

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

  /** Return true when the decoder has decoded a full block. */
  explicit operator bool() const;
  /** Return true when the encoder does not have decoded a full block. */
  bool operator!() const;

private:
  log::default_logger basic_lg, perf_lg;

  lt_row_generator rowgen;
  received_t received_pkts;
  link_cache_t link_cache;
  decoded_t decoded;
  std::size_t decoded_count_;
  std::size_t blockno;
  std::size_t pktsize;

  stat::average_counter avg_mp; /**< Average time to run the message
				 *   passing algorithm.
				 */

  void check_correct_block(const fountain_packet &p);
  void run_message_passing();
};

}

#endif
