#ifndef UEP_RW_UTILS_HPP
#define UEP_RW_UTILS_HPP

#include <cstdint>
#include <istream>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <arpa/inet.h>

namespace uep { namespace rw_utils {

/** Convert a number of type T from network-endianness to
 *  host-endianness.
 */
template<typename T>
struct ntoh_converter;

/** Convert a number of type T from host-endianness to
 *  network-endianness.
 */
template<typename T>
struct hton_converter;

/** Specialization of ntoh_converter for uint16_t. */
template<>
struct ntoh_converter<std::uint16_t> {
  std::uint16_t operator()(std::uint16_t n) const {
    return ntohs(n);
  }
};

/** Specialization of ntoh_converter for uint32_t. */
template<>
struct ntoh_converter<std::uint32_t> {
  std::uint32_t operator()(std::uint32_t n) const {
    return ntohl(n);
  }
};

/** Specialization of hton_converter for uint16_t. */
template<>
struct hton_converter<std::uint16_t> {
  std::uint16_t operator()(std::uint16_t n) const {
    return htons(n);
  }
};

/** Specialization of hton_converter for uint32_t. */
template<>
struct hton_converter<std::uint32_t> {
  std::uint32_t operator()(std::uint32_t n) const {
    return htonl(n);
  }
};

/** Read an object of type T from the byte range [first,last).
 *  Returns an iterator past the end of the read object.
 */
template<typename T, typename Iter>
Iter read_raw(T &out, Iter first, Iter last);

/** Write an object of type T into the byte range [first,last).
 *  Returns an iterator past the end of the written object.
 */
template<typename T, typename Iter>
Iter write_raw(const T &in, Iter first, Iter last);

/** Read a number of type T from the byte range [first,last) and
 *  convert to the host endianness.  Returns an iterator past the end
 *  of the read object.
 */
template<typename T, typename Iter>
Iter read_ntoh(T &out, Iter first, Iter last);

/** Write a number of type T into the byte range [first,last)
 *  converting to the network endianness.  Returns an iterator past
 *  the end of the written object.
 */
template<typename T, typename Iter>
Iter write_hton(const T &in, Iter first, Iter last);

/** Read an object of type T from the byte stream. */
template<typename T>
void read_raw(T &out, std::istream &str);

/** Write an object of type T into the byte stream. */
template<typename T>
void write_raw(const T &in, std::ostream &str);

/** Read a number of type T from the stream and convert to the host
 *  endianness.
 */
template<typename T>
void read_ntoh(T &out, std::istream &str);

/** Write a number of type T into the byte stream converting to the
 *  network endianness.
 */
template<typename T>
void write_hton(const T &in, std::ostream &str);

	       //// function template definitions ////
template<typename T, typename Iter>
Iter read_raw(T &out, Iter first, Iter last) {
  using iter_value_type = typename std::iterator_traits<Iter>::value_type;
  static_assert(std::is_same<iter_value_type,char>::value,
		"Iter is not an iterator over char");

  char *raw_out = reinterpret_cast<char*>(&out);
  std::size_t n = 0;
  while (n < sizeof(T) && first != last) {
    *raw_out++ = *first++;
    ++n;
  }
  if (n < sizeof(T))
    throw std::range_error("Range is too short for the given type");
  return first;
}

template<typename T, typename Iter>
Iter write_raw(const T &in, Iter first, Iter last) {
  using iter_value_type = typename std::iterator_traits<Iter>::value_type;
  static_assert(std::is_same<iter_value_type,char>::value,
		"Iter is not an iterator over char");

  const char *raw_in = reinterpret_cast<const char*>(&in);
  std::size_t n = 0;
  while (n < sizeof(T) && first != last) {
    *first++ = *raw_in++;
    ++n;
  }
  if (n < sizeof(T))
    throw std::range_error("Range is too short for the given type");
  return first;
}

template<typename T, typename Iter>
Iter read_ntoh(T &out, Iter first, Iter last) {
  static const ntoh_converter<T> conv;
  Iter i = read_raw<T>(out, first, last);
  out = conv(out);
  return i;
}

template<typename T, typename Iter>
Iter write_hton(const T &in, Iter first, Iter last) {
  static const hton_converter<T> conv;
  return write_raw<T>(conv(in), first, last);
}

template<typename T>
void read_raw(T &out, std::istream &str) {
  std::istreambuf_iterator<char> first(str);
  std::istreambuf_iterator<char> last;
  read_raw<T>(out, first, last);
}

template<typename T>
void write_raw(const T &in, std::ostream &str) {
  std::ostreambuf_iterator<char> first(str);
  std::ostreambuf_iterator<char> last;
  write_raw<T>(in, first, last);
}

template<typename T>
void read_ntoh(T &out, std::istream &str) {
  std::istreambuf_iterator<char> first(str);
  std::istreambuf_iterator<char> last;
  read_ntoh<T>(out, first, last);
}

template<typename T>
void write_hton(const T &in, std::ostream &str) {
  std::ostreambuf_iterator<char> first(str);
  std::ostreambuf_iterator<char> last;
  write_hton<T>(in, first, last);
}

}}

#endif
