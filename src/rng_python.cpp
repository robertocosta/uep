#include "rng.hpp"

#include <boost/python.hpp>

using namespace std;

BOOST_PYTHON_MODULE(rng_python)
{
  typedef fountain_<soliton_distribution>::generator_type soliton_gen_t;
  using namespace boost::python;
  class_<soliton_gen_t>("soliton_generator");

  class_<soliton_distribution>("soliton_distribution", init<uint_fast32_t>())
    .def("K", &soliton_distribution::K)
    .def("generate", &soliton_distribution::operator()<soliton_gen_t>);
  
}
