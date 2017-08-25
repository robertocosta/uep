#ifndef UEP_BLOCK_QUEUES_HPP
#define UEP_BLOCK_QUEUES_HPP

#include <cstddef>
#include <queue>
#include <stdexcept>
#include <vector>

#include "packets.hpp"

namespace uep {

/** Class used to segment a stream in fixed-length blocks.  The
 *  elements are enqueued using push() and, when has_block() is true,
 *  a block can be accessed using a pair of iterators or by position.
 */
template <class T>
class block_queue {
public:
  typedef T value_type;
  typedef typename std::vector<T>::iterator block_iterator;
  typedef typename std::vector<T>::const_iterator const_block_iterator;
  typedef std::move_iterator<block_iterator> move_block_iterator;

  explicit block_queue(std::size_t block_size);

  /** Enqueue an element */
  void push(T &&p);
  /** Enqueue an element */
  void push(const T &p);
  /** Remove the current block. If there is no block, raise a logic_error. */
  void pop_block();
  /** Remove all elements. */
  void clear();

  /** Return true when there are enough elements to form a block. */
  bool has_block() const;
  /** Return true when there are no elements in the queue and in the
   *  current block.
   */
  bool empty() const;
  /** The block size. */
  std::size_t block_size() const;
  /** The number of queued elements, excluding those of the current
   *  block.
   */
  std::size_t queue_size() const;
  /** The total number of elements held by the block_queue. */
  std::size_t size() const;

  /** Constant random iterator pointing to the start of the block.
   *  If there is no block, a logic_error is raised.
   */
  const_block_iterator block_begin() const;
  /** Constant random iterator pointing to the end of the block.
   *  If there is no block, a logic_error is raised.
   */
  const_block_iterator block_end() const;
  /** Return a const reference to the element at position pos in the
   *  block.  If there is no block, a logic_error is raised.
   */
  const T &block_at(std::size_t pos) const;
  /** Move-iterator pointing to the start of the block. */
  move_block_iterator block_mbegin();
  /** Move-iterator pointing to the end of the block. */
  move_block_iterator block_mend();

  /** True when has_block. */
  explicit operator bool() const;
  /** True when has_block is false. */
  bool operator!() const;

private:
  std::size_t K;
  std::queue<T> input_queue;
  std::vector<T> input_block;

  /** Check if the queue has enough elements to build a block. */
  void check_has_block();
};

}

/** Block queue for packets. */
typedef uep::block_queue<packet> input_block_queue;

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
   *  the queue. The packets can be move-copied if Iter is a
   *  move_iterator.
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
  /** Return a reference to the top of the queue. */
  packet &front();

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

//	       output_block_queue template definitions

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

//		 block_queue<T> template definitions

namespace uep {

template <class T>
block_queue<T>::block_queue(std::size_t block_size):
  K(block_size) {
  input_block.reserve(K);
}

template <class T>
void block_queue<T>::push(T &&p) {
  input_queue.push(std::move(p));
  check_has_block();
}

template <class T>
void block_queue<T>::push(const T &p) {
  T p_copy(p);
  push(std::move(p_copy));
}

template <class T>
bool block_queue<T>::has_block() const {
  return input_block.size() == K;
}

template <class T>
bool block_queue<T>::empty() const {
  return input_queue.empty() && input_block.empty();
}

template <class T>
void block_queue<T>::pop_block() {
  if (!has_block())
    throw std::logic_error("Cannot pop without a full block");
  input_block.clear();
  check_has_block();
}

template <class T>
typename block_queue<T>::const_block_iterator
block_queue<T>::block_begin() const {
  if (!has_block()) throw std::logic_error("Does not have a block");
  return input_block.cbegin();
}

template <class T>
typename block_queue<T>::const_block_iterator
block_queue<T>::block_end() const {
  if (!has_block()) throw std::logic_error("Does not have a block");
  return input_block.cend();
}

template <class T>
const T &block_queue<T>::block_at(std::size_t pos) const {
  if (!has_block()) throw std::logic_error("Does not have a block");
  return input_block.at(pos);
}

template <class T>
typename block_queue<T>::move_block_iterator
block_queue<T>::block_mbegin() {
  if (!has_block()) throw std::logic_error("Does not have a block");
  return move_block_iterator(input_block.begin());
}

template <class T>
typename block_queue<T>::move_block_iterator
block_queue<T>::block_mend() {
  if (!has_block()) throw std::logic_error("Does not have a block");
  return move_block_iterator(input_block.end());
}

template <class T>
void block_queue<T>::clear() {
  input_block.clear();
  typedef decltype(input_queue) queue_t;
  input_queue = queue_t();
}

template <class T>
std::size_t block_queue<T>::block_size() const {
  return K;
}

template <class T>
std::size_t block_queue<T>::queue_size() const {
  return input_queue.size();
}

template <class T>
std::size_t block_queue<T>::size() const {
  return input_queue.size() + (has_block() ? K : 0);
}

template <class T>
void block_queue<T>::check_has_block() {
  using std::swap;

  if (!has_block() && input_queue.size() >= K)
    for (std::size_t i = 0; i < K; ++i) {
      T p;
      swap(p, input_queue.front());
      input_queue.pop();
      input_block.push_back(std::move(p));
    }
}

template <class T>
block_queue<T>::operator bool() const {
  return has_block();
}

template <class T>
bool block_queue<T>::operator!() const {
  return !has_block();
}

}

#endif
