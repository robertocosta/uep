#include "packets_rw.hpp"

using namespace std;
using namespace uep::rw_utils;

using boost::numeric_cast;

void append_hton_int(std::vector<char> &out, std::uint16_t n) {
  out.resize(out.size() + sizeof(std::uint16_t));
  write_hton<std::uint16_t>(n,
			    out.end() - sizeof(std::uint16_t),
			    out.end());
}

void append_hton_int(std::vector<char> &out, std::uint32_t n) {
  out.resize(out.size() + sizeof(std::uint32_t));
  write_hton<std::uint32_t>(n,
			    out.end() - sizeof(std::uint32_t),
			    out.end());
}

template <class IterType>
std::uint16_t extract_ntoh_uint16(IterType &in) {
  std::uint16_t out;
  IterType i = read_ntoh<std::uint16_t>(out, in, in+sizeof(std::uint16_t));
  in = i;
  return out;
}

template <class IterType>
std::uint32_t extract_ntoh_uint32(IterType &in) {
  std::uint32_t out;
  IterType i = read_ntoh<std::uint32_t>(out, in, in+sizeof(std::uint32_t));
  in = i;
  return out;
}

std::vector<char> build_raw_packet(const fountain_packet &fp) {
  vector<char> out;
  out.reserve(data_header_size + fp.size());

  out.push_back(raw_packet_type::data);

  uint16_t blockno = numeric_cast<uint16_t>(fp.block_number());
  append_hton_int(out, blockno);

  uint16_t seqno = numeric_cast<uint16_t>(fp.sequence_number());
  append_hton_int(out, seqno);

  // Don't throw on negative values
  uint32_t seed = numeric_cast<int32_t>(fp.block_seed());
  append_hton_int(out, seed);

  // This is not needed when using UDP (length is known)
  uint16_t length = numeric_cast<uint16_t>(fp.size());
  append_hton_int(out, length);

  out.insert(out.end(), fp.cbegin(), fp.cend());

  return out;
}

fountain_packet parse_raw_data_packet(const std::vector<char> &rp) {
  if (rp.size() < data_header_size) throw runtime_error("The packet is too short");
  auto i = rp.cbegin();
  fountain_packet fp;

  char type = *i++;
  if (type != raw_packet_type::data) throw runtime_error("Not a data packet");

  uint16_t blockno = extract_ntoh_uint16(i);
  fp.block_number(blockno);

  uint16_t seqno = extract_ntoh_uint16(i);
  fp.sequence_number(seqno);

  uint32_t seed = extract_ntoh_uint32(i);
  fp.block_seed(seed);

  uint16_t length = extract_ntoh_uint16(i);
  if (length != 0) {
    if (rp.size() < length + data_header_size)
      throw runtime_error("The packet is too short");
    fp.resize(length);
    copy(i, i + length, fp.begin());
  }

  return fp;
}

std::vector<char> build_raw_ack(std::size_t blockno) {
  vector<char> out;
  out.reserve(ack_header_size);

  out.push_back(raw_packet_type::block_ack);

  uint16_t block_ack = numeric_cast<uint16_t>(blockno);
  append_hton_int(out, block_ack);

  return out;
}

std::size_t parse_raw_ack_packet(const std::vector<char> &rp) {
  if (rp.size() < ack_header_size)
    throw runtime_error("The packet is too short");
  auto i = rp.cbegin();

  char type = *i++;
  if (type != raw_packet_type::block_ack)
    throw runtime_error("Not an ACK packet");

  uint16_t blockno = extract_ntoh_uint16(i);
  return blockno;
}
