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
 */
class lt_decoder {
public:
  typedef robust_lt_parameter_set parameter_set;
  typedef block_decoder::const_block_iterator const_block_iterator;

  /** Maximum allowed value for the block numbers.
   *  The decoder expects that it loops back to zero after this value.
   */
  static const std::size_t MAX_BLOCKNO = 0xffff;
  /** Maximum forward distance to not consider a block from the past. */
  static const std::size_t BLOCK_WINDOW = MAX_BLOCKNO / 2;

  explicit lt_decoder(const parameter_set &ps);
  explicit lt_decoder(std::size_t K, double c, double delta);
  explicit lt_decoder(const degree_distribution &distr);
  explicit lt_decoder(const lt_row_generator &rg);

  void push(const fountain_packet &p);
  void push(fountain_packet &&p);

  packet next_decoded();

  /** Const iterator pointing to the start of the last decoded block. */
  const_block_iterator decoded_begin() const;
  /** Const iterator pointing to the end of the last decoded block. */
  const_block_iterator decoded_end() const;

  /** Return true if the current block has been decoded. */
  bool has_decoded() const;
  /** Return the block size. */
  std::size_t K() const;
  /** Return the current block number. */
  std::size_t blockno() const;
  /** Return the current block seed. */
  int block_seed() const;
  /** Number of unique packets received for the current block. */
  std::size_t received_count() const;
  /** Number of packets decoded for the current block. */
  std::size_t decoded_count() const;
  /** Number of output queued packets. */
  std::size_t queue_size() const;

private:
  block_decoder the_block_decoder;
  output_block_queue the_output_queue;
  circular_counter<std::size_t> blockno_counter;
  bool has_enqueued;

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
