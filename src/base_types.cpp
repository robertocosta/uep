#include "base_types.hpp"

using namespace std;

namespace uep {
void inplace_xor(buffer_type &lhs, const buffer_type &rhs) {
  if (lhs.size() != rhs.size())
    throw runtime_error("XOR buffers with different sizes");
  if (lhs.empty())
    throw runtime_error("XOR empty bufffers");

  typedef std::uint_fast32_t fast_uint;

  fast_uint *i = reinterpret_cast<fast_uint*>(lhs.data());
  const fast_uint *j = reinterpret_cast<const fast_uint*>(rhs.data());
  fast_uint *lhs_fast_end = i + (lhs.size() / sizeof(fast_uint));
  while (i != lhs_fast_end) {
    *i++ ^= *j++;
  }

  char *i_c = reinterpret_cast<char*>(i);
  const char *j_c = reinterpret_cast<const char*>(j);
  char *lhs_end = lhs.data() + lhs.size();
  while (i_c != lhs_end) {
    *i_c++ ^= *j_c++;
  }
}
}
