#ifndef UEP_ENCODER_HPP
#define UEP_ENCODER_HPP

#include <limits>
#include <stdexcept>
#include <utility>

#include "block_encoder.hpp"
#include "block_queues.hpp"
#include "log.hpp"
#include "packets.hpp"
#include "rng.hpp"
#include "counter.hpp"
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

  static_assert(std::numeric_limits<
		decltype(fountain_packet().sequence_number())
		>::max() >= MAX_SEQNO,
		"the seqno type is too small");
  static_assert(std::numeric_limits<
		decltype(fountain_packet().block_number())
		>::max() >= MAX_BLOCKNO,
		"the blockno type is too small");
  
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
    the_input_queue(rg.K()),
    the_block_encoder(rg),
    seqno_counter(MAX_SEQNO),
    blockno_counter(MAX_BLOCKNO) {
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
    the_block_encoder.reset();
    the_input_queue.pop_block();
    blockno_counter.next();
    seqno_counter.reset();
    //PUT_STAT_COUNTER(loggers.newblock);
    check_has_block();
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
  /** The number of queued packets. */
  std::size_t queue_size() const { return the_input_queue.queue_size(); }
  /** Total number of packets held by the encoder. */
  std::size_t size() const { return the_input_queue.size(); }

  lt_row_generator row_generator() const {
    return the_block_encoder.row_generator();
  }
  
  seed_generator_type seed_generator() const { return the_seed_gen; }
  const_block_iterator current_block_begin() const {
     return the_input_queue.block_begin();
  }
  const_block_iterator current_block_end() const {
    return the_input_queue.block_end();
  }

private:
    input_block_queue the_input_queue;
    block_encoder the_block_encoder;
    seed_generator_type the_seed_gen;
    counter<std::size_t> seqno_counter;
    circular_counter<std::size_t> blockno_counter;


    // struct loggers_t {
  //   default_logger enc_pkts = make_stat_logger("EncoderCodedPackets", counter);
  //   default_logger in_pkts = make_stat_logger("EncoderInputPackets", counter);
  //   default_logger newblock = make_stat_logger("EncoderNewBlock", counter);
  //   default_logger rows = make_stat_logger("EncoderRowDegree", scalar);
  //   default_logger text;
  // } loggers;

  void check_has_block() {
    if (the_input_queue && !the_block_encoder) {
      the_block_encoder.set_block_shallow(the_input_queue.block_begin(),
					  the_input_queue.block_end());
      the_block_encoder.set_seed(the_seed_gen());
    }
  }
};

}

#endif
