#ifndef UEP_BLOCK_ENCODER
#define UEP_BLOCK_ENCODER

#include <vector>

#include "packets.hpp"
#include "rng.hpp"

/** Class used to perform the LT-encoding of a single block. */
class block_encoder {
public:
  typedef std::vector<packet>::iterator block_iterator;
  typedef std::vector<packet>::const_iterator const_block_iterator;
  typedef lt_row_generator::rng_type::result_type seed_t;

  explicit block_encoder(const lt_row_generator &rg);
  
  /** Reset the row generator with the specified seed. */
  void set_seed(seed_t seed);
  /** Replace the current block with [first,last). */
  template <class InputIt>
  void set_block(InputIt first, InputIt last);
  /** Replace the current block by shallow-copying [first,last). */
  template <class InputIt>
  void set_block_shallow(InputIt first, InputIt last);
  /** Reset the encoder to the initial state: default seed and empty block. */
  void reset();

  /** Return true when the encoder has been given a block. */
  bool can_encode() const;
  /** Return the last seed used to reset the row generator. */
  seed_t seed() const;
  /** Return an iterator to the start of the block. */
  const_block_iterator block_begin() const;
  /** Return an iterator to the end of the block. */
  const_block_iterator block_end() const;
  /** Return the fixed block size. */
  std::size_t block_size() const;

  /** Return a copy of the row generator being used. */
  lt_row_generator row_generator() const;
  /** Return the number of encoded packets generated for the current block. */
  std::size_t output_count() const;

  /** Produce a new encoded packet. */
  packet next_coded();

  /** Return true when the encoder has a block. */
  explicit operator bool() const;
  /** Return true when the encoder does not have a block. */
  bool operator!() const;
  
private:
  lt_row_generator rowgen;
  std::vector<packet> block;
  std::size_t out_count;
};

// Template definitions

template <class InputIt>
void block_encoder::set_block(InputIt first, InputIt last) {
  if ((last - first) != block_size())
    throw std::logic_error("The block must have fixed length");
  block.clear();
  out_count = 0;
  for (;first != last; ++first) {
    block.push_back(*first);
  }
}

template <class InputIt>
void block_encoder::set_block_shallow(InputIt first, InputIt last) {
  if ((last-first) != block_size())
    throw std::logic_error("The block must have fixed length");
  block.clear();
  out_count = 0;
  for (;first != last; ++first) {
    block.push_back(first->shallow_copy());
  }
}

#endif
