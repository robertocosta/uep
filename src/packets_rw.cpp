#include "packets_rw.hpp"

#include <stdexcept>
#include <iterator>

#include <boost/numeric/conversion/cast.hpp>

using namespace std;
using boost::numeric_cast;

template <class T>
void append_raw(std::vector<char> &out, const T &n) {
  const char *start = reinterpret_cast<const char*>(&n);
  const char *end = start + sizeof(T);
  out.insert(out.end(), start, end);
}

template <class T, class IterType>
T extract_raw(IterType &in) {
  typedef typename iterator_traits<IterType>::value_type iter_value_type;
  static_assert(is_same<iter_value_type,char>::value,
		"IterType is not an iterator over char");
  
  T out;
  char *raw_out = reinterpret_cast<char*>(&out);
  IterType start = in;
  IterType end = (in += sizeof(T));
  copy(start, end, raw_out);

  return out;
}

void append_hton_int(std::vector<char> &out, std::uint16_t n) {
  n = htons(n);
  append_raw<uint16_t>(out, n);
}

void append_hton_int(std::vector<char> &out, std::uint32_t n) {
  n = htonl(n);
  append_raw<uint32_t>(out, n);
}

template <class IterType>
std::uint16_t extract_ntoh_uint16(IterType &in) {
  uint16_t n = extract_raw<uint16_t, IterType>(in);
  return ntohs(n);
}

template <class IterType>
std::uint32_t extract_ntoh_uint32(IterType &in) {
  uint32_t n = extract_raw<uint32_t, IterType>(in);
  return ntohl(n);
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
    if (rp.size() != length + data_header_size)
      throw runtime_error("The packet has the wrong length");
    fp.resize(length);
    copy(i, rp.cend(), fp.begin());
  }

  return fp;
}
