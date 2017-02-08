#ifndef PACKETS_HPP
#define PACKETS_HPP

#include <iostream>
#include <vector>

/** Base packet class that holds a sequence of bytes (chars). */
class packet : public std::vector<unsigned char> {
public:
  virtual ~packet() = default;
};

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

/** Packet class used to represent an LT-coded packet. */
class fountain_packet : public packet {
public:
  /** Construct an empty fountain_packet. */
  fountain_packet();
  /** Construct a fountain_packet with a copy of p's data. */
  explicit fountain_packet(const packet &p);
  /** Construct a fountain_packet with p's data. */
  explicit fountain_packet(packet &&p);

  /** Get the block number. */
  int block_number() const;
  /** Get the seed used to generate this packet's block. */
  int block_seed() const;
  /** Get the sequence number within the block. */
  int sequence_number() const;

  /** Set the block number. */
  void block_number(int blockno_);
  /** Set the seed used to generate this packet's block. */
  void block_seed(int seed_);
  /** Set the sequence number within the block. */
  void sequence_number(int seqno_);

private:
  int blockno;
  int seqno;
  int seed;
};

/** In-place bitwise-XOR between the data held by two packets.
 *  \param a The packet whose content will be modified.
 *  \param b The other packet.
 *  \return A reference to a.
 */
fountain_packet &operator^=(fountain_packet &a, const packet &b);

/** Write a text representation of the fountain_packet to a stream */
std::ostream &operator<<(std::ostream &out, const fountain_packet &p);

#endif
