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
/** 8-bit type used to handle raw data. */
using byte = unsigned char;

namespace uep {
typedef std::vector<char> buffer_type;

/** Perform a bitwise XOR between two buffers. */
void inplace_xor(buffer_type &lhs, const buffer_type &rhs);

}

/** Chunk of raw data. Both the start and the end of the buffer can be
 *  modified, while remaining inside the allocated memory. The
 *  underlying memory must already be allocated. The template
 *  parameter specifies if the buffer data cannot be modified.
 */
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
  /** Construct a buffer with size 0. */
  base_buffer();
  /** Construct a buffer over the already allocated memory given by
   *  the arguments. The buffer will not free this memory upon
   *  destruction.
   */
  explicit base_buffer(void_ptr mem, std::size_t size);

  /** Pointer to the start of the allocated memory. */
  void_ptr allocated_memory() const;
  /** Size of the allocated memory. */
  std::size_t allocated_size() const;

  /** Access the data as an array of the given type. Returns either a
   *  T* or a const T*, depending on the value of constbuf.
   */
  template<typename T>
  auto data() const;

  /** Pointer to the start of the data. */
  byte_ptr begin() const;
  /** Pointer to the end of the data. */
  byte_ptr end() const;
  /** Size of the data. */
  std::size_t size() const;

  /** Remove the given number of bytes from the start of the buffer. */
  void trim_front(std::size_t num);
  /** Add the given number of bytes before the start of the
   *  buffer. The new bytes are not initialized.
   */
  void extend_front(std::size_t num);
  /** Remove the given number of bytes from the end of the buffer. */
  void trim_back(std::size_t num);
  /** Add the given number of bytes after the end of the buffer. The
   *  new bytes are not initialized.
   */
  void extend_back(std::size_t num);

  /** Return a new buffer that holds a subrange of the current
   *  one. The new buffer still has access to the whole allocated
   *  memory range.
   */
  base_buffer slice(std::size_t seek, std::size_t size) const;

  /** Allow an implicit conversion from mutable buffer to constant
   *  buffer.
   */
  operator base_buffer<true>() const;
  // Allow the conversion method to access the pointers of both kind
  // of buffers.
  template<bool cb>
  friend base_buffer<cb>::operator base_buffer<true>() const;

protected:
  byte_ptr membegin; /**< Start of the allocated memory. */
  byte_ptr bufbegin; /**< Start of the buffer. */
  byte_ptr memend; /**< End of the allocated memory. */
  byte_ptr bufend; /**< End of the buffer. */
};

// Defined only for the mutable buffer (and before the instantiation)
template<>
inline base_buffer<false>::operator base_buffer<true>() const {
  base_buffer<true> cbuf;
  cbuf.membegin = membegin;
  cbuf.bufbegin = bufbegin;
  cbuf.memend = memend;
  cbuf.bufend = bufend;
  return cbuf;
}

// Link from base_types.cpp
extern template class base_buffer<true>;
extern template class base_buffer<false>;

/** Alias for a base_buffer with mutable data. */
using buffer = base_buffer<false>;
/** Alias for a base_buffer with constant data. */
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

	 //// base_buffer<constbuf> template definitions ////
template<bool constbuf>
base_buffer<constbuf>::base_buffer() : base_buffer(nullptr, 0) {
}

template<bool constbuf>
base_buffer<constbuf>::base_buffer(void_ptr mem, std::size_t size) :
  membegin{reinterpret_cast<byte_ptr>(mem)},
  bufbegin{membegin},
  memend{membegin + size},
  bufend{memend} {
}

template<bool constbuf>
typename base_buffer<constbuf>::void_ptr
base_buffer<constbuf>::allocated_memory() const {
    return membegin;
}

template<bool constbuf>
std::size_t base_buffer<constbuf>::allocated_size() const {
  return memend - membegin;
}

template<bool constbuf>
template<typename T>
auto base_buffer<constbuf>::data() const {
  using ret_type = typename std::conditional<constbuf,
					     const T*,
					     T*>::type;
  return reinterpret_cast<ret_type>(bufbegin);
}

template<bool constbuf>
typename base_buffer<constbuf>::byte_ptr
base_buffer<constbuf>::begin() const {
  return bufbegin;
}

template<bool constbuf>
typename base_buffer<constbuf>::byte_ptr
base_buffer<constbuf>::end() const {
  return bufend;
}

template<bool constbuf>
std::size_t base_buffer<constbuf>::size() const {
  return bufend - bufbegin;
}

template<bool constbuf>
void base_buffer<constbuf>::trim_front(std::size_t num) {
  if (static_cast<std::size_t>(bufend - bufbegin) >= num)
    bufbegin += num;
  else
    throw std::range_error("Would trim after the end");
}

template<bool constbuf>
void base_buffer<constbuf>::extend_front(std::size_t num) {
  if (static_cast<std::size_t>(bufbegin - membegin) >= num)
    bufbegin -= num;
  else
    throw std::range_error("Would extend after the allocated memory");
}

template<bool constbuf>
void base_buffer<constbuf>::trim_back(std::size_t num) {
  if (static_cast<std::size_t>(bufend - bufbegin) >= num)
    bufend -= num;
  else
    throw std::range_error("Would trim after the beginning");
}

template<bool constbuf>
void base_buffer<constbuf>::extend_back(std::size_t num) {
  if (static_cast<std::size_t>(memend - bufend) >= num)
    bufend += num;
  else
    throw std::range_error("Would extend after the allocated memory");
}

template<bool constbuf>
base_buffer<constbuf>
base_buffer<constbuf>::slice(std::size_t seek, std::size_t size) const {
  base_buffer b{*this};
  b.trim_front(seek);
  if (this->size() - seek >= size) {
    b.trim_back(this->size() - seek  - size);
  }
  else {
    b.extend_back(size - this->size() + seek);
  }
  return b;
}

#endif
