#ifndef PACKETS_HPP
#define PACKETS_HPP

#include <cstdint>
#include <memory>

class fountain_packet {
public:
  fountain_packet();
  explicit fountain_packet(std::uint_fast32_t length);
  explicit fountain_packet(const std::shared_ptr<std::uint8_t> &data_ptr,
			   std::uint_fast32_t length);

  // fountain_packet &operator=(const fountain_packet &other);

  std::uint_fast32_t length() const;
  std::uint_fast32_t blockno() const;
  std::uint_fast32_t seqno() const;
  bool has_data() const;
  fountain_packet clone() const;
  std::shared_ptr<std::uint8_t> data() const;

  void blockno(std::uint_fast32_t blockno);
  void seqno(std::uint_fast32_t seqno);

  void xor_data(const fountain_packet &other);
  fountain_packet &operator^=(const fountain_packet &other);

private:
  std::uint_fast32_t length_;
  std::shared_ptr<std::uint8_t> data_;

  std::uint_fast32_t blockno_;
  std::uint_fast32_t seqno_;
};

fountain_packet operator^(const fountain_packet &a, const fountain_packet &b);

struct packet_data_equal {
  bool operator()(const fountain_packet &lhs, const fountain_packet &rhs);
};

#endif
