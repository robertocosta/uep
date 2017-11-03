#include <cmath>
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
  BOOST_CHECK_EQUAL(c.last(), 0);
  BOOST_CHECK_EQUAL(c.value(), 0);
  for(T i = 1; i < max; i++) {
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
      BOOST_CHECK_EQUAL(i, c.last());
      if (i < max)
	BOOST_CHECK_EQUAL(i+1, c.next());
      else
	BOOST_CHECK_EQUAL(0, c.next());
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

BOOST_AUTO_TEST_CASE_TEMPLATE(equality, T, int_types) {
  T max = 0xffff;
  counter<T> c1(max), c2(max), c3(0xff);
  BOOST_CHECK(c1 == c2);
  BOOST_CHECK(c2 == c3);
  BOOST_CHECK(c1 == c3);

  c1 = 0xff; c3 = 0xff;
  BOOST_CHECK(c2 != c1);
  BOOST_CHECK(c2 != c3);
  BOOST_CHECK(c1 == c3);

  BOOST_CHECK(c3.overflow());
  ++c1;
  BOOST_CHECK(c1 != c3);
  BOOST_CHECK(c3 != c1);

  c1 = max;
  BOOST_CHECK(c1.overflow());
  BOOST_CHECK(c1 != c3);
  BOOST_CHECK(c3 != c1);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(circular_comparison, T, int_types) {
  T max = 0xffff;
  T win = 0xff;
  circular_counter<T> c1(max,win), c2(max,win);
  BOOST_CHECK(c1 == c2);
  BOOST_CHECK(!c1.is_after(c2));
  BOOST_CHECK(!c1.is_before(c2));

  c1 = 0xff;
  BOOST_CHECK(c1 != c2);
  BOOST_CHECK(c1.is_after(c2));
  BOOST_CHECK(c2.is_before(c1));

  ++c1;
  BOOST_CHECK(!c1.is_after(c2));
  BOOST_CHECK(!c1.is_before(c2));

  c2 = 0x100;
  BOOST_CHECK(c1 == c2);
  BOOST_CHECK(!c1.is_after(c2));
  BOOST_CHECK(!c1.is_before(c2));

  c1 = max;
  c2 = 0;
  BOOST_CHECK(c2.is_after(c1));
  BOOST_CHECK(c1.is_before(c2));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(multiple_increment, T, int_types) {
  T max = 0xffff;
  T win = 100;
  circular_counter<T> c1(max, win);
  auto c2 = c1;

  c1.next(0);
  BOOST_CHECK(c1 == c2);

  c1.next(1);
  c2.next();
  BOOST_CHECK(c1 == c2);

  BOOST_CHECK_THROW(c1.next(win+1), std::invalid_argument);

  c1 = 0xfff0;
  c2 = 0xffff;
  c1.next(15);
  BOOST_CHECK(c1 == c2);

  c1 = 0xfff0;
  c2 = 0;
  c1.next(16);
  BOOST_CHECK(c1 == c2);

  c1 = 0xfff0;
  c2 = 16;
  c1.next(32);
  BOOST_CHECK(c1 == c2);

  c1 = 0xffff;
  c2 = 10;
  c1.next(11);
  BOOST_CHECK(c1 == c2);
}

using namespace uep::stat;

BOOST_AUTO_TEST_CASE(avg) {
  average_counter ac;

  BOOST_CHECK_EQUAL(ac.count(), 0);
  BOOST_CHECK(std::isnan(ac.value()));

  ac.add_sample(1.0);
  BOOST_CHECK_EQUAL(ac.count(), 1);
  BOOST_CHECK_EQUAL(ac.value(), 1);

  ac.add_sample(10.0);
  BOOST_CHECK_EQUAL(ac.count(), 2);
  BOOST_CHECK_CLOSE(ac.value(), 5.5, 1e-9);

  ac.reset();
  BOOST_CHECK_EQUAL(ac.count(), 0);

  for (int i = 0; i < 100; ++i) {
    ac.add_sample(i);
  }
  BOOST_CHECK_CLOSE(ac.value(), ((99.0*100)/2) / 100, 1e-9);
}
