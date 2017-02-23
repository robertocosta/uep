#include <vector>

#define BOOST_TEST_MODULE test_skip_iterator

//#include <boost/mpl/list.hpp>
//#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "skip_iterator.hpp"

using namespace std;
using namespace uep;

bool is_zero(int i) { return i == 0; }

BOOST_AUTO_TEST_CASE(vector_skip_iterator) {
  vector<int> v{1,2,3,0,0,4,0,0,5};
  auto begin = v.cbegin();
  auto end = v.cend();

  auto skip_i = make_skip_iterator(begin, is_zero, end);

  BOOST_CHECK_EQUAL(*skip_i++, 1);
  BOOST_CHECK_EQUAL(*skip_i++, 2);
  BOOST_CHECK_EQUAL(*skip_i++, 3);
  BOOST_CHECK_EQUAL(*skip_i++, 4);
  BOOST_CHECK_EQUAL(*skip_i++, 5);
  BOOST_CHECK(end == skip_i.base());

  v.push_back(0);
  v.insert(v.begin(), 0);
  begin = v.cbegin();
  end = v.cend();
  skip_i = make_skip_iterator(begin, is_zero, end);
  BOOST_CHECK_EQUAL(*begin, 0);
  BOOST_CHECK_EQUAL(*skip_i, 1);

  while (*skip_i != 5) ++skip_i;
  skip_i++;
  BOOST_CHECK(skip_i.base() == end);
}

BOOST_AUTO_TEST_CASE(vector_allzeros) {
  vector<int> v(100, 0);
  auto skip_start = make_skip_iterator(v.begin(), is_zero, v.end());
  auto skip_end = make_skip_iterator(v.end(), is_zero, v.end());

  BOOST_CHECK(skip_end == skip_start);
  BOOST_CHECK(skip_end.base() == v.end());
  BOOST_CHECK(skip_start.base() == v.end());
}

BOOST_AUTO_TEST_CASE(vector_empty) {
  vector<int> v;
  auto skip_start = make_skip_iterator(v.begin(), is_zero, v.end());
  auto skip_end = make_skip_iterator(v.end(), is_zero, v.end());

  BOOST_CHECK(skip_end == skip_start);
  BOOST_CHECK(skip_end.base() == v.end());
  BOOST_CHECK(skip_start.base() == v.end());
}

BOOST_AUTO_TEST_CASE(algorithms) {
  vector<int> v{0,1,2,3,0,0,0,1,2,3,0};
  auto skip_begin = make_skip_iterator(v.cbegin(), is_zero, v.cend());
  auto skip_end = make_skip_iterator(v.cend(), is_zero, v.cend());
  int c = count_if(skip_begin, skip_end, [](int) -> bool {return true;});
  BOOST_CHECK_EQUAL(c, 6);
}

BOOST_AUTO_TEST_CASE(auto_skip) {
  vector<int> v{0,1,2,3,0,4,0,0,5,0};
  BOOST_CHECK(!static_cast<bool>(0));
  BOOST_CHECK(static_cast<bool>(1));

  auto skip_begin = make_skip_iterator(v.cbegin(), v.cend());
  auto skip_end = make_skip_iterator(v.cend(), v.cend());

  BOOST_CHECK_EQUAL(*skip_begin++, 1);
  BOOST_CHECK_EQUAL(*skip_begin++, 2);
  BOOST_CHECK_EQUAL(*skip_begin++, 3);
  BOOST_CHECK_EQUAL(*skip_begin++, 4);
  BOOST_CHECK_EQUAL(*skip_begin++, 5);
  BOOST_CHECK(skip_begin == skip_end);
  BOOST_CHECK(skip_begin.base() == v.cend());
}

BOOST_AUTO_TEST_CASE(modify_elements) {
  vector<int> v{0,1,2,3,0,4,0,0,5,0};
  auto skip_begin = make_skip_iterator(v.begin(), v.end());
  auto skip_end = make_skip_iterator(v.end(), v.end());
  for (; skip_begin != skip_end; ++skip_begin) {
    *skip_begin = 99;
  }
  const vector<int> expected{0,99,99,99,0,99,0,0,99,0};
  BOOST_CHECK_EQUAL(v.size(), expected.size());
  BOOST_CHECK(equal(v.cbegin(), v.cend(), expected.cbegin()));
}
