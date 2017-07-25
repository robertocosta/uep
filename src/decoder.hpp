#ifndef UEP_DECODER_HPP
#define UEP_DECODER_HPP

#include "block_decoder.hpp"
#include "block_queues.hpp"
#include "counter.hpp"
#include "log.hpp"
#include "lt_param_set.hpp"
#include "packets.hpp"
#include "rng.hpp"
#include "utils.hpp"

namespace uep {

/** Standard LT-code decoder.
 *  The decoder passes packets received through push() to a
 *  block_decoder, until a full block can be decoded or the current
 *  block is manually discarded. The decoded packets are buffered in a
 *  FIFO queue and can be extraced one by one using next_decoded, or
 *  using the iterator pair for the last decoded block.
 */
class lt_decoder {
public:
  /** The collection of parameters required to setup the decoder. */
  typedef robust_lt_parameter_set parameter_set;
  typedef block_decoder::const_block_iterator const_block_iterator;

  /** Maximum allowed value for the block numbers.
   *  The decoder expects that it loops back to zero after this value.
   */
  static const std::size_t MAX_BLOCKNO = 0xffff;
  /** Maximum forward distance for a block to be considered more recent. */
  static const std::size_t BLOCK_WINDOW = MAX_BLOCKNO / 2;

  /** Construct using the given parameter set. */
  explicit lt_decoder(const parameter_set &ps);
  /** Construct using a robust_soliton_distribution with the given paramters. */
  explicit lt_decoder(std::size_t K, double c, double delta);
  /** Construct using the given degree_distribution. */
  explicit lt_decoder(const degree_distribution &distr);
  /** Construct using the given row generator. */
  explicit lt_decoder(const lt_row_generator &rg);

  /** Pass a received packet. \sa push(fountain_packet&&) */
  void push(const fountain_packet &p);
  /** Pass a received packet.
   *  This method causes the message-passing algorithm to be run on
   *  the accumulated set of packets, unless the current block is
   *  already fully decoded. Duplicate and old packets are silently
   *  discarded.  The decoder switches to a new block if a more recent
   *  block number is received.
   */
  void push(fountain_packet &&p);

  /** Extract the oldest decoded packet from the FIFO queue. */
  packet next_decoded();

  /** Const iterator pointing to the start of the last decoded block.
   *  This can become invalid after a call to push().
   */
  const_block_iterator decoded_begin() const;
  /** Const iterator pointing to the end of the last decoded block.
   *  This can become invalid after a call to push().
   */
  const_block_iterator decoded_end() const;

  /** Return true if the current block has been decoded. */
  bool has_decoded() const;
  /** Return the block size. */
  std::size_t K() const;
  /** Return the current block number. */
  std::size_t blockno() const;
  /** Return a copy of the current block number counter. */
  circular_counter<std::size_t> block_number_counter() const;
  /** Return the current block seed. */
  int block_seed() const;
  /** Number of unique packets received for the current block. */
  std::size_t received_count() const;
  /** Number of packets decoded for the current block. */
  std::size_t decoded_count() const;
  /** Number of output queued packets. */
  std::size_t queue_size() const;
  /** True if there are decoded packets still in the queue. */
  bool has_queued_packets() const;
  /** Return the total number of unique received packets. */
  std::size_t total_received_count() const;
  /** Return the total number of decoded packets. */
  std::size_t total_decoded_count() const;

  /** True if there are decoded packets still in the queue. */
  explicit operator bool() const;
  /** True if all the decoded packets have been extracted. */
  bool operator!() const;

private:
  block_decoder the_block_decoder;
  output_block_queue the_output_queue;
  circular_counter<std::size_t> blockno_counter;
  bool has_enqueued; /**< Set to true when the current block has been
			decoded and enqueued in the_output_queue. */

  std::size_t uniq_recv_count; /**< Total number of unique received
				  packets. */
  std::size_t tot_dec_count; /**< Total number of decoded packets. */

  // struct loggers_t {
  //   default_logger dec_blocks = make_stat_logger("DecoderDecodedBlocks", counter);
  //   default_logger decodeable = make_stat_logger("DecoderDecodeablePackets", scalar);
  //   default_logger dup_pkts = make_stat_logger("DecoderDuplicatePackets", counter);
  //   default_logger in_pkts = make_stat_logger("DecoderInputPackets", counter);
  //   default_logger newblock = make_stat_logger("DecoderNewBlock", counter);
  //   default_logger old_dropped = make_stat_logger("DecoderOldDropped", counter);
  //   default_logger recv_size = make_stat_logger("DecoderTryWithReceived", scalar);
  //   default_logger rows = make_stat_logger("DecoderRowDegree", scalar);
  //   default_logger text;
  // } loggers;
};

}

#endif
