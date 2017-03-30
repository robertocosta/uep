#define BOOST_TEST_MODULE test_data_client_server
#include <iostream>

#include <boost/test/unit_test.hpp>

#include "data_client_server.hpp"
#include "encoder.hpp"
#include "decoder.hpp"


struct random_packet_source {
  struct parameter_set {
    std::mt19937::result_type seed;
    std::size_t size;
    std::size_t max_count;
  };

  std::independent_bits_engine<std::mt19937, CHAR_BIT, unsigned char> rng;
  std::size_t size;
  std::size_t max_count;
  std::size_t count;

  explicit random_packet_source(const parameter_set &ps) :
    rng(ps.seed), size(ps.size), max_count(ps.max_count), count(0) {};

  packet next_packet() {
    if (count == max_count) throw std::runtime_error("Max packet count");
    else ++count;
    packet p;
    p.resize(size);
    for (std::size_t i=0; i < size; i++) {
      p[i] = rng();
    }
    return p;
  }

  explicit operator bool() const {
    return count < max_count;
  }

  bool operator!() const { return !static_cast<bool>(*this); }
};

struct memory_sink {
  struct parameter_set {};
  explicit memory_sink(const parameter_set&) {}

  std::vector<packet> received;

  void push(const packet &p) {
    packet p_copy(p);
    using std::move;
    push(move(p));
  }

  void push(packet &&p) {
    using std::move;
    received.push_back(move(p));
  }

  explicit operator bool() const { return true; }
  bool operator!() const { return false; }
};

using namespace std;
using namespace uep;
using namespace uep::net;

using namespace boost::asio;

struct setup_cs {
  io_service io;

  std::size_t L = 1024;

  lt_encoder<std::mt19937>::parameter_set enc_ps;
  lt_decoder::parameter_set dec_ps;
  random_packet_source::parameter_set src_ps;
  memory_sink::parameter_set sink_ps;

  std::vector<packet> original;

  data_server<lt_encoder<std::mt19937>,random_packet_source> ds;
  data_client<lt_decoder,memory_sink> dc;

  setup_cs() :
    enc_ps{100, 0.1, 0.5},
    dec_ps{100, 0.1, 0.5},
    src_ps{0x42, L, 100*10},
    ds(io),
    dc(io) {
      ds.setup_encoder(enc_ps);
      ds.setup_source(src_ps);
      ds.target_send_rate(1024);
      ds.open("127.0.0.1", "9999");



      BOOST_CHECK_EQUAL(src_ps.seed, 0x42);
      BOOST_CHECK_EQUAL(src_ps.size, L);
      BOOST_CHECK_EQUAL(src_ps.max_count, 100*10);

      dc.setup_decoder(dec_ps);
      dc.setup_sink(sink_ps);
      dc.bind("9999");
  }
};

BOOST_FIXTURE_TEST_CASE(test_send, setup_cs) {
  //BOOST_CHECK(enc.has_block());

  auto srv_ep = ds.server_endpoint();
  dc.start_receive(srv_ep);

  boost::asio::steady_timer end_timer(io, std::chrono::seconds(10));
  end_timer.async_wait([this]
		       (const boost::system::error_code&) -> void {
			 cerr << "Stopping" << endl;
			 ds.stop();
			 dc.close();
		       });
  ds.start();
  io.run();

  BOOST_CHECK(equal(original.cbegin(), original.cend(),
		    dc.sink().received.cbegin()));
}
