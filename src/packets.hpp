#ifndef PACKETS_HPP
#define PACKETS_HPP

#include <vector>

typedef std::vector<unsigned char> packet_data;

/** Encoded fountain packet: raw data + block id + order within the block */
class fountain_packet {
public:
  explicit fountain_packet(int l);
  explicit fountain_packet(const packet_data &data_);

  void cumulative_xor(const packet_data &other);
  void cumulative_xor(const fountain_packet &other);

  int data_length() const;
  int block_number() const;
  int sequence_number() const;
  packet_data data() const;

  void block_number(int blockno_);
  void sequence_number(int seqno_);
  void data(const packet_data &data);

private:
  const int length;
  packet_data raw_data;
  int blockno;
  int seqno;
};

fountain_packet operator^(const fountain_packet &a, const fountain_packet &b);

#endif
