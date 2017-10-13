#ifndef UEP_PACKETS_HPP
#define UEP_PACKETS_HPP

#include <cstdint>
#include <memory>
#include <ostream>
#include <random>
#include <type_traits>
#include <vector>

#include "utils.hpp"

namespace uep {

/** Class to hold a chunk of data. */
typedef std::vector<char> buffer_type;

/** Perform a bitwise XOR between two buffers. */
void inplace_xor(buffer_type &lhs, const buffer_type &rhs);

/** Specialization of symbol_traits for buffers. */
namespace utils {
template<>
class symbol_traits<buffer_type> {
public:
  static bool is_empty(const buffer_type &s) {
    return s.empty();
  }

  static void inplace_xor(buffer_type &lhs, const buffer_type &rhs) {
    uep::inplace_xor(lhs,rhs);
  }
};
}

}

/** Base packet class that holds a sequence of bytes (chars).
 *  The interface of this class is very similar to std::vector and
 *  most of the methods are just proxies to a std::vector<char>. The
 *  data is held via a shared pointer, so multiple packets can refer
 *  to the same data. The copy constructor / assignment do a full copy
 *  of the data. To produce a packet without copying the underlying
 *  data use shallow_copy().
 *  \sa shallow_copy(), std::vector
 */
class packet {
public:
  typedef std::vector<char>::size_type size_type;
  typedef std::vector<char>::difference_type difference_type;
  typedef std::vector<char>::iterator iterator;
  typedef std::vector<char>::const_iterator const_iterator;
  typedef std::vector<char>::reverse_iterator reverse_iterator;
  typedef std::vector<char>::const_reverse_iterator const_reverse_iterator;

  packet();

  packet(const uep::buffer_type &b);
  packet(uep::buffer_type &&b);

  explicit packet(size_t size, char value = 0);
  /** Copy-construct a packet.
   *  This constructor duplicates the packet data.
   *  \sa shallow_copy(), packet(packet&&)
   */
  packet(const packet &p);
  /** Move-construct a packet.
   *  This constructor does not copy the packet data, but moves
   *  the shared pointer to it.
   *  \sa shallow_copy(), packet(const packet&)
   */
  packet(packet &&p) = default;

  /** Virtual default destructor.
   *  There are subclasses of packet, ensure that they are always
   *  correctly destroyed.
   */
  virtual ~packet() = default;

  /** Assign a duplicate of the data from another packet. */
  packet &operator=(const packet &p);
  /** Assign the same shared data from another packet rvalue. */
  packet &operator=(packet &&p) = default;

  void assign(size_type count, char value);
  template <class InputIter> void assign(InputIter first, InputIter last);

  char &at(size_type pos);
  const char &at(size_type pos) const;
  char &operator[](size_type pos);
  const char &operator[](size_type pos) const;
  char &front();
  const char &front() const;
  char &back();
  const char &back() const;
  char *data();
  const char *data() const;

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;
  reverse_iterator rbegin();
  reverse_iterator rend();
  const_reverse_iterator rbegin() const;
  const_reverse_iterator rend() const;
  const_reverse_iterator crbegin() const;
  const_reverse_iterator crend() const;

  bool empty() const;
  size_type size() const;
  size_type max_size() const;
  void reserve(size_type new_cap);
  size_type capacity() const;

  void clear();
  iterator insert(const_iterator pos, char value);
  iterator insert(const_iterator pos, size_type count, char value);
  template <class InputIter>
  iterator insert(const_iterator pos, InputIter first, InputIter last);
  iterator erase(const_iterator pos);
  iterator erase(const_iterator first, const_iterator last);
  void push_back(char value);
  void pop_back();
  void resize(size_type count);
  void resize(size_type count, char value);
  void swap(packet &other);

  /** Perform a shallow copy of the packet.
   *  To be useful this method requires the move-constructor and
   *  move-assignment not to perform a full copy of the data.
   *  \return A packet with a shared pointer to this packet's same
   *  data.
   *  \sa packet(packet&&), operator=(packet&&)
   */
  packet shallow_copy() const;
  /** Number of packets sharing this packet's data. */
  std::size_t shared_count() const;

  /** Perform a bitwise-XOR between this packet and another packet. */
  void xor_data(const packet &other);

  uep::buffer_type &buffer() {
    return *shared_data;
  }
  const uep::buffer_type &buffer() const {
    return *shared_data;
  }

  /** Is true when the packet is non-empty. */
  explicit operator bool() const;
  /** Is true when the packet is empty. */
  bool operator!() const;

  friend bool operator==(const packet &lhs, const packet &rhs);

protected:
  /** Shared pointer to the packet's data. Must never be null. */
  std::shared_ptr<uep::buffer_type> shared_data;
};

bool operator==(const packet &lhs, const packet &rhs);
bool operator!=(const packet &lhs, const packet &rhs);
void swap(packet &lhs, packet &rhs);

/** In-place bitwise-XOR between the data held by two packets.
 *  \param a The packet whose content will be modified.
 *  \param b The other packet.
 *  \return A reference to a.
 */
packet &operator^=(packet &a, const packet &b);

/** Bitwise-XOR between the data held by two packets.
 *  \param a First packet.
 *  \param b Second packet.
 *  \return A new packet which holds the XOR result.
 */
packet operator^(const packet &a, const packet &b);

/** Packet class used to represent an LT-coded packet.
 *  This kind of packets holds, in addition to the data, a block
 *  number and sequence number to order it and a seed to allow the
 *  regeneration of the same rows in the LT-code.
 */
class fountain_packet : public packet {
public:
  fountain_packet();

  explicit fountain_packet(int blockno_, int seqno_, int seed_);
  explicit fountain_packet(size_type count, char value);
  explicit fountain_packet(int blockno_, int seqno_, int seed_,
			size_type count, char value);
  explicit fountain_packet(int blockno_, int seqno_, int seed_,
			   size_type count, char value, uint8_t priority);
  /** Construct a fountain_packet with a copy of a packet's data. */
  explicit fountain_packet(const packet &p);
  /** Construct a fountain_packet moving a packet's data. */
  explicit fountain_packet(packet &&p);
  fountain_packet(const fountain_packet &fp) = default;
  fountain_packet(fountain_packet &&fp) = default;

  fountain_packet &operator=(const packet &other);
  fountain_packet &operator=(packet &&other);
  fountain_packet &operator=(const fountain_packet &other) = default;
  fountain_packet &operator=(fountain_packet &&other) = default;

  /** Get the priority */
  uint8_t getPriority() const;
  /** Get the block number. */
  int block_number() const;
  /** Get the seed used to generate this packet's block. */
  int block_seed() const;
  /** Get the sequence number within the block. */
  int sequence_number() const;

  /** Set the priority */
  void setPriority(uint8_t p);
  /** Set the block number. */
  void block_number(int blockno_);
  /** Set the seed used to generate this packet's block. */
  void block_seed(int seed_);
  /** Set the sequence number within the block. */
  void sequence_number(int seqno_);

  /** Override the superclass' method to return a fountain_packet.
   *  \sa packet::shallow_copy()
   */
  fountain_packet shallow_copy() const;

private:
  int blockno;
  int seqno;
  int seed;
  uint8_t priorita;
};

/** In-place bitwise-XOR between the data held by two packets.
 *  Keep the block number, sequence number and seed of the
 * fountain_packet.  Other cases are ambiguous and can only return a
 * packet using the superclass' operator.
 *  \param a The packet whose content will be modified.
 *  \param b The other packet.
 *  \return A reference to a.
 */
fountain_packet &operator^=(fountain_packet &a, const packet &b);

/** Write a text representation of the fountain_packet to a stream */
std::ostream &operator<<(std::ostream &out, const fountain_packet &p);

bool operator==(const fountain_packet &lhs, const fountain_packet &rhs );
bool operator!=(const fountain_packet &lhs, const fountain_packet &rhs );

namespace uep {

/** Packet class used to handle the UEP packets. Each packet carries,
 *  in addition to a shared buffer, a circular seqno and a priority
 *  level.
 */
class uep_packet {
public:
  /** The type used to store the seqno inside the packet payload. The
   *  actual number of available bits is 31, since the MSB is used as
   *  a flag for padding packets.
   */
  using seqno_type = std::uint32_t;
  /** Maximum value that the sequence number can take. */
  static const std::uint32_t MAX_SEQNO = 0x7fffffff;

  /** Convert a packet into a uep_packet. Read the seqno stored in the
   *  payload. \sa to_packet
   */
  static uep_packet from_packet(const packet &p);

  /** Convert a packet into a uep_packet. Read the seqno stored in the
   *  payload and copy the priority from the fountain_packet.
   *  \sa to_packet
   */
  static uep_packet from_fountain_packet(const fountain_packet &fp);

  /** Make a padding packet with the given size and sequence
   *  number. This packet contains random data.
   */
  static uep_packet make_padding(std::size_t size, seqno_type seqno);

  /** Construct a uep_packet with an empty buffer, seqno 0 and priority 0. */
  uep_packet();

  /** Construct a uep_packet with the given buffer and default seqno
   *  and priority.
   */
  explicit uep_packet(buffer_type &&b);
  /** Construct a uep_packet with a copy of the given buffer and
   *  default seqno and priority.
   */
  explicit uep_packet(const buffer_type &b);

  /** Convert to packet. Insert the seqno into the payload. */
  packet to_packet() const;
  /** Convert to packet. Insert the seqno into the payload and copy
   *  the priority.
   */
  fountain_packet to_fountain_packet() const;

  /** Return a reference to the shared buffer. */
  buffer_type &buffer();
  /** Return a const reference to the shared buffer. */
  const buffer_type &buffer() const;

  /** Return a shared pointer to the buffer. */
  std::shared_ptr<buffer_type> shared_buffer();
  /** Return a shared pointer to the const buffer. */
  std::shared_ptr<const buffer_type> shared_buffer() const;

  /** Return the priority level. */
  std::size_t priority() const;
  /** Set the priority level. */
  void priority(std::size_t p);

  /** Return the sequence number. */
  seqno_type sequence_number() const;
  /** Set the sequence number. */
  void sequence_number(seqno_type sn);

  /** Return the status of the padding flag. */
  bool padding() const;
  /** Set the padding flag. */
  void padding(bool enable);

private:
  static std::independent_bits_engine<std::default_random_engine,
				      8,
				      byte> padding_rng;

  std::shared_ptr<buffer_type> shared_buf;
  std::size_t priority_lvl;
  seqno_type seqno;
};

}

// Template definitions

template <class InputIter>
void packet::assign(InputIter first, InputIter last) {
  shared_data->assign(first, last);
}

template <class InputIter>
packet::iterator packet::insert(const_iterator pos, InputIter first, InputIter last) {
  return shared_data->insert(pos, first, last);
}

#endif
