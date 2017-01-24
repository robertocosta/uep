#include <boost/python.hpp>
#include "rng.hpp"

void test();

BOOST_PYTHON_MODULE(testpy) {
  using namespace boost::python;
  def("test_rng", test);
}
