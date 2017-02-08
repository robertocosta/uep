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

enum raw_packet_type : char {
  data = 0,
  block_ack = 1
};

const std::size_t data_header_size = 11;

std::vector<char> build_raw_packet(const fountain_packet &fp);
fountain_packet parse_raw_data_packet(const std::vector<char> &rp);

#endif
