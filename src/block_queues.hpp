#ifndef UEP_BLOCK_QUEUES_HPP
#define UEP_BLOCK_QUEUES_HPP

#include <cstddef>
#include <queue>
#include <stdexcept>
#include <vector>

#include "packets.hpp"

/** Class used to segment a stream of packets in fixed-length blocks.
 *  The packets are enqueued using push() and, when has_block() is
 *  true, a block can be accessed using a pair of iterators or by
 *  position.
 */
class input_block_queue {
public:
  typedef std::vector<packet>::iterator block_iterator;
  typedef std::vector<packet>::const_iterator const_block_iterator;

  explicit input_block_queue(std::size_t block_size);

  /** Enqueue a packet */
  void push(packet &&p);
  /** Enqueue a packet */
  void push(const packet &p);
  /** Remove the current block. If there is no block, raise a logic_error. */
  void pop_block();
  /** Remove all packets. */
  void clear();

  /** Return true when there are enough packets to form a block. */
  bool has_block() const;
  /** The block size. */
  std::size_t block_size() const;
  /** The number of queued packets, excluding those of the current block. */
  std::size_t queue_size() const;
  /** The total number of packets. */
  std::size_t size() const;

  /** Constant random iterator pointing to the start of the block.
   *  If there is no block, a logic_error is raised.
   */
  const_block_iterator block_begin() const;
  /** Constant random iterator pointing to the end of the block.
   *  If there is no block, a logic_error is raised.
   */
  const_block_iterator block_end() const;
  /** Return a const reference to the packet at position pos in the block.
   *  If there is no block, a logic_error is raised.
   */
  const packet &block_at(std::size_t pos) const;

  /** True when has_block returns true. */
  explicit operator bool() const;
  /** True when has_block returns false. */
  bool operator!() const;

private:
  std::size_t K;
  std::queue<packet> input_queue;
  std::vector<packet> input_block;

  void check_has_block();
};

/** Class used to pack together blocks of packets.
 *  This class copies (deeply or shallowly) every block of packets
 *  passed to it in a FIFO queue. Then it allows access to the oldest
 *  one in a way similar to the std::queue class.
 */
class output_block_queue {
public:
  /** Construct a queue that only accepts blocks of size
   *  block_size.
   */
  explicit output_block_queue(std::size_t block_size);

  /** Push all the packets in the [first,last) sequence to the end of
   *  the queue. The packets are deep-copied.
   */
  template <class InputIt>
  void push(InputIt first, InputIt last);
  /** Push all the packets in the [first,last) sequence to the end of
   *  the queue. The packets are shallow-copied.
   */
  template <class InputIt>
  void push_shallow(InputIt first, InputIt last);

  /** Return a const reference to the packet at the top of the queue. */
  const packet &front() const;
  /** Remove the packet at the front of the queue. */
  void pop();
  /** Remove all the packets from the queue. */
  void clear();

  /** Return true when the queue is empty. */
  bool empty() const;
  /** Return the number of queued packets. */
  std::size_t size() const;

  /** True when empty returns false. */
  explicit operator bool() const;
  /** True when empty returns true. */
  bool operator!() const;

private:
  std::size_t K;
  std::queue<packet> output_queue;
};

// Template definitions

template <class InputIt>
void output_block_queue::push(InputIt first, InputIt last) {
  std::size_t count = 0;
  for (;first != last; ++first) {
    output_queue.push(*first);
    ++count;
  }
  if (count != K)
    throw std::logic_error("The block must have length K");
}

template <class InputIt>
void output_block_queue::push_shallow(InputIt first, InputIt last) {
  std::size_t count = 0;
  for (;first != last; ++first) {
    output_queue.push(first->shallow_copy());
    ++count;
  }
  if (count != K)
    throw std::logic_error("The block must have length K");
}

#endif
