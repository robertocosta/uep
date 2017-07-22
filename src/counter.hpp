#ifndef UEP_COUNTER_HPP
#define UEP_COUNTER_HPP

#include <cstddef>
#include <limits>
#include <stdexcept>

namespace uep {

/** Counter that overflows after a specified maximum value. */
template <class IntType = std::size_t>
class counter {
public:
  typedef IntType value_type;

  /** Initialize the counter before 0 and set the max_value. */
  explicit counter(IntType max_value) : max_seqno(max_value),
					next_seqno(0),
					has_overflown(false) {
    if (max_seqno <= 0) throw std::invalid_argument("The max cannot be <= 0");
  }

  /** Set the value of the counter. A subsequent call to last() will
   *  return value. A call to next() will return value + 1 (or
   *  overflow).
   */
  void set(IntType value) {
    if (value < 0 || value > max_seqno)
      throw std::invalid_argument("The value must be between 0 and max_value");
    next_seqno = value;
    has_overflown = false;
    next();
  }

  /** Return the current value of the counter, i.e. the last value
   *  output by next(). If the counter has yet to output 0, throws a
   *  std::underflow_error.
   */
  IntType last() const {
    if (next_seqno == 0) throw std::underflow_error("No last value");
    if (has_overflown) return max_seqno;
    return next_seqno-1;
  }

  /** Return the maximum value. */
  IntType max() const {
    return max_seqno;
  }

  /** Increment the value of the counter and return the incremented
   *  value. If the counter has reached the maximum value, throws
   *  std::overflow_error.
   */
  IntType next() {
    if (has_overflown)
      throw std::overflow_error("Seqno overflow");
    IntType n = next_seqno;
    if (n == max_seqno)
      has_overflown = true;
    else
      ++next_seqno;
    return n;
  }

  /** Reset the counter to the initial state. */
  void reset() {
    next_seqno = 0;
    has_overflown = false;
  }

  // counter &operator++() {
  //   next();
  //   return *this;
  // }

  // counter operator++(int) {
  //   return next();
  // }

  // explicit operator IntType() const {
  //   return last();
  // }

  // bool operator==(const counter &rhs) {
  //   if (!has_overflown && !rhs.has_overflown)
  //     return next_seqno == rhs.next_seqno;
  //   else if (has_overflown && rhs.has_overflown)
  //     return max_value == rhs.max_value;
  //   else if (has_overflown)
  //     return (rhs.max_seqno >= max_seqno) &&
  //	(rhs.next_seqno != 0) &&
  //	rhs.last() == max_seqno;
  //   else
  //     return (max_seqno >= rhs.max_seqno) &&
  //	(next_seqno != 0) &&
  //	last() == rhs.max_seqno;
  // }

  // bool operator!=(const counter &rhs) {
  //   return !(*this == rhs);
  // }

private:
  const IntType max_seqno;
  IntType next_seqno;
  bool has_overflown;
};

/** Counter that falls back to zero after it reaches a specified
 *  maximum value.
 */
template <class IntType = std::size_t>
class circular_counter {
public:
  typedef IntType value_type;

  /** Initialize the counter before 0 and set the maximum value. */
  explicit circular_counter(IntType max_value) : max_seqno(max_value),
						 next_seqno(0),
						 looped_once(false) {
    if (max_seqno <= 0) throw std::invalid_argument("The max cannot be <= 0");
  }

  circular_counter &operator=(const circular_counter &other) {
    if (other.max_seqno != max_seqno)
      throw std::runtime_error("The max values must be the same");
    next_seqno = other.next_seqno;
    looped_once = other.looped_once;
    return *this;
  }

  /** Set the current value of the counter. */
  void set(IntType value) {
    if (value < 0 || value > max_seqno)
      throw std::invalid_argument("The value must be between 0 and max_value");
    next_seqno = value;
    looped_once = false;
    next();
  }

  /** Return the current value of the counter, i.e. the last output of
   *  next(). If next() was never called throw std::underflow_error.
   */
  IntType last() const {
    if (next_seqno == 0) {
      if (looped_once)
	return max_seqno;
      else
	throw std::underflow_error("No last value");
    }
    else return next_seqno-1;
  }

  /** Return the maximum value. */
  IntType max() const {
    return max_seqno;
  }

  /** Increment the counter and return the incremented value.  After
   *  the counter reaches the maximum value it restarts from 0.
   */
  IntType next() {
    IntType n = next_seqno;
    if (n == max_seqno) {
      looped_once = true;
      next_seqno = 0;
    }
    else
      ++next_seqno;
    return n;
  }

  /** Reset the counter to the initial state. */
  void reset() {
    next_seqno = 0;
    looped_once = false;
  }

  /** Compute the distance between this counter and the other counter.
   *  The forward distance is the number of calls to next() required
   *  to obtain the same value of other.
   */
  IntType forward_distance(const circular_counter &other) {
    if (other.max() != max())
      throw std::invalid_argument("Must use the same range");
    if (!looped_once && next_seqno == 0)
      return !other.looped_once && (other.next_seqno == 0);

    IntType ol = other.last();
    IntType l = last();
    if (ol >= l)
      return ol - l;
    else
      return max() - l + ol + 1;
  }

private:
  const IntType max_seqno;
  IntType next_seqno;
  bool looped_once;
};

}

#endif
