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
  static constexpr std::size_t MAX_BLOCKNO = 0xffff;
  /** Maximum forward distance for a block to be considered more recent. */
  static constexpr std::size_t BLOCK_WINDOW = MAX_BLOCKNO / 2;

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

  /** Pass many received packets at once. The packets can be reordered
   *  before being decoded. To move from the packets the iterators can
   *  be wrapped in std::move_iterator.
   */
  template <class Iter>
  void push(Iter first, Iter last);

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

  /** Push the current incomplete block to the queue and wait for the
   *  next block.
   */
  void flush();

  /** Push the current incomplete block to the queue, assume all
   *  blocks are failed up to the given one and wait for packets
   *  belonging to the given block.
   */
  void flush(std::size_t blockno_);

  /** Push the current incomplete block and `n-1` additional empty
   *  blocks to the queue
   */
  void flush_n_blocks(std::size_t n);

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
  /** Return the total number of packets that were decoded and passed
   *  to the queue. This excludes the packets decoded in the current
   *  block.
   */
  std::size_t total_decoded_count() const;
  /** Return the total number of failed packets that were passed to
   *  the queue. This does not count the undecoded packets in the
   *  current block.
   */
  std::size_t total_failed_count() const;

  /** Return the average time to push a packet. */
  double average_push_time() const;

  /** True if there are decoded packets still in the queue. */
  explicit operator bool() const;
  /** True if all the decoded packets have been extracted. */
  bool operator!() const;

private:
  log::default_logger basic_lg, perf_lg;

  block_decoder the_block_decoder;
  output_block_queue the_output_queue;
  circular_counter<std::size_t> blockno_counter;
  bool has_enqueued; /**< Set to true when the current block has been
			decoded and enqueued in the_output_queue. */

  std::size_t uniq_recv_count; /**< Total number of unique received
				  packets. */
  std::size_t tot_dec_count; /**< Total number of decoded packets. */
  std::size_t tot_failed_count; /**< Total number of packets that were
				 *   not decoded.
				 */
  stat::average_counter avg_push_t; /**< Average time spent processing
				     *	 an incoming packet.
				     */

  /** If the current block was not yet enqueued, then do it even if it
   *  is not fully decoded. The missing packets will be empty.
   */
  void enqueue_partially_decoded();

  /** Used to push incomplete or empty blocks to the queue. This
   *  requires the target blockno to be within the comparison
   *  window. After a call the decoder will expect packets with
   *  blocknumber `blockno_`.
   *  \sa flush
   */
  void flush_small_blockno(std::size_t blockno_);
};

//		   lt_decoder template definitions

template <class Iter>
void lt_decoder::push(Iter first, Iter last) {
  using namespace std::chrono;

  auto tic = high_resolution_clock::now();

  // Sort the packets by blockno
  std::vector<fountain_packet> pkts;
  for (Iter i = first; i != last; ++i) {
    pkts.push_back(*i);
  }
  std::stable_sort(pkts.begin(), pkts.end(),
		   [](const fountain_packet &lhs, const fountain_packet &rhs){
		     return lhs.block_number() < rhs.block_number();
		   });

  // Push to the block decoder block-by-block
  auto i = std::make_move_iterator(pkts.begin());
  auto end = std::make_move_iterator(pkts.end());
  while (i != end) {
    auto bn = i.base()->block_number();
    auto next_b = std::find_if_not(i.base(), end.base(),
				 [bn](const fountain_packet &p){
				   return p.block_number() == bn;
				 });
    auto next = std::make_move_iterator(next_b);

    // Handle different block number
    if (blockno_counter.last() != static_cast<std::size_t>(bn)) {
      auto recv_blockno(blockno_counter);
      recv_blockno.set(bn);
      if (recv_blockno.is_after(blockno_counter)) {
	BOOST_LOG(perf_lg) << "lt_decoder::push new_block blockno="
			   << bn;
	flush_small_blockno(bn); // Then push normally
      }
      else {
	BOOST_LOG(perf_lg) << "lt_decoder::push old_block blockno="
			   << bn;
	// This is not a new block number: do nothing
	i = next;
	continue;
      }
    }

    std::size_t pushed = the_block_decoder.push(i, next);
    uniq_recv_count += pushed;
    if (pushed != static_cast<std::size_t>(next - i))
      BOOST_LOG(perf_lg) << "lt_decoder::push duplicate_pkts blockno="
			 << bn;

    // Extract if fully decoded block (just once)
    if (the_block_decoder) {
      enqueue_partially_decoded();
    }

    i = next;
  }

  duration<double> push_tdiff = high_resolution_clock::now() - tic;
  BOOST_LOG(perf_lg) << "lt_decoder::push push_time="
		     << push_tdiff.count();
  avg_push_t.add_sample(push_tdiff.count());
}

}

#endif
