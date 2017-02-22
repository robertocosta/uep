#include <cstdint>

#define BOOST_TEST_MODULE test_counters

#include <boost/mpl/list.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "counter.hpp"

using namespace std;
using namespace uep;

typedef boost::mpl::list<int, long, size_t, uint_least16_t> int_types;
typedef boost::mpl::list<int, long> signed_int_types;
typedef boost::mpl::list<size_t, uint_least16_t> unsigned_int_types;

BOOST_AUTO_TEST_CASE_TEMPLATE(counter_full_range, T, int_types) {
  T max = 0xffff;
  counter<T> c(max);
  BOOST_CHECK_EQUAL(c.max(), max);
  BOOST_CHECK_THROW(c.last(), underflow_error);
  for(T i = 0; i < max; i++) {
    T j = c.next();
    BOOST_CHECK_EQUAL(i, j);
    T l = c.last();
    BOOST_CHECK_EQUAL(i, l);
  }
  T max_j = c.next();
  T max_l = c.last();
  BOOST_CHECK_EQUAL(max_j, max);
  BOOST_CHECK_EQUAL(max_l, max);
  BOOST_CHECK_THROW(c.next(), overflow_error);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(counter_set, T, int_types) {
  T max = 0xffff;
  counter<T> c(max);

  c.set(0xff00);
  BOOST_CHECK_EQUAL(c.last(), 0xff00);
  BOOST_CHECK_EQUAL(c.next(), 0xff01);

  c.reset();
  BOOST_CHECK_THROW(c.last(), underflow_error);
  BOOST_CHECK_EQUAL(c.next(), 0);

  c.set(max);
  BOOST_CHECK_EQUAL(c.last(), max);
  BOOST_CHECK_THROW(c.next(), overflow_error);

  c.set(0);
  BOOST_CHECK_EQUAL(c.last(), 0);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(counter_negative_s, T, signed_int_types) {
  BOOST_CHECK_THROW(counter<T>(-3), invalid_argument);
  counter<T> c(100);
  BOOST_CHECK_THROW(c.set(-1), invalid_argument);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(counter_negative_u, T, unsigned_int_types) {
  BOOST_CHECK_NO_THROW(counter<T>(-3));
  counter<T> c1(100);
  BOOST_CHECK_THROW(c1.set(-1), invalid_argument);

  counter<T> c2(numeric_limits<T>::max());
  BOOST_CHECK_NO_THROW(c2.set(-1));
  BOOST_CHECK_EQUAL(c2.last(), numeric_limits<T>::max());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(circular_full_range, T, int_types) {
  T max = 0xffff;
  circular_counter<T> c(max);
  BOOST_CHECK_EQUAL(c.max(), max);
  BOOST_CHECK_THROW(c.last(), underflow_error);
  for(T i = 0; i < max; i++) {
    T j = c.next();
    BOOST_CHECK_EQUAL(i, j);
    T l = c.last();
    BOOST_CHECK_EQUAL(i, l);
  }
  T max_j = c.next();
  T max_l = c.last();
  BOOST_CHECK_EQUAL(max_j, max);
  BOOST_CHECK_EQUAL(max_l, max);
  BOOST_CHECK_EQUAL(c.next(), 0);
  BOOST_CHECK_EQUAL(c.last(), 0);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(circular_many_loops, T, int_types) {
  T max = 100;
  circular_counter<T> c(max);
  for (T j = 0; j < 100; ++j) {
    for(T i = 0; i <= max; ++i) {
      BOOST_CHECK_EQUAL(i, c.next());
      BOOST_CHECK_EQUAL(i, c.last());
    }
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(circular_set, T, int_types) {
  T max = 100;
  circular_counter<T> c(max);
  c.set(50);
  BOOST_CHECK_EQUAL(c.last(), 50);
  BOOST_CHECK_EQUAL(c.next(), 51);

  c.set(max);
  BOOST_CHECK_EQUAL(c.last(), max);
  BOOST_CHECK_EQUAL(c.next(), 0);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(circular_loop_limit, T, int_types) {
  T max = numeric_limits<T>::max();
  circular_counter<T> c(max);
  c.set(max - 1);
  BOOST_CHECK_EQUAL(c.last(), max - 1);
  BOOST_CHECK_EQUAL(c.next(), max);
  BOOST_CHECK_EQUAL(c.last(), max);
  BOOST_CHECK_EQUAL(c.next(), 0);
  BOOST_CHECK_EQUAL(c.last(), 0);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(circular_negative_s, T, signed_int_types) {
  BOOST_CHECK_THROW(circular_counter<T>(-3), invalid_argument);
  circular_counter<T> c(100);
  BOOST_CHECK_THROW(c.set(-1), invalid_argument);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(circular_negative_u, T, unsigned_int_types) {
  BOOST_CHECK_NO_THROW(circular_counter<T>(-3));
  circular_counter<T> c1(100);
  BOOST_CHECK_THROW(c1.set(-1), invalid_argument);

  circular_counter<T> c2(numeric_limits<T>::max());
  BOOST_CHECK_NO_THROW(c2.set(-1));
  BOOST_CHECK_EQUAL(c2.last(), numeric_limits<T>::max());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(circular_distance, T, int_types) {
  T max = 0xffff;
  BOOST_CHECK_EQUAL(max, numeric_limits<uint_least16_t>::max());
  circular_counter<T> c1(max);
  circular_counter<T> c2(max);
  circular_counter<T> c3(max-1);
  BOOST_CHECK_THROW(c3.forward_distance(c1), invalid_argument);
  
  c1.set(0xfff0);
  c2.set(0xffff);
  T d12 = c1.forward_distance(c2);
  T d21 = c2.forward_distance(c1);
  BOOST_CHECK_EQUAL(d12, 0xf);
  BOOST_CHECK_EQUAL(d21, 1 + 0xfff0);

  c1.set(max);
  c2.set(0);
  d12 = c1.forward_distance(c2);
  d21 = c2.forward_distance(c1);
  BOOST_CHECK_EQUAL(d12, 1);
  BOOST_CHECK_EQUAL(d21, max);

  c1.set(0xff00);
  c2.set(0xff00);
  d12 = c1.forward_distance(c2);
  d21 = c2.forward_distance(c1);
  BOOST_CHECK_EQUAL(d12, 0);
  BOOST_CHECK_EQUAL(d21, 0);
}
