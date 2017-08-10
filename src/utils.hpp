#ifndef UEP_UTILS_HPP
#define UEP_UTILS_HPP

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>

#include <boost/iterator/filter_iterator.hpp>

namespace uep {
namespace utils {

/** Predicate that just converts the input to bool. */
template <class T>
struct to_bool {
  bool operator()(const T &x) const {
    return static_cast<bool>(x);
  }
};

/** Iterator adaptor that skips over the values that convert to false. */
template <class BaseIter>
using skip_false_iterator = boost::filter_iterator<
  to_bool<typename std::iterator_traits<BaseIter>::value_type>,
  BaseIter>;

template <class T>
std::ostream &operator<<(std::ostream &out, const std::vector<T> &v) {
  out << '[';
  for (auto i = v.cbegin(); i != v.cend(); ++i) {
    out << *i << ", ";
  }
  out << "\b\b]";
  return out;
}

/** Integer hashing function. */
template <class T>
struct knuth_mul_hasher {
  std::size_t operator()(const T *k) const {
    std::ptrdiff_t v = (std::ptrdiff_t)k * UINT32_C(2654435761);
    // Right-shift v by the size difference between a pointer and a
    // 32-bit integer (0 for x86, 32 for x64)
    v >>= ((sizeof(std::ptrdiff_t) - sizeof(std::uint32_t)) * 8);
    return (std::size_t)(v & UINT32_MAX);
  }
};

}}

#endif
