#define BOOST_TEST_MODULE test_nal_rw
#include <boost/test/unit_test.hpp>

#include "nal_reader.hpp"

using namespace std;
using namespace uep;

// Set globally the log severity level
struct global_fixture {
  global_fixture() {
    uep::log::init();
    auto warn_filter = boost::log::expressions::attr<
     uep::log::severity_level>("Severity") >= uep::log::warning;
    boost::log::core::get()->set_filter(warn_filter);
  }

  ~global_fixture() {
  }
};
BOOST_GLOBAL_FIXTURE(global_fixture);


BOOST_AUTO_TEST_CASE(nal_read) {
  nal_reader::parameter_set ps;
  ps.streamName = "CREW_352x288_30_orig_01";
  ps.packet_size = 512;
  nal_reader r(ps);
  BOOST_CHECK(!r.header().empty());
  BOOST_CHECK_EQUAL(r.header().size(), 451+15+16+8+8-1); // One byte
							 // shifted to
							 // the next
							 // packets

  BOOST_CHECK(r.has_packet());
  BOOST_CHECK(static_cast<bool>(r));

  size_t npkts = 0;

  fountain_packet fp;
  for (size_t i = 0; i < 14; ++i) {
    fp = r.next_packet();
    ++npkts;
    BOOST_CHECK_EQUAL(fp.getPriority(), 0);
    BOOST_CHECK_EQUAL(fp.buffer().size(), ps.packet_size);
  }
  BOOST_CHECK(all_of(fp.buffer().cend() - 137, fp.buffer().cend(),
		     [](char c){return c == 0;}));
  BOOST_CHECK_NE(*(fp.buffer().cend() - 138), 0);

  for (size_t i = 0; i < 16; ++i) {
    fp = r.next_packet();
    ++npkts;
    BOOST_CHECK_EQUAL(fp.getPriority(), 1);
    BOOST_CHECK_EQUAL(fp.buffer().size(), ps.packet_size);
  }
  BOOST_CHECK(all_of(fp.buffer().cend() - 310, fp.buffer().cend(),
		     [](char c){return c == 0;}));
  BOOST_CHECK_NE(*(fp.buffer().cend() - 311), 0);

  while(r) {
    fp = r.next_packet();
    ++npkts;
  }
  BOOST_CHECK_EQUAL(fp.getPriority(), 1);
  BOOST_CHECK(all_of(fp.buffer().cend() - 21, fp.buffer().cend(),
		     [](char c){return c == 0;}));
  BOOST_CHECK_NE(*(fp.buffer().cend() - 22), 0);

  BOOST_CHECK_GE(npkts, ceil(static_cast<double>(r.totLength()) / ps.packet_size));
}
