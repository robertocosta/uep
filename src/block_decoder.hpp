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

/** Iterator adaptor that converts from lazy_xors to packets. */
template <class BaseIter>
using lazy2p_iterator = boost::transform_iterator<lazy2p_conv, BaseIter>;

/** Class to decode a single LT-encoded block of packets.
 *  The LT-code parameters are given by the lt_row_generator passed to
 *  the constructor. The seed is read from the fountain_packets.
 */
class block_decoder {
private:
  /** Type of the underlying message passing context. */
  typedef mp::mp_context<lazy_xor<packet>> mp_ctx_t;
  /** Type of the container for the unique received packets. */
  typedef std::set<fountain_packet, seqno_less> received_t;
  /** Type of the container used to cache the row generator output. */
  typedef std::vector<lt_row_generator::row_type> link_cache_t;

public:
  /** Iterator over the input packets, either decoded or empty. */
  typedef lazy2p_iterator<mp_ctx_t::inputs_iterator> const_partial_iterator;
  /** Iterator over the decoded input packets. */
  typedef lazy2p_iterator<mp_ctx_t::decoded_iterator> const_block_iterator;
  /** Type of the seed used by the row generator. */
  typedef lt_row_generator::rng_type::result_type seed_t;

  /** Construct with the given row generator. */
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
  mp_ctx_t mp_ctx;
  std::size_t blockno;
  std::size_t pktsize;

  stat::average_counter avg_mp; /**< Average time to run the message
				 *   passing algorithm.
				 */

  /** Check the blocno, seqno and seed of the packet and raise an
   *  exception if they don't match the current block.
   */
  void check_correct_block(const fountain_packet &p);
  /** Run the message passing algortihm over the currently received
   *  packets.
   */
  void run_message_passing();
};

}

#endif
