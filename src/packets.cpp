#include "packets.hpp"

#include <stdexcept>

using namespace std;

fountain_packet::fountain_packet(int l) :
  length(l) {
}

fountain_packet::fountain_packet(const packet_data &data_) :
  length(data_.size()),
  raw_data(data_),
  blockno(0),
  seqno(0) {
}

void fountain_packet::cumulative_xor(const packet_data &other) {
  if (length != other.size())
    throw runtime_error("The packets to XOR must have the same size");
  for (int i = 0; i < length; i++) {
    raw_data[i] ^= other[i];
  }
}

void fountain_packet::cumulative_xor(const fountain_packet &other) {
  cumulative_xor(other.raw_data);
}

int fountain_packet::data_length() const {
  return length;
}

int fountain_packet::block_number() const {
  return blockno;
}

int fountain_packet::sequence_number() const {
  return seqno;
}

packet_data fountain_packet::data() const {
  return raw_data;
}

void fountain_packet::block_number(int blockno_) {
  blockno = blockno_;
}

void fountain_packet::sequence_number(int seqno_) {
  seqno = seqno_;
}

void fountain_packet::data(const packet_data &data) {
  if (data.size() != length)
    throw runtime_error("The packet size cannot change");
  raw_data = data;
}

fountain_packet operator^(const fountain_packet &a, const fountain_packet &b) {
  fountain_packet c(a);
  c.cumulative_xor(b);
  return c;
}
