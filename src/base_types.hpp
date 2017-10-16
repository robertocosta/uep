#ifndef UEP_BASE_TYPES_HPP
#define UEP_BASE_TYPES_HPP

#include <cstdint>
#include <cstring>
#include <type_traits>

using f_int = std::int_fast32_t;
using f_uint = std::uint_fast32_t;
static_assert(std::is_same<std::uint8_t,unsigned char>::value,
	      "unsigned char is not std::uint8_t");
using byte = unsigned char;

template<bool constbuf>
class base_buffer {
  using cbyte_ptr = const byte*;
  using byte_ptr = typename std::conditional<constbuf,
					     const byte*,
					     byte*>::type;
  using cvoid_ptr = const void*;
  using void_ptr = typename std::conditional<constbuf,
					     const void*,
					     void*>::type;
public:
  base_buffer() : base_buffer(nullptr, 0) {
  }
  /** Construct a buffer over the already allocated memory given by
   *  the arguments. The buffer will not free this memory upon
   *  destruction.
   */
  explicit base_buffer(void_ptr mem, std::size_t size) :
    membegin(reinterpret_cast<byte_ptr>(mem)),
    bufbegin(membegin),
    memend(membegin+size),
    bufend(memend) {
  }

  cvoid_ptr allocated_memory() const {
    return membegin;
  }

  void_ptr allocated_memory() {
    return membegin;
  }

  std::size_t allocated_size() const {
    return memend - membegin;
  }

  template<typename chartype>
  const chartype *data() const {
    return reinterpret_cast<const chartype*>(bufbegin);
  }

  template<typename chartype>
  auto data()/* -> ret_type */{
    using ret_type = typename std::conditional<constbuf,
					       const chartype*,
					       chartype*>::type;
    return reinterpret_cast<ret_type>(bufbegin);
  }

  cbyte_ptr begin() const {
    return bufbegin;
  }

  byte_ptr begin() {
    return bufbegin;
  }

  cbyte_ptr end() const {
    return bufend;
  }

  byte_ptr end() {
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

  const base_buffer slice(std::size_t seek, std::size_t size) const {
    base_buffer b{*this};
    b.trim_front(seek);
    if (static_cast<std::size_t>(b.memend - b.bufbegin) >= size)
      b.bufend = b.bufbegin + size;
    else
      throw std::range_error("Would extend after the allocated memory");
    return b;
  }

  base_buffer slice(std::size_t seek, std::size_t size) {
    const base_buffer *const cthis = this;
    return cthis->slice(seek, size);
  }

  operator base_buffer<true>() const;
  template<bool constbuf_>
  friend base_buffer<constbuf_>::operator base_buffer<true>() const;

protected:
  byte_ptr membegin; /**< Start of the allocated memory. */
  byte_ptr bufbegin; /**< Start of the buffer. */
  byte_ptr memend; /**< End of the allocated memory. */
  byte_ptr bufend; /**< End of the buffer. */
};

template<>
inline base_buffer<false>::operator base_buffer<true>() const {
  base_buffer<true> cbuf;
  cbuf.membegin = membegin;
  cbuf.bufbegin = bufbegin;
  cbuf.memend = memend;
  cbuf.bufend = bufend;
  return cbuf;
}

using buffer = base_buffer<false>;
using const_buffer = base_buffer<true>;

class unique_buffer : public buffer {
public:
  unique_buffer() : unique_buffer{8} {
  }

  explicit unique_buffer(std::size_t size) :
    mem{new byte[size]} {
    membegin = mem.get();
    bufbegin = membegin;
    memend = membegin + size;
    bufend = memend;
  }

  void resize(std::size_t size) {
    if (static_cast<std::size_t>(memend - bufbegin) >= size) {
      bufend = bufbegin + size;
    }
    else {
      decltype(mem) newmem{new byte[size]};
      std::memmove(newmem.get(), bufbegin, this->size());
      membegin = mem.get();
      bufbegin = membegin;
      memend = membegin + size;
      bufend = memend;
    }
  }

private:
  std::unique_ptr<byte[]> mem;
};

#endif
