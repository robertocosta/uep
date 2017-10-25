#ifndef UEP_BASE_TYPES_HPP
#define UEP_BASE_TYPES_HPP

#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

/** General-purpose integer type. */
using f_int = std::int_fast32_t;
/** General-purpose unsigned integer type. */
using f_uint = std::uint_fast32_t;
// Check that chars are 8-bit long
static_assert(std::is_same<std::uint8_t,unsigned char>::value,
	      "unsigned char is not std::uint8_t");

namespace uep {
typedef std::vector<char> buffer_type;

/** Perform a bitwise XOR between two buffers. */
void inplace_xor(buffer_type &lhs, const buffer_type &rhs);

}

#endif
