#include "packets.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

using namespace std;

packet &operator^=(packet &a, const packet &b) {
  if (a.size() != b.size())
    throw runtime_error("The packets must have equal size");
  if (a.empty())
    throw runtime_error("The packets must not be empty");
  auto i = b.cbegin();
  auto j = a.begin();
  while(i != b.cend()) {
    *j++ ^= *i++;
  }
  return a;
}

packet operator^(const packet &a, const packet &b) {
  packet c(a);
  return c ^= b;
}

fountain_packet::fountain_packet() : blockno(0), seqno(0), seed(0) {
}

fountain_packet::fountain_packet(const packet &p) : packet(p) {
}

fountain_packet::fountain_packet(packet &&p) : packet(move(p)) {
}

int fountain_packet::block_number() const {
  return blockno;
}

int fountain_packet::block_seed() const {
  return seed;
}

int fountain_packet::sequence_number() const {
  return seqno;
}

void fountain_packet::block_number(int blockno_) {
  blockno = blockno_;
}

void fountain_packet::block_seed(int seed_) {
  seed = seed_;
}

void fountain_packet::sequence_number(int seqno_) {
  seqno = seqno_;
}

fountain_packet &operator^=(fountain_packet &a, const packet &b) {
  packet &a_p = a;
  a_p ^= b;
  return static_cast<fountain_packet&>(a_p);
}
