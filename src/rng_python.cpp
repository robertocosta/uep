#include "rng.hpp"

#include <boost/python.hpp>

#include <iostream>

using namespace std;

void hello_python_method() {
  cout << "Hello" << endl;
}

BOOST_PYTHON_MODULE(rng_python)
{
    using namespace boost::python;
    def("hello", hello_python_method);
}
