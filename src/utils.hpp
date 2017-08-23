#ifndef UEP_UTILS_HPP
#define UEP_UTILS_HPP

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>

#include <boost/iterator/filter_iterator.hpp>

namespace uep { namespace utils {

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

/** Type traits that provide the interface for the symbols used in the
 *  mp_context class.
 */
template <class Symbol>
class symbol_traits {
private:
  static void swap_syms(Symbol &lhs, Symbol &rhs) {
    using std::swap;
    swap(lhs, rhs);
  }

  static_assert(std::is_move_constructible<Symbol>::value,
		"Symbol must be move-constructible");
  static_assert(std::is_move_assignable<Symbol>::value,
		"Symbol must be move-assignable");

public:
  /** Create an empty symbol. */
  static Symbol create_empty() {
    return Symbol();
  }

  /** True when a symbol is empty. */
  static bool is_empty(const Symbol &s) {
    return !s;
  }

  /** Perform the in-place XOR between two symbols. */
  static void inplace_xor(Symbol &lhs, const Symbol &rhs) {
    lhs ^= rhs;
  }

  /** Swap two symbols. */
  static void swap(Symbol &lhs, Symbol &rhs) {
    // Avoid using the function with the same name
    swap_syms(lhs,rhs);
  }
};

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

namespace std {

/** Write a text representation of a vector. */
template <class T>
ostream &operator<<(ostream &out, const vector<T> &v) {
  out << '[';
  for (auto i = v.cbegin(); i != v.cend(); ++i) {
    out << *i << ", ";
  }
  out << "\b\b]";
  return out;
}

}

#endif
