#ifndef PACKETS_RW_HPP
#define PACKETS_RW_HPP

#include "packets.hpp"

#include <cstdint>
#include <climits>
#include <vector>
#include <streambuf>
#include <istream>
#include <ostream>
#include <algorithm>
#include <type_traits>

extern "C" {
#include <arpa/inet.h>
}

static_assert(CHAR_BIT == 8, "Char is not 8 bits long");

/** Byte used to identify the type of packet. */
enum raw_packet_type : char {
  data = 0,
  block_ack = 1
};

/** Total size of the header of a data packet. */
const std::size_t data_header_size = 11;
/** Total size of the header of an ACK packet. */
const std::size_t ack_header_size = 3;

/** Build a raw packet, with network-endian fields, from a
 *  fountain_packet.
 */
std::vector<char> build_raw_packet(const fountain_packet &fp);
/** Build a raw ACK packet that carries the given block number. */
std::vector<char> build_raw_ack(std::size_t blockno);
/** Parse a raw data packet into a fountain_packet.
 *  If the packet is malformed throw a runtime_error.
 */
fountain_packet parse_raw_data_packet(const std::vector<char> &rp);
/** Parse a raw ACK packet to get the block number carried by it. */
std::size_t parse_raw_ack_packet(const std::vector<char> &rp);

#endif
