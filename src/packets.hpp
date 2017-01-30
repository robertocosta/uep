#ifndef PACKETS_HPP
#define PACKETS_HPP

#include <vector>

class packet : public std::vector<unsigned char> {
public:
  virtual ~packet() = default;
};

packet &operator^=(packet &a, const packet &b);
packet operator^(const packet &a, const packet &b);

class fountain_packet : public packet {
public:
  fountain_packet();
  explicit fountain_packet(const packet &p);
  explicit fountain_packet(packet &&p);

  int block_number() const;
  int block_seed() const;
  int sequence_number() const;

  void block_number(int blockno_);
  void block_seed(int seed_);
  void sequence_number(int seqno_);

private:
  int blockno;
  int seqno;
  int seed;
};

fountain_packet &operator^=(fountain_packet &a, const packet &b);

#endif
