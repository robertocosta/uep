#include "packets.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

using namespace std;

fountain_packet::fountain_packet() :
  length_(0) {
}

fountain_packet::fountain_packet(std::uint_fast32_t length) :
  length_(length) {
}

fountain_packet::fountain_packet(const std::shared_ptr<std::uint8_t> &data_ptr,
				 std::uint_fast32_t length) :
  fountain_packet(length) {
  data_ = data_ptr;
}

// fountain_packet &fountain_packet::operator=(const fountain_packet &other) {
//   if (other.length_ != length_) {
//     throw runtime_error("The packets have different sizes");
//   }
//   data_ = other.data_;
//   blockno_ = other.blockno_;
//   seqno_ = other.seqno_;
//   return *this;
// }

std::uint_fast32_t fountain_packet::length() const {
  return length_;
}

std::uint_fast32_t fountain_packet::blockno() const {
  return blockno_;
}

std::uint_fast32_t fountain_packet::seqno() const {
  return seqno_;
}

bool fountain_packet::has_data() const {
  return data_ != nullptr;
}

fountain_packet fountain_packet::clone() const {
  uint8_t *cloned_data = new uint8_t[length_];
  uint8_t *first = data_.get();
  uint8_t *last = first + length_;
  copy(first, last, cloned_data);
  shared_ptr<uint8_t> cloned_ptr(cloned_data, default_delete<uint8_t[]>());
  fountain_packet p(*this);
  p.data_ = cloned_ptr;
  return p;
}

void fountain_packet::blockno(std::uint_fast32_t blockno) {
  blockno_ = blockno;
}

void fountain_packet::seqno(std::uint_fast32_t seqno) {
  seqno_ = seqno;
}

std::shared_ptr<std::uint8_t> fountain_packet::data() const {
  return data_;
}

void fountain_packet::xor_data(const fountain_packet &other) {
  if (length_ != other.length_)
    throw runtime_error("The packets to XOR must have the same size");
  for (uint_fast32_t i = 0; i < length_; i++) {
    data_.get()[i] ^= other.data_.get()[i];
  }
}

fountain_packet operator^(const fountain_packet &a, const fountain_packet &b) {
  if (a.length() != b.length())
    throw runtime_error("The packets to XOR must have the same size");
  fountain_packet c = a.clone();
  c.xor_data(b);
  return c;
}

fountain_packet &fountain_packet::operator^=(const fountain_packet &other) {
  xor_data(other);
  return *this;
}

bool packet_data_equal::operator()(const fountain_packet &lhs,
				   const fountain_packet &rhs) {
  if (lhs.length() != rhs.length()) return false;
  const uint8_t *raw_lhs = lhs.data().get();
  const uint8_t *raw_rhs = rhs.data().get();
  return std::equal(raw_lhs, raw_lhs + lhs.length(), raw_rhs);
}
