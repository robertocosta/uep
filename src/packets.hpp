#ifndef PACKETS_HPP
#define PACKETS_HPP

#include <cstdint>
#include <vector>
#include <memory>

typedef std::vector<std::uint8_t> packet;

packet &operator^=(packet &a, const packet &b);
packet operator^(const packet &a, const packet &b);

class fountain_packet : public packet {
public:
  fountain_packet();
  explicit fountain_packet(const packet &p);
  explicit fountain_packet(packet &&p);

  std::uint_fast16_t block_number() const;
  std::uint_fast32_t block_seed() const;
  std::uint_fast16_t sequence_number() const;

  void block_number(std::uint_fast16_t blockno_);
  void block_seed(std::uint_fast32_t seed_);
  void sequence_number(std::uint_fast16_t seqno_);

private:
  std::uint_fast16_t blockno;
  std::uint_fast16_t seqno;
  std::uint_fast32_t seed;
};

fountain_packet &operator^=(fountain_packet &a, const packet &b);

#endif
