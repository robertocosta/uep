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

  /** Construct a counter with the maximum value of the underlying
   *  integer type.
   */
  counter() : counter(std::numeric_limits<value_type>::max()) {}

  /** Initialize the counter before 0 and set the max_value. */
  explicit counter(value_type max_value) : max_seqno(max_value),
					   next_seqno(0),
					   has_overflown(false) {
    if (max_seqno <= 0) throw std::invalid_argument("The max cannot be <= 0");
  }

  /** Set the value of the counter. A subsequent call to last() will
   *  return value. A call to next() will return value + 1 (or
   *  overflow).
   */
  void set(value_type value) {
    if (value < 0 || value > max_seqno)
      throw std::invalid_argument("The value must be between 0 and max_value");
    next_seqno = value;
    has_overflown = false;
    next();
  }

  /** Set the value of the counter. \see set(value_type). */
  counter &operator=(value_type value) {
    set(value);
    return *this;
  }

  /** Return the current value of the counter, i.e. the last value
   *  output by next(). If the counter has yet to output 0, throws a
   *  std::underflow_error.
   */
  value_type last() const {
    if (next_seqno == 0) throw std::underflow_error("No last value");
    if (has_overflown) return max_seqno;
    return next_seqno-1;
  }

  /** Return the maximum value. */
  value_type max() const {
    return max_seqno;
  }

  /** Increment the value of the counter and return the incremented
   *  value. If the counter has reached the maximum value, throws
   *  std::overflow_error.
   */
  value_type next() {
    if (has_overflown)
      throw std::overflow_error("Seqno overflow");
    value_type n = next_seqno;
    if (n == max_seqno)
      has_overflown = true;
    else
      ++next_seqno;
    return n;
  }

  /** Prefix increment operator. */
  counter &operator++() {
    next();
    return *this;
  }

  /** Postfix increment operator. */
  counter &operator++(int) {
    counter tmp(*this);
    next();
    return tmp;
  }

  /** Reset the counter to the initial state. */
  void reset() {
    next_seqno = 0;
    has_overflown = false;
  }

  /** Compare two counters for equality. They compare equal if they
   *  are currently set to the same value, irrespective of their
   *  maximum values.
   */
  bool operator==(const counter &rhs) {
    if (!has_overflown && !rhs.has_overflown)
      return next_seqno == rhs.next_seqno;
    else if (has_overflown && rhs.has_overflown)
      return max_seqno == rhs.max_seqno;
    else if (has_overflown)
      return (rhs.max_seqno >= max_seqno) &&
	(rhs.next_seqno != 0) &&
	rhs.last() == max_seqno;
    else
      return (max_seqno >= rhs.max_seqno) &&
	(next_seqno != 0) &&
	last() == rhs.max_seqno;
  }

  /** Compare two counter for inequality. \see operator= */
  bool operator!=(const counter &rhs) {
    return !(*this == rhs);
  }

  /** True when the counter has overflown. That is, it is not possible
   *  to call next().
   */
  bool overflow() const {
    return has_overflown;
  }

private:
  value_type max_seqno;
  value_type next_seqno;
  bool has_overflown;
};

/** Counter that falls back to zero after it reaches a specified
 *  maximum value.
 */
template <class IntType = std::size_t>
class circular_counter {
public:
  typedef IntType value_type;

  /** Initialize a circular counter with the max value given by the
   *  underlying type.
   */
  circular_counter() :
    circular_counter(std::numeric_limits<value_type>::max()) {
  }

  /** Construct a circular counter with the given max value and
   *  max_value/2 as the window size.
   */
  explicit circular_counter(value_type max_value) :
    circular_counter(max_value, max_value/2) {
  }

  /** Construct a counter with the given maximum value and comparison
   *  window size, with 0 as initial_value.
   */
  explicit circular_counter(value_type max_value, value_type winsize) :
    max_val(max_value),
    window_size(winsize),
    curr_val(0) {
    if (max_val <= 0) throw std::invalid_argument("The max cannot be <= 0");
    if (window_size <= 0) throw std::invalid_argument("The winsize cannot be <= 0");
  }

  /** Set the current value of the counter. */
  void set(value_type value) {
    if (value < 0 || value > max_val)
      throw std::invalid_argument("The value must be between 0 and max_value");
    curr_val = value;
  }

  /** Set the value. \see set(value_type). */
  circular_counter &operator=(value_type value) {
    set(value);
    return *this;
  }

  /** Return the current value of the counter (use value() instead). */
  value_type last() const {
    return curr_val;
  }

  /** Return the current value of the counter. */
  value_type value() const {
    return curr_val;
  }

  /** Return the maximum value. */
  value_type max() const {
    return max_val;
  }

  /** Return the window used when comparing counters. */
  value_type comparison_window() const {
    return window_size;
  }

  /** Increment the counter and return the incremented value.  After
   *  the counter reaches the maximum value it restarts from 0.
   */
  value_type next() {
    if (curr_val == max_val) {
      curr_val = 0;
    }
    else {
      ++curr_val;
    }
    return curr_val;
  }

  /** Prefix increment operator. */
  circular_counter &operator++() {
    next();
    return *this;
  }

  /** Postfix increment operator. */
  circular_counter &operator++(int) {
    circular_counter tmp(*this);
    next();
    return tmp;
  }

  /** Increment the counter `n` times and return the incremnted
   *  value. The increment count `n` can not bring the counter outside
   *  of the comparison window. \sa next()
   */
  value_type next(value_type n) {
    if (n > window_size)
      throw std::invalid_argument("Would fall out of the comparison window");

    if (max_val - curr_val >= n) {
      curr_val += n;
    }
    else {
      curr_val = n - (max_val - curr_val + 1);
    }
    return curr_val;
  }

  /** Reset the counter to the initial state. */
  void reset() {
    curr_val = 0;
  }

  /** Compute the distance between this counter and the other counter.
   *  The forward distance is the number of calls to next() required
   *  to obtain the same value of other.
   */
  value_type forward_distance(const circular_counter &other) const {
    if (other.max() != max())
      throw std::invalid_argument("Must use the same range");

    value_type ov = other.curr_val;
    value_type v = curr_val;;
    if (ov >= v)
      return ov - v;
    else
      return max() - v + ov + 1;
  }

  /** Compute the backward distance. This is the number of times
   *  `other` has to to be incremented to reach the value of this
   *  counter.
   */
  value_type backward_distance(const circular_counter &other) const {
    return other.forward_distance(*this);
  }

  /** True if this counter comes before the other counter within the
   *  comparison window.
   */
  bool is_before(const circular_counter &other) const {
    if (comparison_window() != other.comparison_window())
      throw std::invalid_argument("Cannot compare with different winsizes");

    value_type d = forward_distance(other);
    return d > 0 && d <= comparison_window();
  }

  /** True if this counter comes after the other counter within the
   *  comparison window.
   */
  bool is_after(const circular_counter &other) const {
    if (comparison_window() != other.comparison_window())
      throw std::invalid_argument("Cannot compare with different winsizes");

    value_type d = backward_distance(other);
    return d > 0 && d <= comparison_window();
  }

  /** True when the two counters have the same range and the same
   *  value.
   */
  bool operator==(const circular_counter &other) const {
    return forward_distance(other) == 0;
  }

  /** True when the two counters are different. */
  bool operator!=(const circular_counter &other) const {
    return !operator==(other);
  }

private:
  value_type max_val;
  value_type window_size;
  value_type curr_val;
};

namespace stat {

class average_counter {
public:
  average_counter() : n(0) {}

  void add_sample(double s) {
    if (n == 0) {
      avg = s;
    }
    else {
      avg = (avg * n + s) / (n+1);
    }
    ++n;
  }

  double value() const {
    if (n == 0) throw std::runtime_error("No samples");
    return avg;
  }

  std::size_t count() const {
    return n;
  }

  void reset() {
    n = 0;
  }

private:
  std::size_t n;
  double avg;
};

}}

#endif
