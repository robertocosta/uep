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
public:
  /** The collection of parameters required to setup the encoder. */
  typedef lt_uep_parameter_set parameter_set;
  /** The type of the object called to seed the row generator at each
   *  new block.
   */
  typedef Gen seed_generator_type;
  /** Iterator over the current block. */
  typedef typename lt_encoder<Gen>::const_block_iterator const_block_iterator;

  static constexpr std::size_t MAX_SEQNO = lt_encoder<Gen>::MAX_SEQNO;
  static constexpr std::size_t MAX_BLOCKNO = lt_encoder<Gen>::MAX_BLOCKNO;
  static constexpr std::size_t BLOCK_WINDOW = lt_encoder<Gen>::BLOCK_WINDOW;

  /** Construct using the given parameter set. */
  explicit uep_encoder(const parameter_set &ps);

  /** Enqueue a packet according to its priority level. */
  void push(fountain_packet &&p);
  /** Enqueue a packet according to its priority level. */
  void push(const fountain_packet &p);

  /** Generate the next coded packet from the current block. */
  fountain_packet next_coded();

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
  /** The block size used to pack the input packets. */
  std::size_t block_size_in() const;
  /** Alias of block_size_in(). */
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

  /** Return a copy of the lt_row_generator used. */
  lt_row_generator row_generator() const;
  /** Return a copy of the RNG used to produce the block seeds. */
  seed_generator_type seed_generator() const;

  /** Return an iterator to the start of the current block. */
  const_block_iterator current_block_begin() const;
  /** Return an iterator to the end of the current block. */
  const_block_iterator current_block_end() const;

  /** Return the number of coded packets that were produced for the
   *  current block.
   */
  std::size_t coded_count() const;

  /** Return the total number of coded packets that were produced. */
  std::size_t total_coded_count() const;

  /** Is true when coded packets can be produced. */
  explicit operator bool() const;
  /** Is true when there is not a full block available. */
  bool operator!() const;

private:
  log::default_logger basic_lg, perf_lg;

  std::vector<std::size_t> Ks; /**< Array of sub-block sizes. */
  std::vector<std::size_t> RFs; /**< Array of repetition factors. */
  std::size_t EF; /**< Expansion factor. */
  std::vector<input_block_queue> inp_queues; /**< The queues that
					      *   store the packets
					      *   belonging to
					      *   different priority
					      *   classes.
					      */
  lt_encoder<Gen> std_enc; /**< The standard LT encoder. */

  /** Check whether the queues have enough packets to build a block. */
  void check_has_block();
};

//		uep_decoder<Gen> template definitions

template <class Gen>
uep_encoder<Gen>::uep_encoder(const parameter_set &ps) :
  basic_lg(boost::log::keywords::channel = log::basic),
  perf_lg(boost::log::keywords::channel = log::performance),
  Ks(ps.Ks), RFs(ps.RFs), EF(ps.EF),
  std_enc(1, ps.c, ps.delta) { // Wait until two-level check
  if (RFs.empty()) { // Using two-level UEP
    RFs = {ps.RFM, ps.RFL};
  }
  if (Ks.size() != RFs.size()) {
    throw std::invalid_argument("Ks, RFs sizes do not match");
  }

  inp_queues.reserve(Ks.size());
  for (std::size_t s : Ks) {
    inp_queues.emplace_back(s);
  }

  std_enc = lt_encoder<Gen>(block_size_out(), ps.c, ps.delta);
}

template <class Gen>
void uep_encoder<Gen>::push(const fountain_packet &p) {
  using std::move;
  fountain_packet p_copy(p);
  push(move(p_copy));
}

template <class Gen>
void uep_encoder<Gen>::push(fountain_packet &&p) {
  std::size_t priority = p.getPriority();
  if (inp_queues.size() <= priority)
    throw std::runtime_error("Priority is out of range");
  inp_queues[priority].push(std::move(p));

  check_has_block();
}

template <class Gen>
fountain_packet uep_encoder<Gen>::next_coded() {
  return std_enc.next_coded();
}

template <class Gen>
void uep_encoder<Gen>::next_block() {
  std_enc.next_block();
  check_has_block();
}

template <class Gen>
void uep_encoder<Gen>::next_block(std::size_t bn) {
  auto curr_bnc = std_enc.block_number_counter();
  auto wanted_bnc(curr_bnc);
  wanted_bnc.set(bn);
  // Ignore if not in the comp. window
  if (!wanted_bnc.is_after(curr_bnc)) return;
  std::size_t dist = curr_bnc.forward_distance(wanted_bnc);

  // Drop `dist-1` blocks from the queues
  for (std::size_t i = 0; i < dist - 1; ++i) {
    for (input_block_queue &iq : inp_queues) {
      iq.pop_block();
    }
  }

  // Push `dist-1` empty blocks
  for (std::size_t i = 0; i < block_size_out() * (dist - 1); ++i) {
    std_enc.push(packet());
  }

  std_enc.next_block(bn);
  check_has_block();
}

template <class Gen>
bool uep_encoder<Gen>::has_block() const {
  return std_enc.has_block();
}

template <class Gen>
std::size_t uep_encoder<Gen>::block_size_out() const {
  return EF * std::inner_product(Ks.cbegin(), Ks.cend(),
				 RFs.cbegin(), 0);
}

template <class Gen>
std::size_t uep_encoder<Gen>::block_size_in() const {
  return std::accumulate(Ks.cbegin(), Ks.cend(), 0);
}

template <class Gen>
std::size_t uep_encoder<Gen>::K() const {
  return block_size_in();
}

template <class Gen>
const std::vector<std::size_t> &uep_encoder<Gen>::block_sizes() const {
  return Ks;
}

template <class Gen>
std::size_t uep_encoder<Gen>::blockno() const {
  return std_enc.blockno();
}

template <class Gen>
circular_counter<std::size_t> uep_encoder<Gen>::block_number_counter() const {
  return std_enc.block_number_counter();
}

template <class Gen>
std::size_t uep_encoder<Gen>::seqno() const {
  return std_enc.seqno();
}

template <class Gen>
int uep_encoder<Gen>::block_seed() const {
  return std_enc.block_seed();
}

template <class Gen>
std::size_t uep_encoder<Gen>::queue_size() const {
  return std::accumulate(inp_queues.cbegin(),
			 inp_queues.cend(),
			 0,
			 [](std::size_t sum,
			    const input_block_queue &iq) -> std::size_t {
			   return sum + iq.size();
			 });
}

template <class Gen>
std::size_t uep_encoder<Gen>::size() const {
  std::size_t enc_size = std_enc.size() / block_size_out() * block_size_in();
  return queue_size() + enc_size;
}

template <class Gen>
lt_row_generator uep_encoder<Gen>::row_generator() const {
  return std_enc.row_generator();
}

template <class Gen>
typename uep_encoder<Gen>::seed_generator_type
uep_encoder<Gen>::seed_generator() const {
  return std_enc.seed_generator();
}

template <class Gen>
typename uep_encoder<Gen>::const_block_iterator
uep_encoder<Gen>::current_block_begin() const {
  return std_enc.current_block_begin();
}

template <class Gen>
typename uep_encoder<Gen>::const_block_iterator
uep_encoder<Gen>::current_block_end() const {
  return std_enc.current_block_end();
}

template <class Gen>
std::size_t uep_encoder<Gen>::coded_count() const {
  return std_enc.coded_count();
}

template <class Gen>
std::size_t uep_encoder<Gen>::total_coded_count() const {
  return std_enc.total_coded_count();
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
  if (std_enc) return; // Already has a block
  if (std::all_of(inp_queues.cbegin(),
		  inp_queues.cend(),
		  utils::to_bool<input_block_queue>())) {
    std::vector<packet> the_block;
    the_block.reserve(block_size_out());
    // Iterate over all queues
    for (std::size_t i = 0; i < inp_queues.size(); ++i) {
      // Repeat each sub-block RF times
      for (std::size_t j = 0; j < RFs[i]; ++j) {
	// Shallow-copy the sub-block
	for (auto l = inp_queues[i].block_begin();
	     l != inp_queues[i].block_end();
	     ++l) {
	  the_block.push_back(l->shallow_copy());
	}
      }
      inp_queues[i].pop_block();
    }

    // Expand EF times
    std::size_t orig_size = the_block.size();
    for (std::size_t i = 0; i < EF-1; ++i) {
      // Shallow-copy the first repetition
      for (auto j = the_block.cbegin();
	   j != the_block.cbegin() + orig_size;
	   ++j) {
	the_block.push_back(j->shallow_copy());
      }
    }

    // Pass to the standard encoder
    for (auto i = the_block.begin(); i != the_block.end(); ++i) {
      std_enc.push(std::move(*i));
    }
  }
}

}

#endif
