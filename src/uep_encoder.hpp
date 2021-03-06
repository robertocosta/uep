#ifndef UEP_UEP_ENCODER_HPP
#define UEP_UEP_ENCODER_HPP

#include "encoder.hpp"
#include "lt_param_set.hpp"

namespace uep {

/** Unequal error protection encoder.
 *  This class wraps an lt_encoder and passes it the expanded
 *  blocks. The pushed packets are stored in multiple queues,
 *  mantaining the order between packets with the same pariority. They
 *  are used to build expanded blocks according to the Ks, RFs and EF
 *  parameters.
 */
template <class Gen = std::random_device>
class uep_encoder {
  typedef block_queue<uep_packet> queue_type;

public:
  /** The collection of parameters required to setup the encoder. */
  typedef lt_uep_parameter_set parameter_set;
  /** The type of the object called to seed the row generator at each
   *  new block.
   */
  typedef Gen seed_generator_type;
  /** Iterator over the current block. */
  //typedef typename lt_encoder<Gen>::const_block_iterator const_block_iterator;

  static constexpr std::size_t BLOCK_WINDOW = lt_encoder<Gen>::BLOCK_WINDOW;
  static constexpr std::size_t MAX_BLOCKNO = lt_encoder<Gen>::MAX_BLOCKNO;
  static constexpr std::size_t MAX_SEQNO = uep_packet::MAX_SEQNO;

  /** Construct using the given parameter set. */
  explicit uep_encoder(const parameter_set &ps);
  /** Construct using the given sub-block sizes, repetition factors,
   *  expansion factor, c and delta.
   */
  template<typename KsIter, typename RFsIter>
  explicit uep_encoder(KsIter ks_begin, KsIter ks_end,
		       RFsIter rfs_begin, RFsIter rfs_end,
		       std::size_t ef,
		       double c,
		       double delta);

  /** Enqueue a packet according to its priority level. */
  void push(fountain_packet &&p);
  /** Enqueue a packet according to its priority level. */
  void push(const fountain_packet &p);
  /** Enqueue a packet with default priority 0. */
  void push (packet &&p);
  /** Enqueue a packet with default priority 0. */
  void push (const packet &p);

  /** Generate the next coded packet from the current block. */
  fountain_packet next_coded();

  /** Fill a partial block with padding packets. This allows to encode
   *  even if there are no more source packets to be passed.
   */
  void pad_partial_block();

  /** Drop the current block of packets and prepare to encode the next
   *  one.
   */
  void next_block();
  /** Drop all the blocks up to the given block number.
   *  This method throws an exception if there are not enough queued
   *  packets to skip.
   */
  void next_block(std::size_t bn);

  /** Return true when the encoder has been passed at least K packets
   *  and is ready to produce a coded packet.
   */
  bool has_block() const;
  /** The block size used by the underlying standard LT encoder. */
  std::size_t block_size_out() const;
  /** The block size used when considering the input packets. */
  std::size_t block_size() const;
  /** Alias of block_size. */
  std::size_t block_size_in() const;
  /** Alias of block_size. */
  std::size_t K() const;
  /** Constant reference to the array of blocksizes. */
  const std::vector<std::size_t> &block_sizes() const;
  /** The sequence number of the current block. */
  std::size_t blockno() const;
  /** Return a copy of the current block number counter. */
  circular_counter<std::size_t> block_number_counter() const;
  /** The sequence number of the last generated packet. */
  std::size_t seqno() const;
  /** The seed used in the current block. */
  int block_seed() const;
  /** The number of queued packets, excluding the current block. */
  std::size_t queue_size() const;
  /** Total number of (input) packets held by the encoder. */
  std::size_t size() const;

  /** Return the lt_row_generator used. */
  const uep_row_generator &row_generator() const;
  /** Return a copy of the RNG used to produce the block seeds. */
  seed_generator_type seed_generator() const;

  /** Return an iterator to the start of the current block. */
  //const_block_iterator current_block_begin() const;
  /** Return an iterator to the end of the current block. */
  //const_block_iterator current_block_end() const;

  /** Return the number of coded packets that were produced for the
   *  current block.
   */
  std::size_t coded_count() const;

  /** Return the total number of coded packets that were produced. */
  std::size_t total_coded_count() const;

  /** Number of padding packets added to the current block. */
  std::size_t padding_count() const;
  /** Total number of padding packets added to all blocks. */
  std::size_t total_padding_count() const;

  /** Is true when coded packets can be produced. */
  explicit operator bool() const;
  /** Is true when there is not a full block available. */
  bool operator!() const;

  /** Return the average time to extract a new packet up to now. */
  double average_encoding_time() const {
    return _enc_time_avg.value();
  }

private:
  log::default_logger basic_lg, perf_lg;

  std::vector<queue_type> inp_queues; /**< The queues that
				       *   store the packets
				       *   belonging to
				       *   different priority
				       *   classes.
				       */
  circular_counter<> seqno_ctr; /**< Circular counter for the UEP
				 *   sequence numbers.
				 */
  std::unique_ptr<lt_encoder<Gen>> std_enc; /**< The standard LT
					     *	 encoder. Use a
					     *	 pointer to allow
					     *	 non-movable seed
					     *	 generators.
					     */

  std::size_t pktsize; /**< The size of the pushed packets. */
  stat::sum_counter<std::size_t> padding_cnt; /**< Number of padding
					       *   packets.
					       */
  stat::average_counter _enc_time_avg; /**< Record the average
					*   encoding time.
					*/

  /** Check whether the queues have enough packets to build a block. */
  void check_has_block();
};

//		uep_decoder<Gen> template definitions

template <class Gen>
template<typename KsIter, typename RFsIter>
uep_encoder<Gen>::uep_encoder(KsIter ks_begin, KsIter ks_end,
			      RFsIter rfs_begin, RFsIter rfs_end,
			      std::size_t ef,
			      double c,
			      double delta) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  seqno_ctr(std::numeric_limits<uep_packet::seqno_type>::max()),
  pktsize(0) {
  auto uep_rowgen = std::make_unique<uep_row_generator>(ks_begin, ks_end,
							rfs_begin, rfs_end,
							ef,
							c,
							delta);
  const auto &Ks = uep_rowgen->Ks();
  inp_queues.reserve(Ks.size());
  for (std::size_t Ki : Ks) {
    inp_queues.emplace_back(Ki);
  }

  seqno_ctr.set(0);

  std_enc = std::make_unique<lt_encoder<Gen>>(std::move(uep_rowgen));
}

template<typename Gen>
uep_encoder<Gen>::uep_encoder(const parameter_set &ps) :
  uep_encoder(ps.Ks.begin(), ps.Ks.end(),
	      ps.RFs.begin(), ps.RFs.end(),
	      ps.EF,
	      ps.c,
	      ps.delta) {
}

template <class Gen>
void uep_encoder<Gen>::push(const fountain_packet &p) {
  using std::move;
  fountain_packet p_copy(p);
  push(move(p_copy));
}

template <class Gen>
void uep_encoder<Gen>::push(fountain_packet &&p) {
  if (pktsize == 0) pktsize = p.buffer().size();
  else if (pktsize != p.buffer().size()) {
    throw std::invalid_argument("The packets must have the same size");
  }

  uep_packet up(std::move(p.buffer()));
  up.priority(p.getPriority());
  up.sequence_number(seqno_ctr.value());
  BOOST_LOG(perf_lg) << "uep_encoder::push new_packet"
		     << " orig_size=" << up.buffer().size()
		     << " priority=" << up.priority()
		     << " seqno=" << up.sequence_number();

  if (inp_queues.size() <= up.priority())
    throw std::runtime_error("Priority is out of range");
  inp_queues[up.priority()].push(std::move(up));
  seqno_ctr.next();

  check_has_block();
}

template <class Gen>
void uep_encoder<Gen>::push(packet &&p) {
  fountain_packet fp(std::move(p));
  fp.setPriority(0);
  push(std::move(p));
}

template <class Gen>
void uep_encoder<Gen>::push(const packet &p) {
  packet p_copy(p);
  push(std::move(p));
}

template <class Gen>
fountain_packet uep_encoder<Gen>::next_coded() {
  using namespace std::chrono;
  auto t = high_resolution_clock::now();
  fountain_packet coded_p = std_enc->next_coded();
  duration<double> tdiff = high_resolution_clock::now() - t;
  _enc_time_avg.add_sample(tdiff.count());
  return coded_p;
}

template<typename Gen>
void uep_encoder<Gen>::pad_partial_block() {
  if (has_block()) return;

  std::size_t pad_cnt = 0;
  for (queue_type &q : inp_queues) {
    while (!q.has_block()) {
      // Don't give the padding pkts a seqno
      q.push(uep_packet::make_padding(pktsize));
      ++pad_cnt;
    }
  }
  padding_cnt.add_sample(pad_cnt);

  BOOST_LOG(perf_lg) << "uep_encoder::pad_partial_block"
		     << " pad_cnt=" << pad_cnt;

  check_has_block();
}

template <class Gen>
void uep_encoder<Gen>::next_block() {
  std_enc->next_block();
  padding_cnt.clear_last();
  check_has_block();
}

template <class Gen>
void uep_encoder<Gen>::next_block(std::size_t bn) {
  auto curr_bnc = std_enc->block_number_counter();
  auto wanted_bnc(curr_bnc);
  wanted_bnc.set(bn);
  // Ignore if not in the comp. window
  if (!wanted_bnc.is_after(curr_bnc)) return;
  std::size_t dist = curr_bnc.forward_distance(wanted_bnc);

  // Drop `dist-1` blocks from the queues
  for (std::size_t i = 0; i < dist - 1; ++i) {
    for (queue_type &iq : inp_queues) {
      iq.pop_block();
    }
  }

  // Push `dist-1` empty blocks
  for (std::size_t i = 0; i < K() * (dist - 1); ++i) {
    std_enc->push(packet());
  }

  std_enc->next_block(bn);
  padding_cnt.clear_last();
  check_has_block();
}

template <class Gen>
bool uep_encoder<Gen>::has_block() const {
  return std_enc->has_block();
}

template <class Gen>
std::size_t uep_encoder<Gen>::block_size_out() const {
  return row_generator().K_out();
}

template <class Gen>
std::size_t uep_encoder<Gen>::block_size() const {
  return row_generator().K_in();
}

template <class Gen>
std::size_t uep_encoder<Gen>::block_size_in() const {
  return block_size();
}

template <class Gen>
std::size_t uep_encoder<Gen>::K() const {
  return block_size();
}

template <class Gen>
const std::vector<std::size_t> &uep_encoder<Gen>::block_sizes() const {
  return row_generator().Ks();
}

template <class Gen>
std::size_t uep_encoder<Gen>::blockno() const {
  return std_enc->blockno();
}

template <class Gen>
circular_counter<std::size_t> uep_encoder<Gen>::block_number_counter() const {
  return std_enc->block_number_counter();
}

template <class Gen>
std::size_t uep_encoder<Gen>::seqno() const {
  return std_enc->seqno();
}

template <class Gen>
int uep_encoder<Gen>::block_seed() const {
  return std_enc->block_seed();
}

template <class Gen>
std::size_t uep_encoder<Gen>::queue_size() const {
  return std::accumulate(inp_queues.cbegin(),
			 inp_queues.cend(),
			 0,
			 [](std::size_t sum,
			    const queue_type &iq) -> std::size_t {
			   return sum + iq.size();
			 });
}

template <class Gen>
std::size_t uep_encoder<Gen>::size() const {
  return queue_size() + std_enc->size();
}

template <class Gen>
const uep_row_generator &uep_encoder<Gen>::row_generator() const {
  return static_cast<const uep_row_generator&>(std_enc->row_generator());
}

template <class Gen>
typename uep_encoder<Gen>::seed_generator_type
uep_encoder<Gen>::seed_generator() const {
  return std_enc->seed_generator();
}

// template <class Gen>
// typename uep_encoder<Gen>::const_block_iterator
// uep_encoder<Gen>::current_block_begin() const {
//   return std_enc->current_block_begin();
// }

// template <class Gen>
// typename uep_encoder<Gen>::const_block_iterator
// uep_encoder<Gen>::current_block_end() const {
//   return std_enc->current_block_end();
// }

template <class Gen>
std::size_t uep_encoder<Gen>::coded_count() const {
  return std_enc->coded_count();
}

template <class Gen>
std::size_t uep_encoder<Gen>::total_coded_count() const {
  return std_enc->total_coded_count();
}

template <class Gen>
uep_encoder<Gen>::operator bool() const {
  return has_block();
}

template <class Gen>
bool uep_encoder<Gen>::operator!() const {
  return !has_block();
}

template <class Gen>
void uep_encoder<Gen>::check_has_block() {
  if (std_enc->has_block()) return; // Already has a block
  if (std::any_of(inp_queues.cbegin(),
		  inp_queues.cend(),
		  [](const queue_type &q){
		    return !q.has_block();
		  })) {
    return; // Not all sub-blocks are available
  }

  // Count the non-padding packets for each sub-block
  std::vector<std::size_t> pkt_counts(inp_queues.size(), 0);

  // Iterate over all queues
  for (std::size_t i = 0; i < inp_queues.size(); ++i) {
    queue_type &q = inp_queues[i];

    // Convert the sub-block to packets
    for (auto l = q.block_begin(); l != q.block_end(); ++l) {
      const uep_packet &p = *l;
      if (!p.padding()) ++pkt_counts[i];
      std_enc->push(p.to_packet());
    }
    q.pop_block();
  }

  BOOST_LOG(perf_lg) << "uep_encoder::check_has_block"
		     << " new_block"
		     << " non_padding_pkts=" << pkt_counts;
}

template<typename Gen>
std::size_t uep_encoder<Gen>::padding_count() const {
  return padding_cnt.last_sample(0);
}

template<typename Gen>
std::size_t uep_encoder<Gen>::total_padding_count() const {
  return padding_cnt.value();
}

}

#endif
