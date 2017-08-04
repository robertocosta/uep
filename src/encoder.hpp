#ifndef UEP_ENCODER_HPP
#define UEP_ENCODER_HPP

#include <limits>
#include <stdexcept>
#include <utility>

#include "block_encoder.hpp"
#include "block_queues.hpp"
#include "counter.hpp"
#include "log.hpp"
#include "lt_param_set.hpp"
#include "packets.hpp"
#include "rng.hpp"
#include "utils.hpp"

namespace uep {

/** Standard LT-code encoder.
 *  The encoder receives input packets via push_input, stores them in
 *  a FIFO queue and produces one encoded packet at a time via
 *  next_coded. The packets are XORed according to the rows generated
 *  by an lt_row_generator. The seed for the row generator is produced
 *  at each new block of K packets by an object of class Gen.
 *  \sa fountain_decoder fountain
 */
template<class Gen = std::random_device>
class lt_encoder {
public:
  /** The collection of parameters required to setup the encoder. */
  typedef robust_lt_parameter_set parameter_set;
  /** The type of the object called to seed the row generator at each
   *  new block.
   */
  typedef Gen seed_generator_type;
  typedef input_block_queue::const_block_iterator const_block_iterator;

  /** Maximum allowed value for the sequence numbers.
   *  If the encoder tries to generate more packets from a single
   *  input block, an exception is thrown.
   */
  static const std::size_t MAX_SEQNO = 0xffff;
  /** Maximum allowed value for the block numbers.
   *  When the encoder goes past this value it loop back to zero.
   */
  static const std::size_t MAX_BLOCKNO = 0xffff;
  /** Maximum forward distance for a block number to be considered
   *  more recent.
   */
  static const std::size_t BLOCK_WINDOW = MAX_BLOCKNO / 2;

  // Check that the types used for block numbers and sequence numbers
  // are big enough.
  static_assert(std::numeric_limits<
		decltype(fountain_packet().sequence_number())
		>::max() >= MAX_SEQNO,
		"the seqno type is too small");
  static_assert(std::numeric_limits<
		decltype(fountain_packet().block_number())
		>::max() >= MAX_BLOCKNO,
		"the blockno type is too small");

  /** Construct using the given parameter set. */
  explicit lt_encoder(const parameter_set &ps) :
    lt_encoder(ps.K, ps.c, ps.delta) {}

  /** Construct using a robust_soliton_distribution with parameters K,
   *  c, delta.
   */
  explicit lt_encoder(std::size_t K, double c, double delta) :
    lt_encoder(make_robust_lt_row_generator(K,c,delta)) {}

  /** Construct using an lt_row_generator with degree distribution distr. */
  explicit lt_encoder(const degree_distribution &distr) :
    lt_encoder(lt_row_generator(distr)) {}

  /** Construct with the row_generator rg. */
  explicit lt_encoder(const lt_row_generator &rg) :
    basic_lg(boost::log::keywords::channel = log::basic),
    perf_lg(boost::log::keywords::channel = log::performance),
    the_input_queue(rg.K()),
    the_block_encoder(rg),
    seqno_counter(MAX_SEQNO),
    blockno_counter(MAX_BLOCKNO),
    tot_coded_count(0) {
    blockno_counter.set(0);
  }

  /** Enqueue packet p in the input queue. */
  void push(const packet &p) {
    using std::move;
    packet p_copy(p);
    push(move(p_copy));
  }

  /** Enqueue packet p in the input queue. */
  void push(packet &&p) {
    using std::move;
    the_input_queue.push(move(p));
    //PUT_STAT_COUNTER(loggers.in_pkts);
    //BOOST_LOG_SEV(loggers.text, debug) << "Pushed a packet to the encoder";
    check_has_block();
  }

  /** Generate the next coded packet from the current block. */
  fountain_packet next_coded() {
    fountain_packet p(the_block_encoder.next_coded());
    p.sequence_number(seqno_counter.next());
    p.block_number(blockno_counter.last());
    p.block_seed(the_block_encoder.seed());
    //PUT_STAT_COUNTER(loggers.enc_pkts);
    //BOOST_LOG_SEV(loggers.text, debug) << "New encoded packet: " << p;
    return p;
  }

  /** Drop the current block of packets and prepare to encode the next
   *  one.
   */
  void next_block() {
    BOOST_LOG(perf_lg) << "encoder::next_block coded_pkts="
		       << coded_count();
    tot_coded_count += coded_count();
    the_input_queue.pop_block();
    the_block_encoder.reset();
    blockno_counter.next();
    seqno_counter.reset();
    BOOST_LOG_SEV(basic_lg, log::debug) << "Encoder skipped to the next block: "
					<< blockno_counter.value();
    check_has_block();
  }

  /** Drop all the blocks up to the given block number.
   *  This method throws an exception if there are not enough queued
   *  packets to skip.
   */
  void next_block(std::size_t blockno_) {
    decltype(blockno_counter) wanted_blockno(MAX_BLOCKNO);
    wanted_blockno.set(blockno_);
    std::size_t dist = blockno_counter.forward_distance(wanted_blockno);
    if (dist == 0 || dist > BLOCK_WINDOW) return;

    // Drop dist-1 pkts from the queue (this can fail)
    for (std::size_t i = 0; i < dist-1; ++i) {
      the_input_queue.pop_block();
      blockno_counter.next();
    }

    // Last skip
    next_block();
  }

  /** Return true when the encoder has been passed at least K packets
   *  and is ready to produce a coded packet.
   */
  bool has_block() const { return the_block_encoder.can_encode(); }
  /** The block size. */
  std::size_t K() const { return the_block_encoder.block_size(); }
  /** The sequence number of the current block. */
  std::size_t blockno() const { return blockno_counter.last(); }
  /** The sequence number of the last generated packet. */
  std::size_t seqno() const { return seqno_counter.last(); }
  /** The seed used in the current block. */
  int block_seed() const { return the_block_encoder.seed(); }
  /** The number of queued packets, excluding the current block. */
  std::size_t queue_size() const { return the_input_queue.queue_size(); }
  /** Total number of packets held by the encoder. */
  std::size_t size() const { return the_input_queue.size(); }

  /** Return a copy of the lt_row_generator used. */
  lt_row_generator row_generator() const {
    return the_block_encoder.row_generator();
  }
  /** Return a copy of the RNG used to produce the block seeds. */
  seed_generator_type seed_generator() const { return the_seed_gen; }

  /** Return an iterator to the start of the current block. */
  const_block_iterator current_block_begin() const {
    return the_input_queue.block_begin();
  }
  /** Return an iterator to the end of the current block. */
  const_block_iterator current_block_end() const {
    return the_input_queue.block_end();
  }

  /** Return the number of coded packets that were produced for the
   *  current block.
   */
  std::size_t coded_count() const {
    return the_block_encoder.output_count();
  }

  /** Return the total number of coded packets that were produced. */
  std::size_t total_coded_count() const {
    return tot_coded_count;
  }

  /** Is true when coded packets can be produced. */
  explicit operator bool() const { return has_block(); }
  /** Is true when there is not a full block available. */
  bool operator!() const { return !has_block(); }

private:
  log::default_logger basic_lg, perf_lg;

  input_block_queue the_input_queue;
  block_encoder the_block_encoder;
  seed_generator_type the_seed_gen;
  counter<std::size_t> seqno_counter;
  circular_counter<std::size_t> blockno_counter;
  std::size_t tot_coded_count; /**< Count the total number of coded
				*   packets.
				*/

  /** If the block_encoder is empty and the queue has a full block,
   *  load the block_decoder. Also generate a new block seed.
   */
  void check_has_block() {
    if (the_input_queue && !the_block_encoder) {
      the_block_encoder.set_block_shallow(the_input_queue.block_begin(),
					  the_input_queue.block_end());
      the_block_encoder.set_seed(the_seed_gen());

      BOOST_LOG_SEV(basic_lg, log::trace) << "The encoder has a new block";
    }
  }
};

}

#endif
