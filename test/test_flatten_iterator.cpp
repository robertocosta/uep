#define BOOST_TEST_MODULE test_flatten_iterator

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "flatten_iterator.hpp"

using namespace std;
using namespace uep;

struct flatten_setup {
  vector<vector<string>> v;
  flatten_setup() {
    v.push_back({"A", "B", "C"});
    v.push_back({"D", "E", "F"});
  }
};

BOOST_FIXTURE_TEST_CASE(flatten, flatten_setup) {
  typedef decltype(v)::iterator outer_it;
  auto fl_r = flatten_range(v.begin(), v.end());
  flatten_iterator<outer_it> fl_null,flb = fl_r.first, fle = fl_r.second;
  BOOST_CHECK(fl_null != flb);
  BOOST_CHECK(fl_null != fle);
  BOOST_CHECK(flb != fle);

  BOOST_CHECK_EQUAL(*flb++, "A");
  BOOST_CHECK_EQUAL(*flb++, "B");
  BOOST_CHECK_EQUAL(*flb, "C");
  ++flb;
  string s = *flb;
  BOOST_CHECK_EQUAL(s, "D");
  ++flb;
  BOOST_CHECK_EQUAL(*flb++, "E");
  BOOST_CHECK_EQUAL(*flb, "F");
  BOOST_CHECK(flb != fle);
  ++flb;
  BOOST_CHECK(flb == fle);


}

BOOST_FIXTURE_TEST_CASE(flatten_algo, flatten_setup) {
  const vector<string> flat{"A", "B", "C", "D", "E", "F"};
  auto fl_r = flatten_range(v.begin(), v.end());
  bool e = equal(fl_r.first, fl_r.second, flat.cbegin());
  BOOST_CHECK(e);
}

BOOST_FIXTURE_TEST_CASE(const_flatten, flatten_setup) {
  const vector<string> flat{"A", "B", "C", "D", "E", "F"};
  auto fl_r = flatten_range(v.cbegin(), v.cend());
  bool e = equal(fl_r.first, fl_r.second, flat.cbegin());
  BOOST_CHECK(e);
}

BOOST_AUTO_TEST_CASE(empty_flatten) {
  vector<vector<string>> v;
  v.push_back({});
  v.push_back({"A"});
  v.push_back({});
  v.push_back({"B"});

  vector<string> flat{"A", "B"};

  auto fl_r = flatten_range(v.cbegin(), v.cend());
  bool e = equal(fl_r.first, fl_r.second, flat.cbegin());
  BOOST_CHECK(e);
}
