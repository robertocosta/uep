#ifndef UEP_UTILS_HPP
#define UEP_UTILS_HPP

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>

#include <boost/iterator/filter_iterator.hpp>

static_assert(std::is_same<std::uint8_t,unsigned char>::value,
	      "unsigned char is not std::uint8_t");
using byte = unsigned char;

class buffer {
public:
  buffer() : buffer(nullptr, 0) {
  }
  /** Construct a buffer over the already allocated memory given by
   *  the arguments. The buffer will not free this memory upon
   *  destruction.
   */
  explicit buffer(void *mem, std::size_t size) :
    membegin(reinterpret_cast<byte*>(mem)),
    bufbegin(membegin),
    memend(membegin+size),
    bufend(memend) {
  }

  const void *allocated_memory() const {
    return membegin;
  }

  void *allocated_memory() {
    return membegin;
  }

  std::size_t allocated_size() const {
    return memend - membegin;
  }

  const byte *begin() const {
    return bufbegin;
  }

  byte *begin() {
    return bufbegin;
  }

  const byte *end() const {
    return bufend;
  }

  byte *end() {
    return bufend;
  }

  std::size_t size() const {
    return bufend - bufbegin;
  }

  void trim_front(std::size_t num) {
    if (static_cast<std::size_t>(bufend - bufbegin) >= num)
      bufbegin += num;
    else
      throw std::range_error("Would trim after the end");
  }

  void extend_front(std::size_t num) {
    if (static_cast<std::size_t>(bufbegin - membegin) >= num)
      bufbegin -= num;
    else
      throw std::range_error("Would extend after the allocated memory");
  }

  void trim_back(std::size_t num) {
    if (static_cast<std::size_t>(bufend - bufbegin) >= num)
      bufend -= num;
    else
      throw std::range_error("Would trim after the beginning");
  }

  void extend_back(std::size_t num) {
    if (static_cast<std::size_t>(memend - bufend) >= num)
      bufend += num;
    else
      throw std::range_error("Would extend after the allocated memory");
  }

  const buffer slice(std::size_t seek, std::size_t size) const {
    buffer b{*this};
    b.trim_front(seek);
    if (static_cast<std::size_t>(b.memend - b.bufbegin) >= size)
      b.bufend = b.bufbegin + size;
    else
      throw std::range_error("Would extend after the allocated memory");
    return b;
  }

  buffer slice(std::size_t seek, std::size_t size) {
    const buffer *const cthis = this;
    return cthis->slice(seek, size);
  }

protected:
  byte *membegin; /**< Start of the allocated memory. */
  byte *bufbegin; /**< Start of the buffer. */
  byte *memend; /**< End of the allocated memory. */
  byte *bufend; /**< End of the buffer. */
};

class unique_buffer : public buffer {
public:
  explicit unique_buffer(std::size_t size) :
    mem{new byte[size]} {
    membegin = mem.get();
    bufbegin = membegin;
    memend = membegin + size;
    bufend = memend;
  }

private:
  std::unique_ptr<byte[]> mem;
};

class shared_buffer : public buffer {
public:
  explicit shared_buffer(std::size_t size) :
    mem{new byte[size], std::default_delete<byte[]>{}} {
    membegin = mem.get();
    bufbegin = membegin;
    memend = membegin + size;
    bufend = memend;
  }

private:
  std::shared_ptr<byte> mem;
};

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
