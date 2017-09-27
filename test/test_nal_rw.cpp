#define BOOST_TEST_MODULE test_nal_rw
#include <boost/test/unit_test.hpp>

#include "nal_reader.hpp"
#include "nal_writer.hpp"

using namespace std;
using namespace uep;

bool compare_streams(const std::string &fileA, const std::string &fileB) {
  ifstream a,b;
  a.open(fileA, ios_base::binary);
  b.open(fileB, ios_base::binary);

  std::vector<char> awhole;
  std::vector<char> bwhole;
  
  std::array<char, 4096> abuf, bbuf;
  
  while (a && b) {
    abuf.fill(0);
    bbuf.fill(0);
    
    a.read(abuf.data(), abuf.size());
    b.read(bbuf.data(), bbuf.size());

    awhole.insert(awhole.end(), abuf.cbegin(), abuf.cend());
    bwhole.insert(bwhole.end(), bbuf.cbegin(), bbuf.cend());
  }
  if (!(a.eof() && b.eof())) return false;

  auto i = awhole.cbegin();
  auto j = bwhole.cbegin();
  while (i != awhole.cend() && j != bwhole.cend()) {
    auto sc_a = find_startcode(i, awhole.cend());
    auto sc_b = find_startcode(j, bwhole.cend());
    auto end_a = find_nal_end(i+3, awhole.cend());
    auto end_b = find_nal_end(j+3, bwhole.cend());

    if (end_a - sc_a != end_b - sc_b) return false;    
    if (!std::equal(sc_a, end_a, sc_b)) return false;

    i = end_a;
    j = end_b;
  }

  if (i != awhole.cend()) {
    if (find_startcode(i, awhole.cend()) != awhole.cend())
      return false;
  }
  if (j != bwhole.cend()) {
    if (find_startcode(j, bwhole.cend()) != bwhole.cend())
      return false;
  }
  
  return true;
}

// Set globally the log severity level
struct global_fixture {
  global_fixture() {
    uep::log::init();
    // auto warn_filter = boost::log::expressions::attr<
    //  uep::log::severity_level>("Severity") >= uep::log::warning;
    // boost::log::core::get()->set_filter(warn_filter);
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

BOOST_AUTO_TEST_CASE(nal_read_write) {
  nal_reader::parameter_set ps;
  ps.streamName = "CREW_352x288_30_orig_01";
  ps.packet_size = 512;
  nal_reader r(ps);
  nal_writer w(r.header(), ps.streamName);

  while (r) {
    fountain_packet fp = r.next_packet();
    w.push(fp);
  }
  w.flush();

  BOOST_CHECK(compare_streams("dataset/CREW_352x288_30_orig_01.264",
			      "dataset/CREW_352x288_30_orig_01.264"));
  BOOST_CHECK(compare_streams("dataset_client/CREW_352x288_30_orig_01.264",
			      "dataset_client/CREW_352x288_30_orig_01.264"));  
  
  BOOST_CHECK(compare_streams("dataset/CREW_352x288_30_orig_01.264",
			      "dataset_client/CREW_352x288_30_orig_01.264"));  
}
